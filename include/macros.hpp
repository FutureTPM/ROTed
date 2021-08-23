#ifndef __MACROS_HPP__
#define __MACROS_HPP__
#include <openssl/aes.h>

#define CEILING(x,y) (((x) + (y) - 1) / (y))
#define MIN(x,y) ((x) < (y) ? (x) : (y))
#define AES_OUTPUT_LENGTH(x) (CEILING(x, AES_BLOCK_SIZE) * AES_BLOCK_SIZE)

#endif
