#pragma once

#include <chrono>

namespace onyx
{

struct Clock
{
	u32 GetUnixTime() const { return std::chrono::duration_cast< std::chrono::seconds>( SClock::now().time_since_epoch() ).count(); }

	f32 GetTime() const { return GetDuration( m_start, m_thisTick ); }
	f32 GetDeltaTime() const { return GetDuration( m_lastTick, m_thisTick ); }
	void Tick() { m_lastTick = m_thisTick; m_thisTick = HRClock::now(); }

private:
	using SClock = std::chrono::system_clock;
	using HRClock = std::chrono::high_resolution_clock;
	using TimePoint = std::chrono::time_point< HRClock >;

	TimePoint m_start = HRClock::now();
	TimePoint m_lastTick = m_start;
	TimePoint m_thisTick = m_start;

	static f32 GetDuration( const TimePoint& from, const TimePoint& to )
	{
		return f32( ( to - from ).count() ) / f32( HRClock::period::den );
	}
};

}
