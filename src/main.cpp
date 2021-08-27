/*! \mainpage Lattice-Based Random Oblivious Transfer
 *
 * This repository and source code are the artifact for the paper
 * "ROTed: Random Oblivious Transfer for embedded devices" to be published at
 * [TCHES 2021](). As such, we strongly recommend reading the paper before using
 * the software herein.
 *
 * The repository contains multiple implementations for random and non-random
 * oblivious transfer (OT) algorithms. All code should build on ARM and x86
 * machines.
 *
 * \section folder Folder Structure
 *
 * * `include`, `pvw` and `src` contain our implementations.
 * * `thirdparty` contains third-party libraries which are needed to build the
 * project.
 * * `utils` contains utilities scripts to ease certain tasks
 *
 * \section code Code Overview
 *
 * There are 4 relevant folders: `nfl`, `include`, `pvw`, and `src`.
 *
 * [NFLlib](https://github.com/quarkslab/NFLlib) is the backend used to perform
 * lattice arithmetic. We have modified `nfl` in order to support an AVX512
 * backend for x86 architectures and an NEON
 * backend for ARM architectures. Specifically we added the files
 * avx512.hpp and neon.hpp to `nfl/include/nfl/opt/arch/`.
 * We added the backend support to the operations in ops.hpp and architectural
 * support in arch.hpp. The cmake script was also modified in
 * order to support the new backends.
 *
 * `include` contains all of the data structures used in order to run the proposed
 * ROTs and OTs. The OT implementation is in rlweot.hpp, and
 * the ROT implementation is in rlwerot.hpp. The random
 * oracle implementations are in roms.hpp. All
 * implementations are templated in order to facilitate parameters
 * modifications without sacrificing performance.
 *
 * `pvw` contains the implementation for the PVW08 proposal. The
 * implementation uses OpenSSL as a backend for elliptic curve arithmetic.
 *
 * `src` contains the instantiation and tests for the proposed OT and ROT
 * implementations.
 */
#include <CUnit/Basic.h>
#include "rlweke.hpp"
#include "rlweot.hpp"
#include "rlwerot.hpp"
#include <cstdlib>
#include <cstdint>
#include <algorithm>
#include "comm.hpp"
#include "comm_rot.hpp"
#include <fstream>

#ifndef M_PI
#define M_PI           3.14159265358979323846
#endif

#define N 512
#define K 8

/** Compares two NFL polynomials

    @param a Comparison operand
    @param b Comparison operand
    @return True when a == b
*/
template<typename P>
bool pol_equal(const P &a, const P&b)
{
  bool equal = true;
  for (size_t i = 0; i < P::degree; i++)
    equal = equal && (a(0, i) == b(0, i));

  return equal;
}

/** OT from RLWE test [BDGM19] */
void comm_test()
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
      comm_msg_1_t<P, rbytes> msg_1a, msg_1b;
      comm_msg_2_t<P, rbytes, bbytes, cipher_t> msg_2a, msg_2b;
      comm_msg_3_t<rbytes> msg_3a, msg_3b;
      comm_msg_4_t<cipher_t> msg_4a, msg_4b;
      P m = unif;
      alice_ot_t<P, rbytes, bbytes> alice(&g_prng);
      bob_ot_t<P, rbytes, bbytes> bob(&g_prng);
      int b = i & 1;
      uint32_t sid = i;
      cipher_t c0, c1;
      uint8_t ch[rbytes];
      uint8_t msg0[rbytes], msg1[rbytes], msgb[rbytes];

      nfl::fastrandombytes(msg0, rbytes);
      nfl::fastrandombytes(msg1, rbytes);

      bool success = true;

      alice.msg1(msg_1a.p0, msg_1a.r_sid, b, sid, m);
      msg_1a.sid = sid;

      memcpy(&msg_1b, &msg_1a, sizeof(comm_msg_1_t<P, rbytes>));

      bob.msg1(msg_2a.pS, msg_2a.signal0, msg_2a.signal1,
               msg_2a.u0, msg_2a.u1, msg_2a.a0, msg_2a.a1,
               msg_1b.sid,
               msg_1b.p0, msg_1b.r_sid, m);
      msg_2a.sid = msg_1b.sid;

      memcpy(&msg_2b, &msg_2a, sizeof(comm_msg_2_t<P, rbytes, bbytes, cipher_t>));

      success = success && alice.msg2(msg_3a.ch, msg_2b.sid, msg_2b.pS,
                                      msg_2b.signal0, msg_2b.signal1,
                                      msg_2b.a0, msg_2b.a1,
                                      msg_2b.u0, msg_2b.u1);
      msg_3a.sid = msg_2b.sid;

      if (success)
        {
	  memcpy(&msg_3b, &msg_3a, sizeof(comm_msg_3_t<rbytes>));

          success = success && bob.msg2(msg_4a.c0, msg_4a.c1, msg_3b.ch, msg0, msg1);
          msg_4a.sid = msg_3b.sid;
        }
      if (success)
        {
	  memcpy(&msg_4b, &msg_4a, sizeof(comm_msg_4_t<cipher_t>));

          alice.msg3(msgb, msg_4b.c0, msg_4b.c1);
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
    }
}

