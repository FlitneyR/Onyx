#pragma once

template< typename T, const size_t count >
T DecodeEnum( const char* const ( &names )[ count ], const char* name, T default_val )
{
	for ( u32 val = 0; val < count; ++val )
		if ( !strcmp( names[ val ], name ) )
			return T( val );

	return default_val;
}

namespace onyx::ecs { struct EntityID; }

struct ImGuiScopedID
{
	ImGuiScopedID( const onyx::ecs::EntityID& id );
	ImGuiScopedID( int id );
	ImGuiScopedID( const char* id );
	~ImGuiScopedID();
};

struct ImGuiScopedIndent
{
	f32 m_amount;
	ImGuiScopedIndent( f32 amount );
	~ImGuiScopedIndent();
};
