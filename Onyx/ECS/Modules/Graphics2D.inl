#pragma once
#include "Graphics2D.h"
#include "Core.h"

namespace onyx::Graphics2D
{

template< typename SystemSet >
void RegisterGameplaySystems( SystemSet& system_set )
{
	system_set.AddSystem( UpdateAnimatedSprites::System );
	system_set.AddSystem( UpdateParallaxBackgroundLayers::System );

	system_set.AddDependency( UpdateAnimatedSprites::System, UpdateParallaxBackgroundLayers::System );
}

template< typename SystemSet >
void RegisterEditorSystems( SystemSet& system_set )
{
	system_set.AddSystem( UpdateAnimatedSprites::System );
	system_set.AddSystem( UpdateParallaxBackgroundLayers::System );

	system_set.AddDependency( UpdateAnimatedSprites::System, UpdateParallaxBackgroundLayers::System );
}

template< typename SystemSet >
void RegisterGraphicsSystems( SystemSet& system_set )
{
	system_set.AddSystem( CollectSprites::System );
}

}

