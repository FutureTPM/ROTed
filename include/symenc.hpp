#ifndef __SYMENC_HPP__
#define __SYMENC_HPP__
#include "macros.hpp"
#include <openssl/aes.h>

/** Symmetric-key encryption engine

    @tparam pbytes Size of plaintext
    @tparam rbytes Size of random bytes
    @tparam bbytes Size of key
*/
template<size_t pbytes, size_t rbytes, size_t bbytes>
struct sym_enc_t
{
  /** Size of IV */
  static constexpr size_t ivbytes = 16;
  static constexpr size_t ivlength = MIN(ivbytes, rbytes);
  /** Size of ciphertext for concatention of plaintext and random bytes */
  static constexpr size_t outbytes = AES_OUTPUT_LENGTH(pbytes);
  /** Used to encode/decode plaintext and random bytes */
  uint8_t plain1[outbytes];

  /** Symmetric-key ciphertext type */
  struct cipher_t
  {
    /** Ciphertext */
    uint8_t buf[outbytes];
    /** IV */
    uint8_t iv[ivbytes];
  };

  /** Encrypts (in || r) using (key, out.IV) with AES-CBC

      @param out Outputted ciphertext
      @param in Inputted plaintext
      @param r Inputted random value
      @param key Inputted key */
  void SEnc(cipher_t &out,
	    const uint8_t in[pbytes],
	    const uint8_t r[rbytes],
	    const uint8_t key[bbytes])
  {
    memset(&plain1[pbytes], 0, outbytes - pbytes);
    memcpy(&plain1[0], &in[0], pbytes);
    uint8_t tmp_iv[ivbytes];
    memset(&tmp_iv[ivlength], 0, ivbytes - ivlength);
    memcpy(&tmp_iv[0], &r[0], ivlength);
    memcpy(&out.iv[0], &tmp_iv[0], ivbytes);
    AES_KEY openssl_key;
    AES_set_encrypt_key(key, bbytes * 8, &openssl_key);
    AES_cbc_encrypt(plain1, out.buf, outbytes, &openssl_key, (unsigned char *)&tmp_iv[0], AES_ENCRYPT);
  }

  /** Decrypts in using key with AES-CBC

      @param out Outputted plaintext
      @param in Inputted ciphertext
      @param key Inputted key */
  void SDec(uint8_t out[pbytes],
	    const cipher_t &in,
	    const uint8_t key[bbytes])
  {
    AES_KEY openssl_key;
    AES_set_decrypt_key(key, bbytes * 8, &openssl_key);
    uint8_t tmp_iv[ivbytes];
    memcpy(&tmp_iv[0], &in.iv[0], ivbytes);
    AES_cbc_encrypt(in.buf, plain1, outbytes, &openssl_key, (unsigned char *)&tmp_iv[0], AES_DECRYPT);
    memcpy(out, plain1, pbytes);
  }
};

#endif
