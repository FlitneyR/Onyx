#pragma once
#include "Core.h"

#include "Onyx/ECS/Modules/Graphics2D.h"

namespace asteroids::Core
{

template< typename SystemSet >
void RegisterGameplaySystems( SystemSet& system_set )
{
	system_set.AddSystem( UpdateCamera::System );
	system_set.AddSystem( UpdateLifetimes::System );
	system_set.AddSystem( UpdateOffScreenSpawners::System );
	system_set.AddSystem( UpdateHealthSprites::System );

	system_set.AddDependency( UpdateCamera::System, UpdateOffScreenSpawners::System );
	system_set.AddDependency( UpdateCamera::System, onyx::Graphics2D::UpdateParallaxBackgroundLayers::System );
	system_set.AddDependency( UpdateHealthSprites::System, onyx::Graphics2D::UpdateAnimatedSprites::System );
}

template< typename SystemSet >
void RegisterEditorSystems( SystemSet& system_set )
{
	system_set.AddSystem( UpdateHealthSprites::System );

	system_set.AddDependency( UpdateHealthSprites::System, onyx::Graphics2D::UpdateAnimatedSprites::System );
}

}
