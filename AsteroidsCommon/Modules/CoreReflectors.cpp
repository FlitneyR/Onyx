#include "Core.h"

#include "Common/ECS/Scene.h"
#include "Common/Utils.h"

using namespace asteroids::Core;

COMPONENT_REFLECTOR( Camera )
{
	COMPONENT_REFLECTOR_HEADER( Camera );

	#define xproperties( f )\
		f( Camera, f32, minFov, "Min Fov" )\
		f( Camera, f32, maxFov, "Max Fov" )\
		f( Camera, f32, margin, "Margin" )\
		f( Camera, f32, moveSpeed, "Move Speed" )\
		f( Camera, f32, zoomSpeed, "Zoom Speed" )\
		f( Camera, f32, fov, "Fov" )\

	DEFAULT_REFLECTOR( Camera, xproperties );
	#undef xproperties
};

COMPONENT_REFLECTOR( CameraFocus )
{
	COMPONENT_REFLECTOR_HEADER( CameraFocus );

	#define xproperties( f )

	DEFAULT_REFLECTOR( CameraFocus, xproperties );
	#undef xproperties
};

COMPONENT_REFLECTOR( Health )
{
	COMPONENT_REFLECTOR_HEADER( Health );
	
	#define xproperties( f )\
		f( Health, f32, amount, "Amount" )\

	DEFAULT_REFLECTOR( Health, xproperties );
	#undef xproperties
};

COMPONENT_REFLECTOR( Lifetime )
{
	COMPONENT_REFLECTOR_HEADER( Lifetime );

	#define xproperties( f )\
		f( Lifetime, f32, duration, "Duration")\
		f( Lifetime, bool, active, "Active")\

	DEFAULT_SERIALISE_COMPONENT( Lifetime, xproperties );
	DEFAULT_SERIALISE_COMPONENT_EDITS( Lifetime, xproperties );
	DEFAULT_COMPONENT_EDITOR_UI( Lifetime, xproperties );

	DESERIALISE_COMPONENT()
	{
		BEGIN_DESERIALISE_COMPONENT( Lifetime, component );
		xproperties( DEFAULT_DESERIALISE_PROPERTY );

		component.remaining = component.duration;
	}
	#undef xproperties
};

COMPONENT_REFLECTOR( OnDeath )
{
	COMPONENT_REFLECTOR_HEADER( OnDeath );

	#define xproperties( f )\
		f( OnDeath, std::shared_ptr< onyx::ecs::Scene >, spawnScene, "Spawn Scene" )\

	DEFAULT_REFLECTOR( OnDeath, xproperties );
	#undef xproperties
};

COMPONENT_REFLECTOR( OffScreenSpawner )
{
	COMPONENT_REFLECTOR_HEADER( OffScreenSpawner );

	#define xproperties( f )\
		f( OffScreenSpawner, std::shared_ptr< onyx::ecs::Scene >, prefab, "Prefab")\
		f( OffScreenSpawner, glm::vec2, linearVelocityRange, "Linear Velocity Range")\
		f( OffScreenSpawner, glm::vec2, angularVelocityRange, "Angular Velocity Range")\
		f( OffScreenSpawner, glm::vec2, spawnPeriodRange, "Spawn Period Range")\

	DEFAULT_REFLECTOR( OffScreenSpawner, xproperties )
	#undef xproperties
};

inline static const char* const s_TeamNames[] = { "None", "Player", "Enemy" };

DEFINE_DEFAULT_SERIALISE_PROPERTY( Team::Enum ) { writer.SetLiteral( name, std::string( s_TeamNames[ (u8)value ] ) ); }
DEFINE_DEFAULT_PROPERTY_EDITOR_UI( Team::Enum ) { return ImGui::Combo( name, (int*)&value, s_TeamNames, (int)Team::Count ); }
DEFINE_DEFAULT_PROPERTY_DIFF_HINT( Team::Enum ) { ImGui::SetTooltip( "%s", s_TeamNames[ value ] ); }
DEFINE_DEFAULT_DESERIALISE_PROPERTY( Team::Enum )
{
	const std::string team_name = reader.GetLiteral< std::string >( name );
	value = DecodeEnum< Team::Enum >( s_TeamNames, team_name.c_str(), Team::None );
}

COMPONENT_REFLECTOR( Team )
{
	COMPONENT_REFLECTOR_HEADER( Team );

	#define xproperties( f )\
		f( Team, Team::Enum, team, "Team" )\

	DEFAULT_REFLECTOR( Team, xproperties );
	#undef xproperties
};

DEFINE_COMPONENT_REFLECTOR( Camera );
DEFINE_COMPONENT_REFLECTOR( CameraFocus );
DEFINE_COMPONENT_REFLECTOR( Health );
DEFINE_COMPONENT_REFLECTOR( Lifetime );
DEFINE_COMPONENT_REFLECTOR( OnDeath );
DEFINE_COMPONENT_REFLECTOR( Team );
DEFINE_COMPONENT_REFLECTOR( OffScreenSpawner );

void asteroids::Core::RegisterReflectors( onyx::ecs::ComponentReflectorTable& table )
{
	REGISTER_COMPONENT_REFLECTOR( table, Camera );
	REGISTER_COMPONENT_REFLECTOR( table, CameraFocus );
	REGISTER_COMPONENT_REFLECTOR( table, Health );
	REGISTER_COMPONENT_REFLECTOR( table, Lifetime );
	REGISTER_COMPONENT_REFLECTOR( table, OnDeath );
	REGISTER_COMPONENT_REFLECTOR( table, Team );
	REGISTER_COMPONENT_REFLECTOR( table, OffScreenSpawner );
}
