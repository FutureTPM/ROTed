#ifndef __DDHOT_HPP__
#define __DDHOT_HPP__
#include "ddhcry.hpp"

struct crs_t
{
  EC_POINT *g, *h, *g1, *h1;
  ec_params_t &params;
  int k;

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

  crs_t(const crs_t &other)
    : params(other.params)
  {
    g = EC_POINT_dup(other.g, this->params.group);
    h = EC_POINT_dup(other.h, this->params.group);
    g1 = EC_POINT_dup(other.g1, this->params.group);
    h1 = EC_POINT_dup(other.h1, this->params.group);
    k = other.k;
  }

  void swap(crs_t &other)
  {
    std::swap(g, other.g);
    std::swap(h, other.h);
    std::swap(g1, other.g1);
    std::swap(h1, other.h1);
    std::swap(k, other.k);
  }

  crs_t &operator=(crs_t other)
  {
    this->swap(other);

    return *this;
  }

  ~crs_t()
  {
    EC_POINT_free(g);
    EC_POINT_free(h);
    EC_POINT_free(g1);
    EC_POINT_free(h1);
  }
};

struct alice_ot_t
{
  int b;
  crs_t &crs;
  ec_params_t &params;
  ec_cry_t cry;

  alice_ot_t(crs_t &crs1)
    : crs(crs1),
      params(crs1.params),
      cry(crs1.params, crs1.k)
  {
  }

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
    // BN_free(r);
  }

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

struct bob_ot_t
{
  crs_t &crs;
  ec_params_t &params;
  ec_cry_t cry;
  ec_cry_t cry1;

  bob_ot_t(crs_t &crs1)
    : crs(crs1),
      params(crs1.params),
      cry(crs1.params, crs1.k),
      cry1(crs1.params, crs1.k)
  {
  }

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
