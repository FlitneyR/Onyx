#pragma once
#include "ChronoCommon/Components/Camera.h"
#include "Common/ECS/Query.h"
#include "Common/ECS/Components/Transform.h"

#include "Common/ECS/SystemContexts.h"

#include "Common/Graphics/Camera.h"

namespace chrono
{

using UpdateCamera_CameraFocusQuery = onyx::ecs::Query<
	onyx::ecs::Read< onyx::Transform2D >,
	onyx::ecs::Read< CameraFocus >
>;

using UpdateCamera_CameraQuery = onyx::ecs::Query<
	onyx::ecs::Write< onyx::Transform2D >,
	onyx::ecs::Write< Camera >
>;

void UpdateCamera( onyx::ecs::Context< const onyx::Tick, onyx::Camera2D > ctx, const UpdateCamera_CameraFocusQuery& focii, const UpdateCamera_CameraQuery& cameras );

}
