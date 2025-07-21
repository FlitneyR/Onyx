#include "Physics.h"

#include "Common/ECS/Scene.h"

using PhysicsBody = chrono::Physics::PhysicsBody;
using Collider = chrono::Physics::Collider;
using DamageOnCollision = chrono::Physics::DamageOnCollision;

COMPONENT_REFLECTOR( PhysicsBody )
{
	COMPONENT_REFLECTOR_HEADER( PhysicsBody );

	#define xproperties( f )\
		f( PhysicsBody, f32, linearFriction, "Linear Friction" )\
		f( PhysicsBody, f32, angularFriction, "Angular Friction" )\

	DEFAULT_REFLECTOR( PhysicsBody, xproperties );
	#undef xproperties
};

COMPONENT_REFLECTOR( Collider )
{
	COMPONENT_REFLECTOR_HEADER( Collider );

	#define xproperties( f )\
		f( Collider, f32, radius, "Radius" )\

	DEFAULT_REFLECTOR( Collider, xproperties );
	#undef xproperties
};

COMPONENT_REFLECTOR( DamageOnCollision )
{
	COMPONENT_REFLECTOR_HEADER( DamageOnCollision );

	#define xproperties( f )\
		f( DamageOnCollision, f32, selfDamage, "Self Damage" )\
		f( DamageOnCollision, f32, otherDamage, "Other Damage" )\

	DEFAULT_REFLECTOR( DamageOnCollision, xproperties );
	#undef xproperties
};

DEFINE_COMPONENT_REFLECTOR( PhysicsBody );
DEFINE_COMPONENT_REFLECTOR( Collider );
DEFINE_COMPONENT_REFLECTOR( DamageOnCollision );

void chrono::Physics::RegisterReflectors( onyx::ecs::ComponentReflectorTable& table )
{
	REGISTER_COMPONENT_REFLECTOR( table, PhysicsBody );
	REGISTER_COMPONENT_REFLECTOR( table, Collider );
	REGISTER_COMPONENT_REFLECTOR( table, DamageOnCollision );
}
