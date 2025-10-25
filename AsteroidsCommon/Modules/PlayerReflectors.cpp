#include "Player.h"

#include "Common/ECS/Scene.h"

using PlayerController = asteroids::Player::PlayerController;
using Projectile = asteroids::Player::Projectile;

COMPONENT_REFLECTOR( PlayerController )
{
	COMPONENT_REFLECTOR_HEADER( PlayerController );
	#define xproperties( f )\
		f( PlayerController, f32, boostSpeed, "Boost Speed" )\
		f( PlayerController, f32, turnSpeed, "Turn Speed" )\
		f( PlayerController, f32, timeBetweenShots, "Time Between Shots" )\
		f( PlayerController, std::shared_ptr< onyx::ecs::Scene >, bulletPrefab, "Bullet Prefab" )\
		f( PlayerController, std::shared_ptr< onyx::TextureAnimationAsset >, idleEngineAnimation, "Idle Engine Animation" )\
		f( PlayerController, std::shared_ptr< onyx::TextureAnimationAsset >, boostEngineAnimation, "Boost Engine Animation" )\
		f( PlayerController, onyx::ecs::EntityID, engineEffectSprite, "Engine Effect Entity" )\

	DEFAULT_REFLECTOR( PlayerController, xproperties );
	#undef xproperties

	POST_COPY_TO_WORLD()
	{
		BEGIN_POST_COPY_TO_WORLD( PlayerController, pc );
		UpdateEntityID( pc.engineEffectSprite, entity_id_map );
	}
};

COMPONENT_REFLECTOR( Projectile )
{
	COMPONENT_REFLECTOR_HEADER( Projectile );
	#define xproperties( f )\
		f( Projectile, f32, initialSpeed, "Initial Speed" )\

	DEFAULT_REFLECTOR( Projectile, xproperties );
	#undef xproperties
};

DEFINE_COMPONENT_REFLECTOR( PlayerController );
DEFINE_COMPONENT_REFLECTOR( Projectile );

void asteroids::Player::RegisterReflectors( onyx::ecs::ComponentReflectorTable& table )
{
	REGISTER_COMPONENT_REFLECTOR( table, PlayerController );
	REGISTER_COMPONENT_REFLECTOR( table, Projectile );
}
