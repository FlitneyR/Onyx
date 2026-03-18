#include "Log.h"
#include <fstream>
#include <vector>
#include <ctime>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include "Windows.h"
#endif

static struct Log
{
	Log();
	~Log();

	std::ofstream file;
	std::vector< std::string > messages;

	void Add( const char* string );

} s_log;

void __LogInternal( const char* string )
{
	s_log.Add( string );
}

struct LogFileInitException : std::exception
{
	const char* what() const noexcept override
	{
		return "Failed to create log file";
	}
};

Log::Log()
{
	time_t now;
	tm _now;

	std::time( &now );

#ifdef WIN32
	gmtime_s( &_now, &now );
#else
	gmtime_r( &now, &_now );
#endif

	char buf[ sizeof "YYYY-MM-DDTHH-MM-SSZ" ];
	strftime( buf, sizeof buf, "%FT%TZ", &_now );

	for ( char& c : buf )
		if ( c == ':' )
			c = '-';

	file = std::ofstream( fmt::format( "./logs/{}.txt", buf ), std::ios::app );

	if ( !file.is_open() )
		throw LogFileInitException();
}

Log::~Log()
{
	file.close();
}

void Log::Add( const char* string )
{
	std::printf( "%s\n", string );

	#ifdef _WIN32
	OutputDebugStringA( string );
	#endif

	file << string << std::endl;
}