/** Implements ROT from OT in [BDGM19] using black-box transform
*/
void comm_rotted_test()
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
      // Sender
      int c_sender = ((i ^ 0xdeadbeef) >> 4) & 0x1;

      // Receiver
      uint8_t r0_receiver[32], r1_receiver[32];
      nfl::fastrandombytes(r0_receiver, 32);
      nfl::fastrandombytes(r1_receiver, 32);

      uint8_t r0_receiver_hash[32], r1_receiver_hash[32];
      blake3(r0_receiver_hash, r0_receiver, 32);
      blake3(r1_receiver_hash, r1_receiver, 32);

      uint8_t r0_sender_hash[32], r1_sender_hash[32];
      memcpy(r0_sender_hash, r0_receiver_hash, 32);
      memcpy(r1_sender_hash, r1_receiver_hash, 32);

      /*
       * Start OT
       */
      comm_msg_1_t<P, rbytes> msg_1a, msg_1b;
      comm_msg_2_t<P, rbytes, bbytes, cipher_t> msg_2a, msg_2b;
      comm_msg_3_t<rbytes> msg_3a, msg_3b;
      comm_msg_4_t<cipher_t> msg_4a, msg_4b;
      P m = unif;
      alice_ot_t<P, rbytes, bbytes> alice(&g_prng);
      bob_ot_t<P, rbytes, bbytes> bob(&g_prng);
      int b = i & 1;
      uint32_t sid = i;
      uint8_t ch[rbytes];
      uint8_t msg0[rbytes], msg1[rbytes], msgb[rbytes];

      nfl::fastrandombytes(msg0, rbytes);
      nfl::fastrandombytes(msg1, rbytes);

      bool success = true;

      alice.msg1(msg_1a.p0, msg_1a.r_sid, b, sid, m);
      msg_1a.sid = sid;

      memcpy(&msg_1b, &msg_1a, sizeof(comm_msg_1_t<P, rbytes>));

      bob.msg1(msg_2a.pS, msg_2a.signal0, msg_2a.signal1,
               msg_2a.u0, msg_2a.u1, msg_2a.a0, msg_2a.a1,
               msg_1b.sid,
               msg_1b.p0, msg_1b.r_sid, m);
      msg_2a.sid = msg_1b.sid;

      memcpy(&msg_2b, &msg_2a, sizeof(comm_msg_2_t<P, rbytes, bbytes, cipher_t>));

      success = success && alice.msg2(msg_3a.ch, msg_2b.sid, msg_2b.pS,
                                      msg_2b.signal0, msg_2b.signal1,
                                      msg_2b.a0, msg_2b.a1,
                                      msg_2b.u0, msg_2b.u1);
      msg_3a.sid = msg_2b.sid;

      if (success)
        {
	  memcpy(&msg_3b, &msg_3a, sizeof(comm_msg_3_t<rbytes>));

          success = success && bob.msg2(msg_4a.c0, msg_4a.c1, msg_3b.ch, msg0, msg1);
          msg_4a.sid = msg_3b.sid;
        }
      if (success)
        {
	  memcpy(&msg_4b, &msg_4a, sizeof(comm_msg_4_t<cipher_t>));

          alice.msg3(msgb, msg_4b.c0, msg_4b.c1);
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
      /*
       * END OT
       */

      // Sender
      // Receive r0 and r1
      uint8_t r0_sender[32], r1_sender[32];
      memcpy(r0_sender, r0_receiver, 32);
      memcpy(r1_sender, r1_receiver, 32);

      // Check H(r0) and H(r1)
      uint8_t r0_sender_hash_gen[32], r1_sender_hash_gen[32];
      blake3(r0_sender_hash_gen, r0_sender, 32);
      blake3(r1_sender_hash_gen, r1_sender, 32);

      bool r0_hash_all_equal = true;
      bool r1_hash_all_equal = true;

      for (size_t j = 0; j < 32; j++) {
          r0_hash_all_equal &= (r0_sender_hash_gen[j] == r0_sender_hash[j]);
          r1_hash_all_equal &= (r1_sender_hash_gen[j] == r1_sender_hash[j]);
      }
      CU_ASSERT(r0_hash_all_equal);
      CU_ASSERT(r1_hash_all_equal);

      uint8_t r0_M0_xor_result[rbytes], r1_M1_xor_result[rbytes];
      if (c_sender == 0) {
          for (size_t j = 0; j < rbytes; j++) {
              r0_M0_xor_result[j] = r0_sender[j] ^ msg0[j];
              r1_M1_xor_result[j] = r1_sender[j] ^ msg1[j];
          }
      } else {
          for (size_t j = 0; j < rbytes; j++) {
              r0_M0_xor_result[j] = r1_sender[j] ^ msg1[j];
              r1_M1_xor_result[j] = r0_sender[j] ^ msg0[j];
          }
      }

      // Receiver
      // Receive c and r
      int c_receiver;
      memcpy(&c_receiver, &c_sender, 4);

      int c_xor_b_result = c_receiver ^ b;
      uint8_t rb_Mb_xor_result[rbytes];

      if (b == 0) {
          for (size_t j = 0; j < rbytes; j++) {
              rb_Mb_xor_result[j] = r0_receiver[j] ^ msgb[j];
          }
      } else {
          for (size_t j = 0; j < rbytes; j++) {
              rb_Mb_xor_result[j] = r1_receiver[j] ^ msgb[j];
          }
      }

      // Final Check
      bool final_message_all_equal = true;
      if (c_xor_b_result == 0) {
          for (size_t j = 0; j < rbytes; j++) {
              final_message_all_equal &= (rb_Mb_xor_result[j] == r0_M0_xor_result[j]);
          }
      } else {
          for (size_t j = 0; j < rbytes; j++) {
              final_message_all_equal &= (rb_Mb_xor_result[j] == r1_M1_xor_result[j]);
          }
      }
      CU_ASSERT(final_message_all_equal);
    }
}

