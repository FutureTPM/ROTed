#ifndef NFL_ARCH_HPP
#define NFL_ARCH_HPP

#include "nfl/arch/common.hpp"

#ifdef NFL_OPTIMIZED
#pragma message ( "Compiling with NFL_OPTIMIZED" )

#if defined __AVX2__ && defined NTT_AVX2
#pragma message ( "Compiling with AVX2 directives" )
#define CC_SIMD nfl::simd::avx2
#elif defined __SSE4_2__ && defined NTT_SSE
#pragma message ( "Compiling with SSE4.2 directives" )
#define CC_SIMD nfl::simd::sse
#elif defined __ARM_NEON__ && defined NTT_NEON
#pragma message ( "Compiling with ARM NEON directives" )
#define CC_SIMD nfl::simd::neon
#endif

#endif

#ifndef CC_SIMD
#pragma message ( "Defaulting to Serial directives" )
#define CC_SIMD nfl::simd::serial
#endif

#endif
