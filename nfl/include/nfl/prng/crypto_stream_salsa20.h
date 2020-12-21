#ifndef CRYPTO_STREAM_H
#define CRYPTO_STREAM_H

#include <string>

extern "C" {

#if defined __x86_64__ || defined __i386__
int nfl_crypto_stream_salsa20(unsigned char *c, unsigned long long clen,
                                     const unsigned char *n,
                                     const unsigned char *k);

#else

#if defined __ARM_NEON__ && defined __arm__
int nfl_crypto_stream_salsa20(unsigned char *c, unsigned long long clen,
                                     const unsigned char *n,
                                     const unsigned char *k);
#else
#error This architecture does not support NEON
#endif

#endif
}

#endif
