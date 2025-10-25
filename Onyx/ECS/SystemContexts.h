#pragma once

#include "Onyx/ECS/CommandBuffer.h"

namespace onyx
{

struct Tick
{
	f32 deltaTime = 0.f;
	f32 time = 0.f;
	u32 frame = 0;
};

}
