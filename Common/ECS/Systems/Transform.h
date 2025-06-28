#pragma once

#include "Common/ECS/Components/Transform.h"
#include "Common/ECS/Query.h"
#include "Common/ECS/SystemContexts.h"

namespace onyx
{

using UpdateTransform2DLocales_Transforms = onyx::ecs::Query<
	onyx::ecs::Read< onyx::Transform2D >
>;

using UpdateTransform2DLocales_AttachedTransforms = onyx::ecs::Query<
	onyx::ecs::Write< onyx::Transform2D >,
	onyx::ecs::Read< onyx::AttachedTo >
>;

void UpdateTransform2DLocales( onyx::ecs::Context<> ctx, const UpdateTransform2DLocales_Transforms& parents, const UpdateTransform2DLocales_AttachedTransforms& children );

}
