#pragma once
#include "Core.h"

namespace onyx::Core
{

template< typename SystemSet >
void Register2DGameplaySystems( SystemSet& system_set )
{
	system_set.AddSystem( UpdateTransform2DLocales::System );
}

template< typename SystemSet >
void Register2DEditorSystems( SystemSet& system_set )
{
	system_set.AddSystem( UpdateTransform2DLocales::System );
}

}
