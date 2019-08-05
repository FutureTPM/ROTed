#include <CUnit/Basic.h>
#include "rlweke.hpp"
#include "rlweot.hpp"
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

void ot_test()
{
  const size_t numtests = 10;
  using P = nfl::poly<uint16_t, 512, 1>;
  constexpr size_t rbytes = 16;
  constexpr size_t bbytes = 16;
  nfl::uniform unif;
  typedef typename sym_enc_t<rbytes, rbytes, bbytes>::cipher_t cipher_t;
 
  for (int i = 0; i < numtests; i++)
    {
      P m = unif;
      alice_ot_t<P, rbytes, bbytes> alice(8/sqrt(2*M_PI), 128, 512);
      bob_ot_t<P, rbytes, bbytes> bob(8/sqrt(2*M_PI), 128, 512);
      int b = i & 1;
      uint32_t sid = i;
      P p0, pS, signal0, signal1;
      uint8_t u0[2*rbytes + bbytes];
      uint8_t u1[2*rbytes + bbytes];
      uint8_t r_sid[sizeof(sid) + rbytes];
      cipher_t a0, a1;
      cipher_t c0, c1;
      uint8_t ch[rbytes];
      uint8_t msg0[rbytes], msg1[rbytes], msgb[rbytes];

      nfl::fastrandombytes(msg0, rbytes);
      nfl::fastrandombytes(msg1, rbytes);

      bool success = true;

      alice.msg1(p0, r_sid, b, sid, m);
      bob.msg1(pS, signal0, signal1,
	       u0, u1, a0, a1,
	       sid,
	       p0, r_sid, m);
      success = success && alice.msg2(ch, sid, pS,
				      signal0, signal1,
				      a0, a1,
				      u0, u1);
      success = success && bob.msg2(c0, c1, ch, msg0, msg1);
      alice.msg3(msgb, c0, c1);

      if (b == 0)
	{
	  success = success &&
	    (memcmp(msgb, msg0, rbytes) == 0);
	}
      else
	{
	  success = success &&
	    (memcmp(msgb, msg1, rbytes) == 1);
	}
      CU_ASSERT(success);
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

  CU_pSuite suite2 = CU_add_suite("RLWEOT", NULL, NULL);
  if (suite == NULL) abort();

  if ((NULL == CU_add_test(suite2, "ot_test", ot_test)))
    {
      abort();
    }

  CU_basic_set_mode(CU_BRM_VERBOSE);
  CU_basic_run_tests();
  CU_cleanup_registry();
  return CU_get_error();  
}  
