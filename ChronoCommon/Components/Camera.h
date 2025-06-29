#pragma once

namespace chrono
{

struct Camera
{
	f32 minFov = 1'000.f;
	f32 maxFov = 2'000.f;
	f32 margin = 3.f;
	f32 moveSpeed = 0.5f;
	f32 zoomSpeed = 0.5f;
	f32 fov = 0.5f * ( minFov + maxFov );
};

struct CameraFocus
{};

}
