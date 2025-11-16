#include "Core.h"

#include "tracy/Tracy.hpp"

namespace onyx::Core
{

void UpdateTransform2DLocales::System( Context ctx, const Transforms& parents, const AttachedTransforms& children )
{
	ZoneScoped;

	for ( auto& child : children )
	{
		auto [id, child_transform, attachment] = child.Break();

		if ( auto* parent = parents.Get( attachment.localeEntity ) )
		{
			auto [id, parent_transform] = parent->Break();

			child_transform.SetLocale( parent_transform.GetMatrix() );
		}
	}
}

void PostCopyUpdateRootTransforms2D( const ecs::World& world, const ecs::IDMap& id_map, const glm::mat3& transform )
{
	ZoneScoped;

	for ( const auto& [_, id] : id_map )
	{
		if ( world.GetComponent< onyx::Core::AttachedTo >( id ) )
			continue;

		Transform2D* t = world.GetComponent< Transform2D >( id );
		if ( !t )
			continue;

		t->SetLocale( transform );
		const glm::vec2 position = t->GetWorldPosition();
		const glm::vec2 scale = t->GetWorldScale();
		const f32 rotation = t->GetWorldRotation();

		t->SetLocale( glm::mat3 { 1.f } );
		t->SetLocalPosition( position );
		t->SetLocalScale( scale );
		t->SetLocalRotation( rotation );
	}
}

}
