#pragma once

#include <chrono>

namespace onyx
{

struct Clock
{
	f32 GetTime() const { return GetDuration( m_start, m_thisTick ); }
	f32 GetDeltaTime() const { return GetDuration( m_lastTick, m_thisTick ); }

	void Tick() { m_lastTick = m_thisTick; m_thisTick = _Clock::now(); }

private:
	using _Clock = std::chrono::high_resolution_clock;
	using TimePoint = std::chrono::time_point< _Clock >;

	TimePoint m_start = _Clock::now();
	TimePoint m_lastTick = m_start;
	TimePoint m_thisTick = m_start;

	static f32 GetDuration( const TimePoint& from, const TimePoint& to )
	{
		return f32( ( to - from ).count() ) / f32( _Clock::period::den );
	}
};

}
