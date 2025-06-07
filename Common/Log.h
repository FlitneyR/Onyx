#pragma once

#include <string>
#include <format>

void __LogInternal( const char* string );

#define LOG_INTERNAL( FUNCTION_NAME, severity, fmt, ... ) \
	__LogInternal( std::format( "[" severity "] [ {} : {} ] [ {} ]\n\t{}\n", \
		__FILE__, __LINE__, FUNCTION_NAME, \
		std::format( fmt __VA_OPT__(, __VA_ARGS__ ) ) ).c_str() )

#define LOG( severity, fmt, ... ) LOG_INTERNAL( __FUNCTION__, severity, fmt __VA_OPT__( , __VA_ARGS__ ) )
#define INFO( fmt, ... ) LOG_INTERNAL( __FUNCTION__, "I", fmt __VA_OPT__( , __VA_ARGS__ ) )
#define WARN( fmt, ... ) LOG_INTERNAL( __FUNCTION__, "W", fmt __VA_OPT__( , __VA_ARGS__ ) )
#define ERROR( fmt, ... ) LOG_INTERNAL( __FUNCTION__, "E", fmt __VA_OPT__( , __VA_ARGS__ ) )
