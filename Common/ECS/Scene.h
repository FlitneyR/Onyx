#pragma once

#include "Common/Assets.h"
#include "Common/ECS/World.h"
#include "Common/BjSON/BjSON.h"

namespace onyx::ecs
{

template< typename T >
void SerialiseComponent( BjSON::IReadWriteObject& writer, const T& component );

template< typename T >
T DeserialiseComponent( const BjSON::IReadOnlyObject& reader );

struct Scene : IAsset
{
	ecs::World m_tempWorld;
};

struct SceneEditor : editor::IWindow
{
};

}
