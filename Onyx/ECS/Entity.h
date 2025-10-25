#pragma once

namespace onyx::ecs
{

struct EntityID
{
	constexpr EntityID( u32 id = 0 ) : id( id ) {}
	constexpr EntityID( const EntityID& other ) : id( other.id ) {}
	constexpr EntityID( EntityID&& other ) : id( other.id ) {}

	constexpr operator u32() const { return id; }
	constexpr operator bool() const { return id; }

	bool operator <( const EntityID& other ) const { return id < other.id; }
	bool operator >( const EntityID& other ) const { return id > other.id; }
	bool operator <=( const EntityID& other ) const { return id <= other.id; }
	bool operator >=( const EntityID& other ) const { return id >= other.id; }
	bool operator ==( const EntityID& other ) const { return id == other.id; }
	bool operator !=( const EntityID& other ) const { return id != other.id; }
	
	EntityID& operator =( u32 id ) { this->id = id; return *this; }
	EntityID& operator =( EntityID&& other ) noexcept { this->id = other.id; return *this; }
	EntityID& operator =( const EntityID& other ) { this->id = other.id; return *this; }

	EntityID& operator ++() { id++; return *this; }
	EntityID operator ++( int ) { EntityID copy = *this;  id++; return copy; }

private:
	u32 id = 0;
};

static constexpr EntityID NoEntity;

}

template< typename Char >
struct std::formatter< onyx::ecs::EntityID, Char > : std::formatter< u32, Char >
{
	template<class FmtContext>
	FmtContext::iterator format( onyx::ecs::EntityID entity, FmtContext& ctx ) const
	{
		return std::formatter< u32, Char >::format( u32( entity ), ctx );
	}
};

template<>
struct std::hash< onyx::ecs::EntityID > : std::hash< u32 >
{
	size_t operator ()( const onyx::ecs::EntityID& id ) const
	{
		return std::hash< u32 >::operator ()( id );
	}
};
