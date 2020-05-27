#ifndef CRYPTO_STREAM_H
#define CRYPTO_STREAM_H

#include <string>

extern "C" {

#if defined __x86_64__ || defined __i386__
int nfl_crypto_stream_salsa20(unsigned char *c, unsigned long long clen,
                                     const unsigned char *n,
                                     const unsigned char *k);

#else

#ifdef NFL_OPTIMIZED

#if defined __ARM_NEON__ && defined NTT_NEON && defined __arm__
int nfl_crypto_stream_salsa20(unsigned char *c, unsigned long long clen,
                                     const unsigned char *n,
                                     const unsigned char *k);
#else
#error This architecture does not support NEON
#endif

#else
#error We were expecting a NEON implementation of the Salsa stream cipher
#endif

#endif
}

#endif
