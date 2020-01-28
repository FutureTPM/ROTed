#ifndef __DDHCRY_HPP__
#define __DDHCRY_HPP__
#include "ddh.hpp"

struct ec_cry_t
{
  EC_POINT *g, *h, *g1, *h1;
  BIGNUM *x;
  ec_params_t &params;
  int k;

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

  void encode(EC_POINT *p, BIGNUM *m)
  {
    BIGNUM *shifted_m = BN_dup(m);

    BN_lshift(shifted_m, m, k);

    while (EC_POINT_set_compressed_coordinates_GFp
           (params.group, p, shifted_m, 0, params.ctx) != 1)
      {
        BN_add_word(shifted_m, 1);
      }

    BN_free(shifted_m);
  }

  void decode(BIGNUM *m, EC_POINT *p)
  {
    BIGNUM *y = BN_new();

    EC_POINT_get_affine_coordinates_GFp
      (params.group, p, m, y, params.ctx);
    BN_rshift(m, m, k);

    BN_free(y);
  }

  //in_g1 = in_g^x, in_h1 = in_h^x
  void set_pk(EC_POINT *in_g,
              EC_POINT *in_h,
              EC_POINT *in_g1,
              EC_POINT *in_h1)
  {
    EC_POINT_free(g);
    EC_POINT_free(h);
    EC_POINT_free(g1);
    EC_POINT_free(h1);

    g = EC_POINT_dup(in_g, params.group);
    h = EC_POINT_dup(in_h, params.group);
    g1 = EC_POINT_dup(in_g1, params.group);
    h1 = EC_POINT_dup(in_h1, params.group);
  }

  void set_sk(BIGNUM *in_x)
  {
    BN_free(x);
    x = BN_dup(in_x);
  }

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

  void encrypt(EC_POINT *u, EC_POINT *v, EC_POINT *m)
  {
    randomise(u, v); //v = u^x
    params.point_add(v, v, m); //v = u^x m
  }

  void decrypt(EC_POINT *m, EC_POINT *u, EC_POINT *v)
  {
    params.point_mul(u, u, x);
    params.point_inv(u);
    params.point_add(m, v, u); //m = v / u^x
  }

  ec_cry_t(const ec_cry_t &other)
    : params(other.params)
  {
    auto copy_if_not_null = [&] (EC_POINT *&p, EC_POINT *p1) -> void
    {
      if (p1 != NULL)
        p = EC_POINT_dup(p1, this->params.group);
      else
        p = NULL;
    };
    copy_if_not_null(g, other.g);
    copy_if_not_null(h, other.h);
    copy_if_not_null(g1, other.g1);
    copy_if_not_null(h1, other.h1);

    if (other.x != NULL)
      x = BN_dup(other.x);
    else
      x = NULL;

    k = other.k;
  }

  void swap(ec_cry_t &other)
  {
    std::swap(g, other.g);
    std::swap(h, other.h);
    std::swap(g1, other.g1);
    std::swap(h1, other.h1);
    std::swap(x, other.x);
    std::swap(k, other.k);
  }

  ec_cry_t &operator=(ec_cry_t other)
  {
    this->swap(other);

    return *this;
  }

  void cleanup()
  {
    EC_POINT_free(g);
    g = NULL;
    EC_POINT_free(h);
    h = NULL;
    EC_POINT_free(g1);
    g1 = NULL;
    EC_POINT_free(h1);
    h1 = NULL;
    BN_free(x);
    x = NULL;
  }

  ~ec_cry_t()
  {
    cleanup();
  }
};

#endif
