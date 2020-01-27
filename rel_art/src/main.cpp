#include <CUnit/Basic.h>
#include <iostream>
#include "ddhcry.hpp"

void ddhcry_test()
{
  const size_t numtests = 1000;

  ec_params_t params(NID_X9_62_prime256v1);
  ec_cry_t cry(params);

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

  CU_basic_set_mode(CU_BRM_VERBOSE);
  CU_basic_run_tests();
  CU_cleanup_registry();
  return CU_get_error();
}
