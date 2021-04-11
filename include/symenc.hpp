#ifndef __SYMENC_HPP__
#define __SYMENC_HPP__
#include "macros.hpp"
#include <openssl/aes.h>

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

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
      ar & buf;
      ar & iv;
    }
  };

  void SEncIV(cipher_t &out,
	      const uint8_t in[pbytes],
	      const uint8_t r[rbytes],
	      const uint8_t key[bbytes],
	      const uint8_t iv[ivbytes])
  {
    memset(&plain1[pbytes + rbytes], 0, outbytes - pbytes - rbytes);
    memcpy(&plain1[0], &in[0], pbytes);
    memcpy(&plain1[pbytes], &r[0], rbytes);
    memcpy(&out.iv[0], &iv[0], ivbytes);
    AES_KEY openssl_key;
    AES_set_encrypt_key(key, bbytes * 8, &openssl_key);
    AES_cbc_encrypt(plain1, out.buf, outbytes, &openssl_key, (unsigned char *)out.iv, AES_ENCRYPT);
    memcpy(&out.iv[0], &iv[0], ivbytes);
  }

  void SEnc(cipher_t &out,
	    const uint8_t in[pbytes],
	    const uint8_t r[rbytes],
	    const uint8_t key[bbytes])
  {
    memset(&plain1[pbytes + rbytes], 0, outbytes - pbytes - rbytes);
    memcpy(&plain1[0], &in[0], pbytes);
    memcpy(&plain1[pbytes], &r[0], rbytes);
    uint8_t tmp_iv[ivbytes];
    memcpy(&tmp_iv[0], &out.iv[0], ivbytes);
    AES_KEY openssl_key;
    AES_set_encrypt_key(key, bbytes * 8, &openssl_key);
    AES_cbc_encrypt(plain1, out.buf, outbytes, &openssl_key, (unsigned char *)out.iv, AES_ENCRYPT);
    memcpy(&out.iv[0], &tmp_iv[0], ivbytes);
  }

  void SDec(uint8_t out[pbytes],
	    const cipher_t &in,
	    const uint8_t key[bbytes])
  {
    uint8_t tmp_iv[ivbytes];
    memcpy(&tmp_iv[0], &in.iv[0], ivbytes);
    AES_KEY openssl_key;
    AES_set_decrypt_key(key, bbytes * 8, &openssl_key);
    AES_cbc_encrypt(in.buf, plain1, outbytes, &openssl_key, (unsigned char *)in.iv, AES_DECRYPT);
    memcpy((unsigned char*)in.iv, &tmp_iv[0], ivbytes);
    memcpy(out, plain1, pbytes);
  }
};

#endif
