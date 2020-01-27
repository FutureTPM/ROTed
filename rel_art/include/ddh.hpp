#ifndef __DDH_H__
#define __DDH_H__

#include <openssl/ec.h>
#include <openssl/bn.h>
#include <openssl/ecdh.h>
#include <openssl/obj_mac.h>
#include <algorithm>

struct ec_params_t
{
  EC_GROUP *group;
  BIGNUM *order;
  BN_CTX *ctx;

  ec_params_t(int nid)
  {
    group = EC_GROUP_new_by_curve_name(nid);
    ctx = BN_CTX_new();
    EC_GROUP_precompute_mult(group, ctx);
    order = BN_new();
    EC_GROUP_get_order(group, order, ctx);
  }

  /* assume same order and cofactor as previous generator */
  void set_generator(EC_POINT *gen)
  {
    BIGNUM *order = BN_new();
    BIGNUM *cofactor = BN_new();
    EC_GROUP_get_order(group, order, ctx);
    EC_GROUP_get_cofactor(group, cofactor, ctx);
    EC_GROUP_set_generator(group, gen, order, cofactor);
    EC_GROUP_precompute_mult(group, ctx);
    BN_free(order);
    BN_free(cofactor);
  }

  void point_add(EC_POINT *p, EC_POINT *u, EC_POINT *v)
  {
    EC_POINT_add(group, p, u, v, ctx);
  }

  void point_mul(EC_POINT *p, EC_POINT *g, BIGNUM *x)
  {
    EC_POINT_mul(group, p, NULL, g, x, ctx);
  }

  void point_inv(EC_POINT *p)
  {
    EC_POINT_invert(group, p, ctx);
  }

  void random_generator(EC_POINT *p)
  {
    BIGNUM *x = BN_new();
    BN_rand_range(x, order);
    BIGNUM *y = BN_new();
    BN_zero(y);
    EC_POINT *q = EC_POINT_new(group);
    EC_POINT_set_to_infinity(group, q);

    EC_POINT_mul(group, p, x, q, y, ctx);

    BN_free(x);
    BN_free(y);
  }

  ec_params_t(const ec_params_t &other)
  {
    this->group = EC_GROUP_dup(other.group);
    this->ctx = BN_CTX_new();
    EC_GROUP_precompute_mult(this->group, ctx);
    this->order = BN_dup(other.order);
  }

  void swap(ec_params_t &other)
  {
    std::swap(this->group, other.group);
    std::swap(this->ctx, other.ctx);
    std::swap(this->order, other.order);
  }

  ec_params_t &operator=(ec_params_t other)
  {
    this->swap(other);

    return *this;
  }

  ~ec_params_t()
  {
    EC_GROUP_free(group);
    BN_CTX_free(ctx);
    BN_free(order);
  }
};


#endif
