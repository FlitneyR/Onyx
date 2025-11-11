#pragma once

#include "Entity.h"

#include "tracy/Tracy.hpp"

#include <vector>

namespace onyx::ecs
{

// forward declaration from World.h
struct World;

template< typename Component >
struct ComponentTable;

struct GenericComponentTable
{
	template< typename Component >
	bool IsOfType() const { return m_metaData.componentType == typeid( Component ).hash_code(); }

	struct Page
	{
		Page( size_t component_size, u32 page_id )
			: m_components( malloc( component_size * 16 ) )
			, m_pageId( page_id )
		{}

		void FreeComponents()
		{
			if ( m_components )
			{
				free( m_components );
				m_components = nullptr;
			}
		}

		~Page() = default;
		Page() = default;
		Page( Page&& other ) noexcept : Page() { *this = std::move( other ); }

		Page& operator =( Page&& other ) noexcept
		{
			std::swap( m_components, other.m_components );
			std::swap( m_pageId, other.m_pageId );
			std::swap( m_occupancy, other.m_occupancy );
			std::swap( m_dirty, other.m_dirty );

			return *this;
		}

		Page( const Page& other ) = delete;
		Page& operator =( const Page& other ) = delete;

		constexpr static u32 c_pageIndexMask = 0xf;
		constexpr static u32 c_pageIdMask = ~c_pageIndexMask;

		void* m_components = nullptr;

		// the top 28 bits of the page id match the top 28 bits of the entity ids of all of its components
		// the bottom 4 bits are 0
		u32 m_pageId = 0;
		u16 m_occupancy = 0;
		u16 m_dirty = 0;

		template< typename Component >
		void Clear()
		{
			if ( !m_components )
				return;

			u8 next_occupant;
			while ( (next_occupant = GetNextOccupantIndex()) < 16 )
				RemoveComponent< Component >( next_occupant );

			FreeComponents();
		}

		template< typename Component >
		Component* GetComponent( u8 index ) const
		{
			return HasComponent( index ) ? Components< Component >() + index : nullptr;
		}

		template< typename Component >
		Component& AddComponent( u8 index, Component&& component )
		{
			Component* addr = Components< Component >() + index;

			if ( m_occupancy & (1 << index) )
				return *addr = component;

			m_occupancy |= (1 << index);
			m_dirty |= (1 << index);
			return *new(addr) Component( std::move( component ) );
		}

		template< typename Component >
		bool RemoveComponent( u8 index )
		{
			if ( !HasComponent( index ) )
				return false;

			Components< Component >()[ index ].~Component();
			m_occupancy &= ~(1 << index);
			m_dirty |= (1 << index);

			return true;
		}

		inline EntityID GetEntityID( u8 index ) const
		{
			return ( index < 16 ) ? EntityID( m_pageId + index ) : NoEntity;
		}

		inline bool HasComponent( u8 index ) const
		{
#			if _DEBUG
			STRONG_ASSERT( ( index & c_pageIndexMask ) == index, "Invalid index into a component page: {}", index );
#			endif

			return m_occupancy & ( 1u << index );
		}

#		if _WIN32 // thanks MSVC, very cool!
		__declspec(noinline)
#		endif
		u8 GetNextOccupantIndex( u8 after_index = ~0 ) const
		{
			return std::countr_zero( u16( m_occupancy & ~( ( 1u << ( after_index + 1 ) ) - 1 ) ) );
		}

#		if _WIN32 // thanks MSVC, very cool!
		__declspec(noinline)
#		endif
		u8 GetNextDirtyIndex( u8 after_index = 0 ) const
		{
			return std::countr_zero( u16( m_dirty & ~( ( 1u << ( after_index + 1 ) ) - 1 ) ) );
		}

		inline bool IsDirty( u8 index ) const
		{
#			if _DEBUG
			STRONG_ASSERT( ( index & c_pageIndexMask ) == index, "Invalid index into a component page: {}", index );
#			endif

			return m_dirty & ( 1u << index );
		}

		inline void RemoveDirtyFlag( u8 index )
		{
#			if _DEBUG
			STRONG_ASSERT( ( index & c_pageIndexMask ) == index, "Invalid index into a component page: {}", index );
#			endif

			m_dirty &= ~( 1u << index );
		}

		static inline int PageIDComparator( const Page& page, u32 page_id )
		{ return page.m_pageId < page_id; }

	private:
		template< typename Component >
		Component* Components() const { return reinterpret_cast<Component*>( m_components ); }
	};

	struct Iterator
	{
		GenericComponentTable& m_table;
		Page* m_page = nullptr;
		u8 m_index = 0;

