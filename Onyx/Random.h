#pragma once
#include "External/SquirrelNoise5.hpp"

namespace onyx
{

struct RNG
{
	RNG( u32 seed ) : m_seed( seed ) {}

	template< const u32 N > u32 GetUint( const glm::vec< N, u32 >& pos ) const { return GetUintSeed< N >( pos, m_seed ); }
	template< const u32 N > f32 GetNorm( const glm::vec< N, u32 >& pos ) const { return GetNormSeed< N >( pos, m_seed ); }
	template< const u32 N > f32 Get01( const glm::vec< N, u32 >& pos ) const { return Get01Seed< N >( pos, m_seed ); }

	template< const u32 N > static u32 GetUintSeed( const glm::vec< N, u32 >& p, u32 seed );
	template< const u32 N > static f32 GetNormSeed( const glm::vec< N, u32 >& p, u32 seed );
	template< const u32 N > static f32 Get01Seed( const glm::vec< N, u32 >& p, u32 seed );

	template<> static u32 GetUintSeed< 1 >( const glm::uvec1& p, u32 seed )	{ return Get1dNoiseUint( p.x, seed ); }
	template<> static u32 GetUintSeed< 2 >( const glm::uvec2& p, u32 seed )	{ return Get2dNoiseUint( p.x, p.y, seed ); }
	template<> static u32 GetUintSeed< 3 >( const glm::uvec3& p, u32 seed )	{ return Get3dNoiseUint( p.x, p.y, p.z, seed ); }
	template<> static u32 GetUintSeed< 4 >( const glm::uvec4& p, u32 seed )	{ return Get4dNoiseUint( p.x, p.y, p.z, p.w, seed ); }

	template<> static f32 Get01Seed< 1 >( const glm::uvec1& p, u32 seed )	{ return Get1dNoiseZeroToOne( p.x, seed ); }
	template<> static f32 Get01Seed< 2 >( const glm::uvec2& p, u32 seed )	{ return Get2dNoiseZeroToOne( p.x, p.y, seed ); }
	template<> static f32 Get01Seed< 3 >( const glm::uvec3& p, u32 seed )	{ return Get3dNoiseZeroToOne( p.x, p.y, p.z, seed ); }
	template<> static f32 Get01Seed< 4 >( const glm::uvec4& p, u32 seed )	{ return Get4dNoiseZeroToOne( p.x, p.y, p.z, p.w, seed ); }

	template<> static f32 GetNormSeed< 1 >( const glm::uvec1& p, u32 seed )	{ return Get1dNoiseNegOneToOne( p.x, seed ); }
	template<> static f32 GetNormSeed< 2 >( const glm::uvec2& p, u32 seed )	{ return Get2dNoiseNegOneToOne( p.x, p.y, seed ); }
	template<> static f32 GetNormSeed< 3 >( const glm::uvec3& p, u32 seed )	{ return Get3dNoiseNegOneToOne( p.x, p.y, p.z, seed ); }
	template<> static f32 GetNormSeed< 4 >( const glm::uvec4& p, u32 seed )	{ return Get4dNoiseNegOneToOne( p.x, p.y, p.z, p.w, seed ); }

	u32 GetNextUint() { return GetUintSeed< 1 >( glm::uvec1( m_nextIndex++ ), m_seed ); }
	f32 GetNextNorm() { return GetNormSeed< 1 >( glm::uvec1( m_nextIndex++ ), m_seed ); }
	f32 GetNext01() { return Get01Seed< 1 >( glm::uvec1( m_nextIndex++ ), m_seed ); }

private:
	const u32 m_seed;
	u32 m_nextIndex = 0;
};

}
