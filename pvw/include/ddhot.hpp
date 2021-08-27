/**
@file
*/
#ifndef __DDHOT_HPP__
#define __DDHOT_HPP__
#include "ddhcry.hpp"

/** Implements the CRS of OT from DDH as per [PVW08]

[PVW08] Chris Peikert, Vinod Vaikuntanathan, and Brent Waters. A framework
for efficient and composable oblivious transfer. In David Wagner, editor,
Advances in Cryptology – CRYPTO 2008, volume 5157 of Lecture Notes in
Computer Science, pages 554–571, Santa Barbara, CA, USA, August 17–21,
2008. Springer, Heidelberg, Germany. doi:10.1007/978-3-540-85174-5_31.
*/
struct crs_t
{
  /**@{*/
  /** EC points such that h := g^x and h1 := g1^x1 */
  EC_POINT *g, *h, *g1, *h1;
  /**@}*/
  /** EC parameters */
  ec_params_t &params;
  /** Encoding parameters */
  int k;

  /** Initializes CRS

      @param params1 EC parameters
      @param k1 Encoding parameter
  */
  crs_t(ec_params_t &params1, int k1)
    : params(params1)
  {
    k = k1;
    BIGNUM *x = BN_new();
    BIGNUM *x1 = BN_new();

    BN_rand_range(x, params.order);
    BN_rand_range(x1, params.order);

    g = EC_POINT_new(params.group);
    h = EC_POINT_new(params.group);
    g1 = EC_POINT_new(params.group);
    h1 = EC_POINT_new(params.group);

    params.random_generator(g);
    params.random_generator(h);

    params.point_mul(h, g, x); //h = g^x
    params.point_mul(h1, g1, x1); //h1 = g1^x1

    BN_free(x);
    BN_free(x1);
  }

  /** Copy constructor

      @param other Copied structure */
  crs_t(const crs_t &other)
    : params(other.params)
  {
    g = EC_POINT_dup(other.g, this->params.group);
    h = EC_POINT_dup(other.h, this->params.group);
    g1 = EC_POINT_dup(other.g1, this->params.group);
    h1 = EC_POINT_dup(other.h1, this->params.group);
    k = other.k;
  }

  /** Swaps resources of two ec_params_t structures

      @param other Swapped structure */
  void swap(crs_t &other)
  {
    std::swap(g, other.g);
    std::swap(h, other.h);
    std::swap(g1, other.g1);
    std::swap(h1, other.h1);
    std::swap(k, other.k);
  }

  /** Equal operator (implemented from copy constructor and
      swap operation)

      @param other Structure to be replicated
  */
  crs_t &operator=(crs_t other)
  {
    this->swap(other);

    return *this;
  }

  /** Cleans up resources */
  ~crs_t()
  {
    EC_POINT_free(g);
    EC_POINT_free(h);
    EC_POINT_free(g1);
    EC_POINT_free(h1);
  }
};

/** Implements Alice in the OT from DDH in [PVW08] */
struct alice_ot_t
{
  /** Chosen channel */
  int b;
  /** Common reference string */
  crs_t &crs;
  /** EC parameters */
  ec_params_t &params;
  /** Base dual-mode cryptosystem */
  ec_cry_t cry;

  /** Constructor for Alice

      @param crs1 CRS
  */
  alice_ot_t(crs_t &crs1)
    : crs(crs1),
      params(crs1.params),
      cry(crs1.params, crs1.k)
  {
  }

  /** Implements Alice's first message in OT from DDH in [PVW08]

      @param g Outputted point g = g_(b1)^x for a random x
      @param h Outputted point h = h_(b1)^x for a random x
      @param b1 Chosen OT channel
   */
  void msg1(EC_POINT *g, EC_POINT *h, int b1)
  {
    b = b1;
    BIGNUM *r = BN_new();
    BN_rand_range(r, params.order);
    cry.set_sk(r);

    EC_POINT *tmp1 = nullptr;
    EC_POINT *tmp2 = nullptr;

    if (b == 0) {
        tmp1 = crs.g;
        tmp2 = crs.h;
    } else {
        tmp1 = crs.g1;
        tmp2 = crs.h1;
    }

    params.point_mul(g, tmp1, r);
    params.point_mul(h, tmp2, r);

    //hi^r = (gi^xi)^r
  }

  /** Implements Alice's second message in OT from DDH in [PVW08]

      Outputs m := Decrypt(u_(b1), v_(b1)) for b1 chosen in msg1

      @param m Outputted message
      @param u Channel 0 ciphertext
      @param v Channel 0 ciphertext
      @param u1 Channel 1 ciphertext
      @param v1 Channel 1 ciphertext
  */
  void msg2(BIGNUM *m,
            EC_POINT *u, EC_POINT *v,
            EC_POINT *u1, EC_POINT *v1)
  {
    EC_POINT *pm = EC_POINT_new(params.group);
    if (b == 0) {
        cry.decrypt(pm, u, v);
    } else {
        cry.decrypt(pm, u1, v1);
    }
    cry.decode(m, pm);

    EC_POINT_free(pm);
  }
};

/** Implements Bob in the OT from DDH in [PVW08] */
struct bob_ot_t
{
  /** Common reference string */
  crs_t &crs;
  /** Common reference string */
  ec_params_t &params;
  /** Base dual-mode cryptosystem for channel 0 */
  ec_cry_t cry;
  /** Base dual-mode cryptosystem for channel 1 */
  ec_cry_t cry1;

  /** Constructor for Bob

      @param crs1 CRS
  */
  bob_ot_t(crs_t &crs1)
    : crs(crs1),
      params(crs1.params),
      cry(crs1.params, crs1.k),
      cry1(crs1.params, crs1.k)
  {
  }

  /** Implements Bob's first message in OT from DDH in [PVW08]

      Encrypts m under (crs.g, crs.h, g, h) and m1
      under (crs.g1, crs.h1, g, h), producing (u, v) and
      (u1, v1), respectively

      @param u Outputted ciphertext in channel 0
      @param v Outputted ciphertext in channel 0
      @param u1 Outputted ciphertext in channel 1
      @param v1 Outputted ciphertext in channel 1
      @param g Randomized point produced by Alice
      @param h Randomized point produced by Alice
      @param m Message encrypted under channel 0
      @param m1 Message encrypted under channel 1
   */
  void msg1(EC_POINT *u, EC_POINT *v,
            EC_POINT *u1, EC_POINT *v1,
            EC_POINT *g, EC_POINT *h,
            BIGNUM *m, BIGNUM *m1)
  {
    EC_POINT *pm = EC_POINT_new(params.group);
    EC_POINT *pm1 = EC_POINT_new(params.group);

    cry.encode(pm, m);
    cry1.encode(pm1, m1);

    cry.set_pk(crs.g, crs.h, g, h);
    cry1.set_pk(crs.g1, crs.h1, g, h);

    cry.encrypt(u, v, pm);
    cry1.encrypt(u1, v1, pm1);

    EC_POINT_free(pm);
    EC_POINT_free(pm1);
  }
};

#endif
