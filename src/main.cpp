#include <CUnit/Basic.h>
#include "rlweke.hpp"
#include <cstdlib>
#include <cstdint>

#ifndef M_PI
#define M_PI           3.14159265358979323846
#endif

void ke_test()
{
  const size_t numtests = 10;
  using P = nfl::poly<uint16_t, 512, 1>;
  nfl::uniform unif;

  for (int i = 0; i < numtests; i++)
    {
      P m = unif;
      alice_ke_t<P> alice(8/sqrt(2*M_PI), 128, 512);
      bob_ke_t<P> bob(8/sqrt(2*M_PI), 128, 512);
      P pA;
      P pB, signal;
      alice.msg(pA, m);
      bob.msg(pB, signal, pA, m);
      alice.reconciliate(pB, signal);

      CU_ASSERT(alice.sk == bob.sk);
    }
}

int main(int argc, char *argv[])
{
  if (CUE_SUCCESS != CU_initialize_registry())
    return CU_get_error();

  CU_pSuite suite = CU_add_suite("RLWEKE", NULL, NULL);
  if (suite == NULL) abort();

  if ((NULL == CU_add_test(suite, "ke_test", ke_test)))
    {
      abort();
    }

  CU_basic_set_mode(CU_BRM_VERBOSE);
  CU_basic_run_tests();
  CU_cleanup_registry();
  return CU_get_error();  
}  
