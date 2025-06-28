#pragma once

namespace chrono
{

struct Spawner
{
	std::string prefabScriptPath = "";
	f32 timeToNextSpawn = -1.f;
	f32 timeBetweenSpawns = 1.f;
	f32 spawnTimeVariation = 0.1f;
};

}