		Iterator( GenericComponentTable& table, bool skip_dirty = true )
			: m_table( table )
			, m_page( table.Begin() )
		{
			if ( m_page == m_table.End() )
				return;

			if ( skip_dirty )
			{
				while ( m_page->m_occupancy == 0 && ++m_page != m_table.End() );

				if ( m_page != m_table.End() )
					m_index = m_page->GetNextOccupantIndex();
			}
			else
			{
				m_index = std::min( m_page->GetNextDirtyIndex(), m_page->GetNextOccupantIndex() );
			}
		}

		template< typename Component >
		typename ComponentTable< Component >::Iterator& Cast()
		{
#			if _DEBUG
			STRONG_ASSERT( m_table.IsOfType< Component >(),
				"Trying to use GenericComponentTable with a type other than the one it was created for" );
#			endif

			return *reinterpret_cast< ComponentTable< Component >::Iterator* >( this );
		}

		template< typename Component >
		const typename ComponentTable< Component >::Iterator& Cast() const
		{
#			if _DEBUG
			STRONG_ASSERT( m_table.IsOfType< Component >(),
				"Trying to use GenericComponentTable with a type other than the one it was created for" );
#			endif

			return *reinterpret_cast< const ComponentTable< Component >::Iterator* >( this );
		}

		inline operator bool() const { return m_page != m_table.End(); }
		Iterator& operator ++();

		inline EntityID GetEntityID() const
		{
			return m_page == m_table.End() ? NoEntity
				: m_page->GetEntityID( m_index );
		}

		template< typename Component >
		Component* GetComponent() const
		{
#			if _DEBUG
			STRONG_ASSERT( m_table.IsOfType< Component >(),
				"Trying to use GenericComponentTable with a type other than the one it was created for" );
#			endif

			if ( m_page == m_table.End() )
				return nullptr;

			return m_page->GetComponent< Component >( m_index );
		}

		inline bool IsDirty() const { return m_index < 16 && m_page != m_table.End() && m_page->IsDirty( m_index ); }
		inline void RemoveDirtyFlag() { if ( m_index < 16 && m_page != m_table.End() ) m_page->RemoveDirtyFlag( m_index ); }

		EntityID FindNextDirtyEntityID() const;
		EntityID GetNextEntityID() const;
		void GoToNext();

		// walk _forward_ to the specified entity ID
		// if not present, walk to the next entity ID that is occupied
		void GoTo( EntityID entity );

		inline void CopyToWorld( World& world, EntityID entity ) const
		{
			if ( m_page != m_table.End() )
				m_table.CopyComponentToWorld( world, *m_page, m_index, entity );
		}
	};

	struct MetaData
	{
		size_t componentType;
		void( *DestructorCallback )( GenericComponentTable& self );
		void( *CopyComponentToWorld )( World& world, Page& page, u8 index, EntityID dst_id );
		void( *RemoveComponent )( GenericComponentTable& self, EntityID id );

	private:
		template< typename Component >
		static void __DestructorCallback( GenericComponentTable& self )
		{
			for ( auto& page : self.m_pages )
				page.Clear< Component >();
		}

		template< typename Component >
		static void __CopyComponentToWorld( World& world, Page& page, u8 index, EntityID dst_id );

		template< typename Component >
		static void __RemoveComponent( GenericComponentTable& self, EntityID id )
		{
			self.RemoveComponent< Component >( id );
		}

		constexpr MetaData(
			decltype( componentType ) componentType,
			decltype( DestructorCallback ) DestructorCallback,
			decltype( CopyComponentToWorld ) CopyComponentToWorld,
			decltype( RemoveComponent ) RemoveComponent
		) : componentType( componentType )
		  , DestructorCallback( DestructorCallback )
		  , CopyComponentToWorld( CopyComponentToWorld )
		  , RemoveComponent( RemoveComponent )
		{}

	public:
		template< typename Component >
		inline static const MetaData s_singleton {
			typeid( Component ).hash_code(),
			__DestructorCallback< Component >,
			__CopyComponentToWorld< Component >,
			__RemoveComponent< Component >,
		};
	};

	Iterator Iter() { return Iterator( *this ); }

	void CleanUpPages()
	{
		std::erase_if( m_pages, []( Page& page )
			{
				page.m_dirty = 0;
				if ( page.m_occupancy != 0 ) return false;
				page.FreeComponents();
				return true;
			} );
	}

	template< typename Component >
	Component* GetComponent( EntityID entity )
	{
#		if _DEBUG
		STRONG_ASSERT( IsOfType< Component >(),
			"Trying to use GenericComponentTable with a type other than the one it was created for" );
#		endif

		ZoneScoped;

		const u32 page_id = (u32)entity & Page::c_pageIdMask;
		const u8 index = (u32)entity & Page::c_pageIndexMask;

		auto page = std::lower_bound( m_pages.begin(), m_pages.end(), page_id, Page::PageIDComparator );
		if ( page == m_pages.end() )
			return nullptr;

		return page->m_pageId != page_id ? nullptr : page->GetComponent< Component >( index );
	}

