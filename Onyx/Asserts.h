#pragma once

#include "log.h"

#define ASSERT_INTERNAL( failure, fail_expr, condition, ... ) \
	[ &, function_name = __FUNCTION__ ]() \
	{ \
		auto __result__ = condition; \
		static bool __has_failed_once__ = false; \
		if ( failure ) { \
			__has_failed_once__ = true; \
			std::string s = ""; \
			__VA_OPT__(s = std::format( __VA_ARGS__ ); )\
			LOG_INTERNAL( function_name, "E", "ASSERT FAILED: {}\n\tCondition: {}", s, #condition ); \
			fail_expr \
		} \
		return __result__; \
	} ()

#define STRONG_ASSERT( condition, ... ) ASSERT_INTERNAL( !__result__, __debugbreak(); exit(-1);, condition __VA_OPT__( , __VA_ARGS__ ) )
#define WEAK_ASSERT( condition, ... ) ASSERT_INTERNAL( !__result__, __debugbreak();, condition __VA_OPT__( , __VA_ARGS__ ) )
#define WEAK_ASSERT_ONCE( condition, ... ) ASSERT_INTERNAL( !__result__ && !__has_failed_once__, __debugbreak();, condition __VA_OPT__( , __VA_ARGS__ ) )
#define LOG_ASSERT( condition, ... ) ASSERT_INTERNAL( !__result__, , condition __VA_OPT__( , __VA_ARGS__ ) )
#define LOG_ASSERT_ONCE( condition, ... ) ASSERT_INTERNAL( !__result__ && !__has_failed_once__, , condition __VA_OPT__( , __VA_ARGS__ ) )

#ifndef ASSERT
#define ASSERT(...) STRONG_ASSERT( __VA_ARGS__ )
#endif
