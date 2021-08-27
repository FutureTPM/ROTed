/**
@file
*/
#ifndef __DDHCRY_HPP__
#define __DDHCRY_HPP__
#include "ddh.hpp"

/** Implements Dual-Mode Cryptosystem from DDH based on [PVW08]

[PVW08] Chris Peikert, Vinod Vaikuntanathan, and Brent Waters. A framework
for efficient and composable oblivious transfer. In David Wagner, editor,
Advances in Cryptology – CRYPTO 2008, volume 5157 of Lecture Notes in
Computer Science, pages 554–571, Santa Barbara, CA, USA, August 17–21,
2008. Springer, Heidelberg, Germany. doi:10.1007/978-3-540-85174-5_31.
*/
struct ec_cry_t
{
  /**@{*/
  /** Public key */
  EC_POINT *g, *h, *g1, *h1;
  /**@}*/
  /** Private key */
  BIGNUM *x;
  /** Underlying EC */
  ec_params_t &params;
  /** Encoding parameter */
  int k;

  /** Cryptosystem constructor

      @param params1 EC parameter
      @param k1 Encoding parameter
  */
  ec_cry_t(ec_params_t &params1, int k1)
    : params(params1)
  {
    g = NULL;
    h = NULL;
    g1 = NULL;
    h1 = NULL;
    x = NULL;
    k = k1;
  }

  /** Encodes a number m as an EC point.
      Finds EC point p = (x, y) such that x = m * 2^k + h
      where h is a small value (< 2^k)

      @param p Returned point p
      @param m Encoded message
  */
  inline void encode(EC_POINT *p, BIGNUM *m)
  {
    BIGNUM *shifted_m = BN_dup(m);

    BN_lshift(shifted_m, m, k);

    while (
            EC_POINT_set_compressed_coordinates_GFp(params.group, p,
                                                    shifted_m, 0,
                                                    params.ctx) != 1)
      {
        BN_add_word(shifted_m, 1);
      }

    BN_free(shifted_m);
  }

  /** Implements the inverse of encode, by computing
      m = floor(x / 2^k) for p = (x, y)

      @param m Returned message
      @param p Inputted point
  */
  inline void decode(BIGNUM *m, EC_POINT *p)
  {
    BIGNUM *y = BN_new();

    EC_POINT_get_affine_coordinates_GFp(params.group, p, m, y, params.ctx);
    BN_rshift(m, m, k);

    BN_free(y);
  }

  /** Initializes pointers to public-key

      in_h = in_g^x
      in_h1 = in_g1^x

      @param in_g in_h = in_g^x
      @param in_h in_h = in_g^x
      @param in_g1 in_h1 = in_g1^x
      @param in_h1 in_h1 = in_g1^x
   */
  inline void set_pk(EC_POINT *in_g,
		     EC_POINT *in_h,
		     EC_POINT *in_g1,
		     EC_POINT *in_h1)
  {
    g = in_g;
    h = in_h;
    g1 = in_g1;
    h1 = in_g1;
  }

  /** Initializes pointer to secret-key

      Value x s.t.

      in_h = in_g^x
      in_h1 = in_g1^x

      @param in_x Secret-key number
  */
  inline void set_sk(BIGNUM *in_x)
  {
    BN_free(x);
    x = in_x;
  }

  /** Generates private/public-key pair */
  void keygen()
  {
    x = BN_new();
    BN_rand_range(x, params.order);

    g = EC_POINT_new(params.group);
    h = EC_POINT_new(params.group);
    g1 = EC_POINT_new(params.group);
    h1 = EC_POINT_new(params.group);

    params.random_generator(g);
    params.random_generator(h);
    params.point_mul(g1, g, x); //g1 = g^x
    params.point_mul(h1, h, x); //h1 = h^x
  }

  /** Produces (u,v) s.t.

      u = g^s h^t, v = g1^s h1^t = (g^s + h^t)^x
      where (g, h, g1, h1) is the public-key
      and s and t are random values

      @param u Outputted randomized u = g^s h^t
      @param v Outputted randomized v = g1^s h1^t
  */
  void randomise(EC_POINT *u, EC_POINT *v)
  {
    BIGNUM *s = BN_new();
    BIGNUM *t = BN_new();
    EC_POINT *aux = EC_POINT_new(params.group);

    params.point_mul(u, g, s);
    params.point_mul(aux, h, t);
    params.point_add(u, u, aux); //u = g^s h^t

    params.point_mul(v, g1, s);
    params.point_mul(aux, h1, t);
    params.point_add(v, v, aux); //v = g1^s h1^t = (g^s + h^t)^x

    EC_POINT_free(aux);
    BN_free(s);
    BN_free(t);
  }

  /** Encrypts message under (u, v)

      First, randomise is called on (u, v), then
      (u, v) := (u, v * m) is computed

      @param u Encryption of m
      @param v Encryption of m
      @param m Message to be encrypted
  */
  inline void encrypt(EC_POINT *u, EC_POINT *v, EC_POINT *m)
  {
    randomise(u, v); //v = u^x
    params.point_add(v, v, m); //v = u^x m
  }

  /** Decrypts (u, v) and places the output in m

      Computes m := v / u^x

      @param m Decrypted message
      @param u Ciphertext
      @param v Ciphertext
  */
  inline void decrypt(EC_POINT *m, EC_POINT *u, EC_POINT *v)
  {
    params.point_mul(u, u, x);
    params.point_inv(u);
    params.point_add(m, v, u); //m = v / u^x
  }

  /** Copy constructor

      @param other Copied structure */
  ec_cry_t(const ec_cry_t &other)
    : params(other.params)
  {
    g = other.g;
    h = other.h;
    g1 = other.g1;
    h1 = other.h1;

    x = BN_dup(other.x);
    k = other.k;
  }

  /** Swaps resources of two ec_params_t structures

      @param other Swapped structure */
  void swap(ec_cry_t &other)
  {
    std::swap(g, other.g);
    std::swap(h, other.h);
    std::swap(g1, other.g1);
    std::swap(h1, other.h1);
    std::swap(x, other.x);
    std::swap(k, other.k);
  }

  /** Equal operator (implemented from copy constructor and
      swap operation

      @param other Structure to be replicated
  */
  ec_cry_t &operator=(ec_cry_t other)
  {
    this->swap(other);

    return *this;
  }

  /** Cleans up resources */
  void cleanup()
  {
    g = NULL;
    h = NULL;
    g1 = NULL;
    h1 = NULL;
    BN_free(x);
    x = NULL;
  }

  /** Calls cleanup() */
  ~ec_cry_t()
  {
    cleanup();
  }
};

#endif