/** Proposed ROT test */
void comm_rot_test()
{
  const size_t numtests = 1000;
  using P = nfl::poly_from_modulus<uint16_t, N, 14>;
  constexpr size_t rbytes = 16;
  constexpr size_t bbytes = 16;
  constexpr size_t HASHSIZE = 32;
  nfl::uniform unif;
  nfl::FastGaussianNoise<uint8_t, P::value_type, 2> g_prng(sqrt((double)K/2.), 138, N);

  for (int i = 0; i < numtests; i++)
    {
      P m = unif;
      alice_rot_t<P, rbytes, bbytes, HASHSIZE> alice(&g_prng);
      bob_rot_t<P, rbytes, bbytes, HASHSIZE> bob(&g_prng);
      uint32_t sid = i;
      uint8_t msg0[HASHSIZE], msg1[HASHSIZE], msgb[HASHSIZE];
      int b;

      comm_rot_msg_1_t<P, rbytes, HASHSIZE> msg_1a, msg_1b;
      comm_rot_msg_2_t<P, bbytes, HASHSIZE> msg_2a, msg_2b;
      comm_rot_msg_3_t<bbytes> msg_3a, msg_3b;

      bool success = true;

      alice.msg1(msg_1a.p0, msg_1a.r_sid, msg_1a.hS0, msg_1a.hS1, sid, m);
      msg_1a.sid = sid;

      memcpy(&msg_1b, &msg_1a, sizeof(comm_rot_msg_1_t<P, rbytes, HASHSIZE>));

      bob.msg1(msg_2a.pS, msg_2a.signal0, msg_2a.signal1,
               msg_2a.u, msg_2a.hma0, msg_2a.hma1,
               msg_1b.sid,
           msg_1b.hS0, msg_1b.hS1,
               msg_1b.p0, msg_1b.r_sid, m);
      msg_2a.sid = msg_1b.sid;

      memcpy(&msg_2b, &msg_2a, sizeof(comm_rot_msg_2_t<P, bbytes, HASHSIZE>));

      success = success && alice.msg2(msgb, b, msg_3a.S0, msg_3a.S1,
                      msg_2b.sid, msg_2b.pS,
                                      msg_2b.signal0, msg_2b.signal1,
                                      msg_2b.hma0, msg_2b.hma1,
                                      msg_2b.u);
      msg_3a.sid = msg_2b.sid;

      if (success)
        {
            memcpy(&msg_3b, &msg_3a, sizeof(comm_rot_msg_3_t<bbytes>));

	    success = success && bob.msg2(msg0, msg1, msg_3b.sid, msg_3b.S0, msg_3b.S1);
        }

      if (b == 0)
        {
          success = success &&
            (memcmp(&msgb[0], &msg0[0], HASHSIZE) == 0);
        }
      else
        {
          success = success &&
            (memcmp(&msgb[0], &msg1[0], HASHSIZE) == 0);
        }
      CU_ASSERT(success);
    }
}

int main(int argc, char *argv[])
{
  if (CUE_SUCCESS != CU_initialize_registry())
    return CU_get_error();

#ifdef OT_ROTTED_TEST
#pragma message ("Compiling OT_ROTTED_TEST")
  CU_pSuite suite3 = CU_add_suite("RLWEOT", NULL, NULL);
  if (suite3 == NULL) abort();

  if ((NULL == CU_add_test(suite3, "comm_rotted_test", comm_rotted_test)))
    {
      abort();
    }
#else
#ifdef OT_TEST
  CU_pSuite suite3 = CU_add_suite("RLWEOT", NULL, NULL);
  if (suite3 == NULL) abort();

  if ((NULL == CU_add_test(suite3, "comm_test", comm_test)))
    {
      abort();
    }
#else
  CU_pSuite suite4 = CU_add_suite("RLWEROT", NULL, NULL);
  if (suite4 == NULL) abort();

  if ((NULL == CU_add_test(suite4, "comm_rot_test", comm_rot_test)))
    {
      abort();
    }
#endif // OT_TEST
#endif // OT_ROTTED_TEST

  CU_basic_set_mode(CU_BRM_VERBOSE);
  CU_basic_run_tests();
  CU_cleanup_registry();
  return CU_get_error();
}
