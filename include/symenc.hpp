#ifndef __SYMENC_HPP__
#define __SYMENC_HPP__
#include "aes_cbc.h"
#include "macros.hpp"

template<size_t pbytes, size_t rbytes, size_t bbytes>
struct sym_enc_t
{
  static constexpr size_t ivbytes = 16;
  static constexpr size_t outbytes = AES_OUTPUT_LENGTH(pbytes + rbytes);
  uint8_t plain1[outbytes];
  
  struct cipher_t
  {
    uint8_t buf[outbytes];
    uint8_t iv[ivbytes];
  };

  void SEncIV(cipher_t &out,
	      const uint8_t in[pbytes],
	      const uint8_t r[rbytes],
	      const uint8_t key[bbytes],
	      const uint8_t iv[ivbytes])
  {
    memset(&plain1[0], 0, outbytes);
    memcpy(&plain1[0], &in[0], pbytes);
    memcpy(&plain1[pbytes], &r[0], rbytes);
    memcpy(&out.iv[0], &iv[0], ivbytes);
    AES_CBC_encrypt(&plain1[0], &out.buf[0], outbytes, key, bbytes, &out.iv[0]);
  }

  void SEnc(cipher_t &out,
	    const uint8_t in[pbytes],
	    const uint8_t r[rbytes],
	    const uint8_t key[bbytes])
  {
    memset(&plain1[0], 0, outbytes);
    memcpy(&plain1[0], &in[0], pbytes);
    memcpy(&plain1[pbytes], &r[0], rbytes);
    nfl::fastrandombytes(&out.iv[0], ivbytes);
    AES_CBC_encrypt(&plain1[0], &out.buf[0], outbytes, key, bbytes, &out.iv[0]);
  }

  void SDec(uint8_t out[pbytes],
	    const cipher_t &in,
	    const uint8_t key[bbytes])
  {
    AES_CBC_decrypt(&in.buf[0], &plain1[0], outbytes, key, bbytes, &in.iv[0]);
    memcpy(out, &plain1[0], pbytes);
  }
};

#endif
