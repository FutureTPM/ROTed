#include <CUnit/Basic.h>
#include <iostream>
#include "ddhot.hpp"
#include "ddhcry.hpp"
#include <random>

void ddhot_test()
{
  const size_t numtests = 1000;
  ec_params_t params(NID_X9_62_prime256v1);

  EC_POINT *g, *h, *u, *v, *u1, *v1;
  int b;
  BIGNUM *m, *m1, *m2;

  std::default_random_engine gen;
  std::uniform_int_distribution<unsigned int> dist_bit(0,1);

  g = EC_POINT_new(params.group);
  h = EC_POINT_new(params.group);
  u = EC_POINT_new(params.group);
  v = EC_POINT_new(params.group);
  u1 = EC_POINT_new(params.group); 
  v1 = EC_POINT_new(params.group);
  m = BN_new();
  m1 = BN_new();
  m2 = BN_new();

  for (size_t i = 0; i < numtests; i++)
    {
      crs_t crs(params, 32);
      alice_ot_t alice(crs);
      bob_ot_t bob(crs);
      BN_rand(m, 32, BN_RAND_TOP_ANY, BN_RAND_BOTTOM_ANY);
      BN_rand(m1, 32, BN_RAND_TOP_ANY, BN_RAND_BOTTOM_ANY);

      b = dist_bit(gen);

      alice.msg1(g, h, b);
      bob.msg1(u, v,
               u1, v1,
               g, h,
               m, m1);
      alice.msg2(m2,
                 u, v,
                 u1, v1);

      if (b == 0)
        {
          BN_sub(m2, m2, m);
        }
      else
        {
          BN_sub(m2, m2, m1);
        }

      CU_ASSERT(BN_is_zero(m2));
    }

  EC_POINT_free(g);
  EC_POINT_free(h);
  EC_POINT_free(u);
  EC_POINT_free(v);
  EC_POINT_free(u1);
  EC_POINT_free(v1);
  BN_free(m);
  BN_free(m1);
  BN_free(m2);
}

void ddhcry_test()
{
  const size_t numtests = 1000;

  ec_params_t params(NID_X9_62_prime256v1);
  ec_cry_t cry(params, 32);

  EC_POINT *u, *v, *m1, *m2;

  u = EC_POINT_new(params.group);
  v = EC_POINT_new(params.group);
  m1 = EC_POINT_new(params.group);
  m2 = EC_POINT_new(params.group);

  for (size_t i = 0; i < numtests; i++)
    {
      cry.keygen();

      params.random_generator(m1);
      cry.encrypt(u, v, m1);
      cry.decrypt(m2, u, v);

      params.point_inv(m1);
      params.point_add(m1, m1, m2);

      CU_ASSERT(EC_POINT_is_at_infinity(params.group, m1));

      cry.cleanup();
    }

  EC_POINT_free(u);
  EC_POINT_free(v);
  EC_POINT_free(m1);
  EC_POINT_free(m2);
}

int main(int argc, char *argv[])
{
  if (CUE_SUCCESS != CU_initialize_registry())
    return CU_get_error();

  CU_pSuite suite = CU_add_suite("DDHCRY", NULL, NULL);
  if (suite == NULL) abort();

  if ((NULL == CU_add_test(suite, "ddhcry", ddhcry_test)))
    {
      abort();
    }

  CU_pSuite suite1 = CU_add_suite("DDHOT", NULL, NULL);
  if (suite1 == NULL) abort();

  if ((NULL == CU_add_test(suite1, "ddhot", ddhot_test)))
    {
      abort();
    }

  CU_basic_set_mode(CU_BRM_VERBOSE);
  CU_basic_run_tests();
  CU_cleanup_registry();
  return CU_get_error();
}
