#pragma once

#include "Entity.h"

#include "tracy/Tracy.hpp"

#include <vector>

namespace onyx::ecs
{

// forward declaration from World.h
struct World;

struct IComponentTable
{
	virtual ~IComponentTable() = default;

	virtual void RemoveComponent( EntityID entity ) = 0;

	struct IPage
	{
		IPage( size_t component_size, u32 page_id )
			: m_components( malloc( component_size * 16 ) )
			, m_pageId( page_id )
		{}

		~IPage() = default;
		IPage() = default;
		IPage( IPage&& other ) noexcept : IPage() { *this = std::move( other ); }

		IPage& operator =( IPage&& other ) noexcept
		{
			std::swap( m_components, other.m_components );
			std::swap( m_pageId, other.m_pageId );
			std::swap( m_occupancy, other.m_occupancy );
			std::swap( m_dirty, other.m_dirty );

			return *this;
		}

		IPage( const IPage& other ) = delete;
		IPage& operator =( const IPage& other ) = delete;

		constexpr static u32 c_pageIndexMask = 0xf;
		constexpr static u32 c_pageIdMask = ~c_pageIndexMask;

		void* m_components = nullptr;

		// the top 28 bits of the page id match the top 28 bits of the entity ids of all of its components
		// the bottom 4 bits are 0
		u32 m_pageId = 0;
		u16 m_occupancy = 0;
		u16 m_dirty = 0;

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
			return std::countr_zero( m_occupancy & ~( ( 1u << ( after_index + 1 ) ) - 1 ) );
		}

		inline u8 GetLastOccupant( u8 before_index = 16 ) const
		{
			return 15 - std::countl_zero( u16( m_occupancy & ( ( 1u << before_index ) - 1 ) ) );
		}

		inline u8 GetNextDirty( u8 after_index = 0 ) const
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

		static inline int PageIDComparator( const IPage& page, u32 page_id )
		{ return page.m_pageId < page_id; }
	};

	std::vector< IPage > m_pages;
	IPage* Begin() { return m_pages.empty() ? nullptr : &m_pages.front(); }
	IPage* End() { return m_pages.empty() ? nullptr : ( &m_pages.back() + 1 ); }

	virtual void CopyComponentToWorld( World& world, IPage& page, u8 index, EntityID entity_id ) const = 0;

	struct IIterator
	{
		IComponentTable& m_table;
		IPage* m_page = nullptr;
		u8 m_index = 0;

		IIterator( IComponentTable& table )
			: m_table( table )
			, m_page( table.Begin() )
		{
			if ( m_page != table.End() )
				m_index = m_page->GetNextOccupantIndex();
		}

		inline operator bool() const { return m_page != m_table.End(); }
		IIterator& operator ++();

		inline EntityID GetEntityID() const
		{
			return m_page == m_table.End() ? NoEntity
				: m_page->GetEntityID( m_index );
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

	IIterator Iter() { return IIterator( *this ); }

	bool m_hasChanged = false;
};

template< typename Component >
struct ComponentTable : IComponentTable
{
	struct Page final : IPage
	{
		void Clear()
		{
			if ( !m_components )
				return;

			u8 next_occupant;
			while ( ( next_occupant = GetNextOccupantIndex() ) < 16 )
				RemoveComponent( next_occupant );

			free( m_components );
			m_components = nullptr;
		}

		Component* GetComponent( u8 index ) const
		{
#			if _DEBUG
			STRONG_ASSERT( ( index & c_pageIndexMask ) == index, "Invalid index into a component page: {}", index );
#			endif

			return HasComponent( index ) ? Components() + index : nullptr;
		}

		Component& AddComponent( u8 index, Component&& component )
		{
#			if _DEBUG
			STRONG_ASSERT( ( index & c_pageIndexMask ) == index, "Invalid index into a component page: {}", index );
#			endif

			Component* addr = Components() + index;

			if ( m_occupancy & ( 1 << index ) )
				return *addr = component;

			m_occupancy |= ( 1 << index );
			m_dirty |= ( 1 << index );
			return *new( addr ) Component( std::move( component ) );
		}

		void RemoveComponent( u8 index )
		{
#			if _DEBUG
			STRONG_ASSERT( ( index & c_pageIndexMask ) == index, "Invalid index into a component page: {}", index );
#			endif

			if ( !HasComponent( index ) )
				return;

			Components()[ index ].~Component();
			m_occupancy &= ~( 1 << index );
			m_dirty |= ( 1 << index );
		}

	private:
		Component* Components() const { return reinterpret_cast< Component* >( m_components ); }
	};

	~ComponentTable()
	{
		for ( auto& page : m_pages )
			reinterpret_cast< Page& >( page ).Clear();
	}

	struct Iterator final : IIterator
	{
		Iterator( ComponentTable& table ) : IIterator( table ) {}

		Component* GetComponent() const
		{
			if ( m_page == m_table.End() )
				return nullptr;

			return reinterpret_cast< Page* >( m_page )->GetComponent( m_index );
		}
	};
	
	void CopyComponentToWorld( World& world, IPage& page, u8 index, EntityID entity_id ) const override;

	Iterator Iter() { return Iterator( *this ); }

	Component* GetComponent( EntityID entity )
	{
		ZoneScoped;

		const u32 page_id = (u32)entity & IPage::c_pageIdMask;
		const u8 index = (u32)entity & IPage::c_pageIndexMask;

		auto iter = std::lower_bound( m_pages.begin(), m_pages.end(), page_id, IPage::PageIDComparator );
		if ( iter == m_pages.end() )
			return nullptr;

		Page& page = *reinterpret_cast< Page* >( &*iter );
		return page.m_pageId != page_id ? nullptr : page.GetComponent( index );
	}

	Component& AddComponent( EntityID entity, Component&& component )
	{
		ZoneScoped;

		const u32 page_id = (u32)entity & IPage::c_pageIdMask;
		const u8 index = (u32)entity & IPage::c_pageIndexMask;

		auto iter = std::lower_bound( m_pages.begin(), m_pages.end(), page_id, IPage::PageIDComparator );
		if ( iter == m_pages.end() || iter->m_pageId != page_id )
			iter = m_pages.emplace( iter, sizeof( Component ), page_id );

		Page& page = *reinterpret_cast< Page* >( &*iter );
		m_hasChanged = true;
		return page.AddComponent( index, std::move( component ) );
	}

	void RemoveComponent( EntityID entity )
	{
		ZoneScoped;

		const u32 page_id = (u32)entity & IPage::c_pageIdMask;
		const u8 index = (u32)entity & IPage::c_pageIndexMask;

		auto iter = std::lower_bound( m_pages.begin(), m_pages.end(), page_id, IPage::PageIDComparator );
		if ( iter == m_pages.end() || iter->m_pageId != page_id )
			return;

		Page& page = *reinterpret_cast< Page* >( &*iter );
		m_hasChanged = true;
		page.RemoveComponent( index );

		if ( page.m_occupancy == 0 )
			m_pages.erase( iter );
	}
};

}
