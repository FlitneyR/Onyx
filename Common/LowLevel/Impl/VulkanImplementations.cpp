#define VMA_IMPLEMENTATION

#define VMA_ASSERT( condition ) WEAK_ASSERT( condition )

template< typename ... Ts >
std::string inline_sprintf( const char* format, Ts ... ts )
{
	std::string result;
	result.resize( std::snprintf( nullptr, 0, format, ts ... ) + 1 );
	std::snprintf( const_cast< char* >( result.c_str() ), result.size() - 1, format, ts ... );
	return result;
}

#define VMA_DEBUG_LOG_FORMAT( ... ) INFO( "{}", inline_sprintf( __VA_ARGS__ ) )

#include "vk_mem_alloc.h"