	template< typename Component >
	Component& AddComponent( EntityID entity, Component&& component )
	{
#		if _DEBUG
		STRONG_ASSERT( IsOfType< Component >(),
			"Trying to use GenericComponentTable with a type other than the one it was created for" );
#		endif

		ZoneScoped;

		const u32 page_id = (u32)entity & Page::c_pageIdMask;
		const u8 index = (u32)entity & Page::c_pageIndexMask;

		auto page = std::lower_bound( m_pages.begin(), m_pages.end(), page_id, Page::PageIDComparator );
		if ( page == m_pages.end() || page->m_pageId != page_id )
			page = m_pages.emplace( page, sizeof( Component ), page_id );

		m_hasChanged |= !page->HasComponent( index );
		return page->AddComponent< Component >( index, std::move( component ) );
	}

	template< typename Component >
	void RemoveComponent( EntityID entity )
	{
#		if _DEBUG
		STRONG_ASSERT( IsOfType< Component >(),
			"Trying to use GenericComponentTable with a type other than the one it was created for" );
#		endif

		ZoneScoped;

		const u32 page_id = (u32)entity & Page::c_pageIdMask;
		const u8 index = (u32)entity & Page::c_pageIndexMask;

		auto page = std::lower_bound( m_pages.begin(), m_pages.end(), page_id, Page::PageIDComparator );
		if ( page == m_pages.end() || page->m_pageId != page_id )
			return;

		m_hasChanged |= page->RemoveComponent< Component >( index );
	}

	template< typename Component >
	ComponentTable< Component >& Cast()
	{
#		if _DEBUG
		STRONG_ASSERT( IsOfType< Component >(),
			"Trying to use GenericComponentTable with a type other than the one it was created for" );
#		endif

		return *reinterpret_cast< ComponentTable< Component >* >( this );
	}

	template< typename Component >
	const ComponentTable< Component >& Cast() const
	{
#		if _DEBUG
		STRONG_ASSERT( IsOfType< Component >(),
			"Trying to use GenericComponentTable with a type other than the one it was created for" );
#		endif

		return *reinterpret_cast< const ComponentTable< Component >* >( this );
	}

	void RemoveComponent( EntityID entity )
	{
		m_metaData.RemoveComponent( *this, entity );
	}

	GenericComponentTable( const MetaData& meta_data )
		: m_metaData( meta_data )
	{}

	~GenericComponentTable()
	{
		m_metaData.DestructorCallback( *this );
	}

	GenericComponentTable( GenericComponentTable&& other ) = delete;
	GenericComponentTable( const GenericComponentTable& other ) = delete;

	GenericComponentTable& operator =( GenericComponentTable&& other ) = delete;
	GenericComponentTable& operator =( const GenericComponentTable& other ) = delete;

	void CopyComponentToWorld( World& world, Page& page, u8 index, EntityID entity_id ) const
	{
		m_metaData.CopyComponentToWorld( world, page, index, entity_id );
	}

private:
	const MetaData& m_metaData;
	std::vector< Page > m_pages;
	bool m_hasChanged = false;

public:
	Page* Begin() { return m_pages.empty() ? nullptr : &m_pages.front(); }
	Page* End() { return m_pages.empty() ? nullptr : (&m_pages.back() + 1); }

	bool HasChanged() const { return m_hasChanged; }
	void ResetHasChanged() { m_hasChanged = false; }
};

template< typename Component >
struct ComponentTable : GenericComponentTable
{
	struct Page : GenericComponentTable::Page
	{
		void Clear() { GenericComponentTable::Page::Clear< Component >(); }
		Component* GetComponent( u8 index ) const { return GenericComponentTable::Page::GetComponent< Component >( index ); }
		Component& AddComponent( u8 index, Component&& component ) { return GenericComponentTable::Page::AddComponent< Component >( index, std::move( component ) ); }
		bool RemoveComponent( u8 index ) { return GenericComponentTable::Page::RemoveComponent< Component >( index ); }
	};

	struct Iterator : GenericComponentTable::Iterator
	{
		Component* GetComponent() const { return GenericComponentTable::Iterator::GetComponent< Component >(); }
	};

	Iterator Iter() { return Iterator( *this ); }

	Component* GetComponent( EntityID entity ) { return GenericComponentTable::GetComponent< Component >( entity ); }
	Component& AddComponent( EntityID entity, Component&& component ) { return GenericComponentTable::AddComponent< Component >( entity, std::move( component ) ); }
	void RemoveComponent( EntityID entity ) { GenericComponentTable::RemoveComponent< Component >( entity ); }
};

}
