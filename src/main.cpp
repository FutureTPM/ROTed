#include <CUnit/Basic.h>
#include "rlweke.hpp"
#include "rlweot.hpp"
#include <cstdlib>
#include <cstdint>
#include <algorithm>
#include "cpucycles.h"

#ifndef M_PI
#define M_PI           3.14159265358979323846
#endif

#define N 512
#define K 8

template<typename P>
bool pol_equal(const P &a, const P&b)
{
  bool equal = true;
  for (size_t i = 0; i < P::degree; i++)
    equal = equal && (a(0, i) == b(0, i));

  return equal;
}

void ke_test()
{
  const size_t numtests = 1;
  using P = nfl::poly_from_modulus<uint16_t, N, 14>;
  nfl::uniform unif;
  nfl::FastGaussianNoise<uint8_t, P::value_type, 2> g_prng(sqrt((double)K/2.), 138, N);

  for (int i = 0; i < numtests; i++)
    {
      P m = unif;
      alice_ke_t<P> alice(&g_prng);
      bob_ke_t<P> bob(&g_prng);
      P pA;
      P pB, signal;
      alice.msg(pA, m);
      bob.msg(pB, signal, pA, m);
      alice.reconciliate(pB, signal);

      CU_ASSERT(pol_equal(alice.sk, bob.sk));
    }
}

void ot_test()
{
  const size_t numtests = 1000;
  using P = nfl::poly_from_modulus<uint16_t, N, 14>;
  constexpr size_t rbytes = 16;
  constexpr size_t bbytes = 16;
  nfl::uniform unif;
  typedef typename sym_enc_t<rbytes, rbytes, bbytes>::cipher_t cipher_t;
  nfl::FastGaussianNoise<uint8_t, P::value_type, 2> g_prng(sqrt((double)K/2.), 138, N);

  for (int i = 0; i < numtests; i++)
    {
      //long long start = cpucycles_amd64cpuinfo();
      P m = unif;
      alice_ot_t<P, rbytes, bbytes> alice(&g_prng);
      bob_ot_t<P, rbytes, bbytes> bob(&g_prng);
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
    //if (b == 0)
	//{
	//  CU_ASSERT(pol_equal(alice.skR, bob.skS0));
	//  CU_ASSERT(std::equal(&alice.bskR[0],
	//		       &alice.bskR[bbytes],
	//		       &bob.bskS0[0]));
	//  CU_ASSERT(std::equal(&alice.xb[0],
	//		       &alice.xb[rbytes],
	//		       &bob.w0[0]));
	//  CU_ASSERT(std::equal(&alice.bxb[0],
	//		       &alice.bxb[rbytes],
	//		       &bob.bw0[0]));

	//  CU_ASSERT(std::equal(&alice.xbb[0],
	//		       &alice.xbb[rbytes],
	//		       &bob.w1[0]));
	//  CU_ASSERT(std::equal(&alice.bskRbb[0],
	//		       &alice.bskRbb[bbytes],
	//		       &bob.bskS1[0]));
	//  CU_ASSERT(std::equal(&alice.ybb[0],
	//		       &alice.ybb[rbytes],
	//		       &bob.z1[0]));

	//  CU_ASSERT(std::equal(&alice.yb[0],
	//		       &alice.yb[rbytes],
	//		       &bob.z0[0]));
	//}
    //  else
	//{
	//  CU_ASSERT(pol_equal(alice.skR, bob.skS1));
	//  CU_ASSERT(std::equal(&alice.bskR[0],
	//		       &alice.bskR[bbytes],
	//		       &bob.bskS1[0]));
	//  CU_ASSERT(std::equal(&alice.xb[0],
	//		       &alice.xb[rbytes],
	//		       &bob.w1[0]));
	//  CU_ASSERT(std::equal(&alice.bxb[0],
	//		       &alice.bxb[rbytes],
	//		       &bob.bw1[0]));

	//  CU_ASSERT(std::equal(&alice.xbb[0],
	//		       &alice.xbb[rbytes],
	//		       &bob.w0[0]));
	//  CU_ASSERT(std::equal(&alice.bskRbb[0],
	//		       &alice.bskRbb[bbytes],
	//		       &bob.bskS0[0]));
	//  CU_ASSERT(std::equal(&alice.ybb[0],
	//		       &alice.ybb[rbytes],
	//		       &bob.z0[0]));

	//  CU_ASSERT(std::equal(&alice.yb[0],
	//		       &alice.yb[rbytes],
	//		       &bob.z1[0]));
	//}
      if (success)
	{
	  success = success && bob.msg2(c0, c1, ch, msg0, msg1);
	}
      if (success)
	{
	  alice.msg3(msgb, c0, c1);
	}

      if (b == 0)
	{
	  success = success &&
	    (memcmp(&msgb[0], &msg0[0], rbytes) == 0);
	}
      else
	{
	  success = success &&
	    (memcmp(&msgb[0], &msg1[0], rbytes) == 0);
	}
      CU_ASSERT(success);
      //long long end = cpucycles_amd64cpuinfo();
      //printf("Clock cycles elapsed: %lld\n", end - start);
    }
}

void symenc_test()
{
  const size_t numtests = 1000;
  constexpr size_t pbytes = 14;
  constexpr size_t rbytes = 8;
  constexpr size_t bbytes = 16;
  typedef typename sym_enc_t<pbytes, rbytes, bbytes>::cipher_t cipher_t;
  sym_enc_t<pbytes, rbytes, bbytes> symenc;

  for (size_t i = 0; i < numtests; i++)
    {
      uint8_t in[pbytes], r[rbytes], key[bbytes];
      uint8_t out[pbytes];
      cipher_t c;

      nfl::fastrandombytes(in, pbytes);
      nfl::fastrandombytes(r, rbytes);
      nfl::fastrandombytes(key, bbytes);

      symenc.SEnc(c, in, r, key);
      symenc.SDec(out, c, key);

      CU_ASSERT(std::equal(&in[0], &in[pbytes], &out[0]));
    }
}

int main(int argc, char *argv[])
{
  if (CUE_SUCCESS != CU_initialize_registry())
    return CU_get_error();

  //CU_pSuite suite = CU_add_suite("RLWEKE", NULL, NULL);
  //if (suite == NULL) abort();

  //if ((NULL == CU_add_test(suite, "ke_test", ke_test)))
  //  {
  //    abort();
  //  }

  CU_pSuite suite2 = CU_add_suite("RLWEOT", NULL, NULL);
  if (suite2 == NULL) abort();

  if ((NULL == CU_add_test(suite2, "ot_test", ot_test)))
    {
      abort();
    }

  //CU_pSuite suite3 = CU_add_suite("symenc", NULL, NULL);
  //if (suite3 == NULL) abort();

  //if ((NULL == CU_add_test(suite3, "symenc_test", symenc_test)))
  //  {
  //    abort();
  //  }

  CU_basic_set_mode(CU_BRM_VERBOSE);
  CU_basic_run_tests();
  CU_cleanup_registry();
  return CU_get_error();
}
