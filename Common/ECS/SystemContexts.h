#pragma once

#include "Common/ECS/CommandBuffer.h"

namespace onyx
{

struct Tick
{
	f32 deltaTime;
	f32 time;
	u32 frame;
};

}
