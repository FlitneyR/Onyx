#pragma once
#include "glm/glm.hpp"

typedef float	f32;
typedef double	f64;

typedef char		i8;
typedef short		i16;
typedef int			i32;
typedef long long	i64;

typedef unsigned char		u8;
typedef unsigned short		u16;
typedef unsigned int		u32;
typedef unsigned long long	u64;

typedef unsigned char byte;

static_assert( sizeof( f32 ) == 32 / 8 );
static_assert( sizeof( f64 ) == 64 / 8 );

static_assert( sizeof( i8 ) == 8 / 8 );
static_assert( sizeof( i16 ) == 16 / 8 );
static_assert( sizeof( i32 ) == 32 / 8 );
static_assert( sizeof( i64 ) == 64 / 8 );

static_assert( sizeof( u8 ) == 8 / 8 );
static_assert( sizeof( u16 ) == 16 / 8 );
static_assert( sizeof( u32 ) == 32 / 8 );
static_assert( sizeof( u64 ) == 64 / 8 );
