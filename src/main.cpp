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
  const size_t numtests = 10;
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
      //{
      //  std::ofstream ofs("msg_1.txt");
      //  boost::archive::text_oarchive oa(ofs);
      //  oa << msg_1a;
      //}

      //{
      //  std::ifstream ifs("msg_1.txt");
      //  boost::archive::text_iarchive ia(ifs);
      //  ia >> msg_1b;
      //}

      bob.msg1(msg_2a.pS, msg_2a.signal0, msg_2a.signal1,
               msg_2a.u0, msg_2a.u1, msg_2a.a0, msg_2a.a1,
               msg_1b.sid,
               msg_1b.p0, msg_1b.r_sid, m);
      msg_2a.sid = msg_1b.sid;

      memcpy(&msg_2b, &msg_2a, sizeof(comm_msg_2_t<P, rbytes, bbytes, cipher_t>));
      //{
      //  std::ofstream ofs("msg_2.txt");
      //  boost::archive::text_oarchive oa(ofs);
      //  oa << msg_2a;
      //}

      //{
      //  std::ifstream ifs("msg_2.txt");
      //  boost::archive::text_iarchive ia(ifs);
      //  ia >> msg_2b;
      //}

      success = success && alice.msg2(msg_3a.ch, msg_2b.sid, msg_2b.pS,
                                      msg_2b.signal0, msg_2b.signal1,
                                      msg_2b.a0, msg_2b.a1,
                                      msg_2b.u0, msg_2b.u1);
      msg_3a.sid = msg_2b.sid;

      //if (b == 0)
      //  {
      //    CU_ASSERT(pol_equal(alice.skR, bob.skS0));
      //    CU_ASSERT(std::equal(&alice.bskR[0],
      //                         &alice.bskR[bbytes],
      //                         &bob.bskS0[0]));
      //    CU_ASSERT(std::equal(&alice.xb[0],
      //                         &alice.xb[rbytes],
      //                         &bob.w0[0]));
      //    CU_ASSERT(std::equal(&alice.bxb[0],
      //                         &alice.bxb[rbytes],
      //                         &bob.bw0[0]));

      //    CU_ASSERT(std::equal(&alice.xbb[0],
      //                         &alice.xbb[rbytes],
      //                         &bob.w1[0]));
      //    CU_ASSERT(std::equal(&alice.bskRbb[0],
      //                         &alice.bskRbb[bbytes],
      //                         &bob.bskS1[0]));
      //    CU_ASSERT(std::equal(&alice.ybb[0],
      //                         &alice.ybb[rbytes],
      //                         &bob.z1[0]));

      //    CU_ASSERT(std::equal(&alice.yb[0],
      //                         &alice.yb[rbytes],
      //                         &bob.z0[0]));
      //  }
      //else
      //  {
      //    CU_ASSERT(pol_equal(alice.skR, bob.skS1));
      //    CU_ASSERT(std::equal(&alice.bskR[0],
      //                         &alice.bskR[bbytes],
      //                         &bob.bskS1[0]));
      //    CU_ASSERT(std::equal(&alice.xb[0],
      //                         &alice.xb[rbytes],
      //                         &bob.w1[0]));
      //    CU_ASSERT(std::equal(&alice.bxb[0],
      //                         &alice.bxb[rbytes],
      //                         &bob.bw1[0]));

      //    CU_ASSERT(std::equal(&alice.xbb[0],
      //                         &alice.xbb[rbytes],
      //                         &bob.w0[0]));
      //    CU_ASSERT(std::equal(&alice.bskRbb[0],
      //                         &alice.bskRbb[bbytes],
      //                         &bob.bskS0[0]));
      //    CU_ASSERT(std::equal(&alice.ybb[0],
      //                         &alice.ybb[rbytes],
      //                         &bob.z0[0]));

      //    CU_ASSERT(std::equal(&alice.yb[0],
      //                         &alice.yb[rbytes],
      //                         &bob.z1[0]));
      //  }
      if (success)
        {
            memcpy(&msg_3b, &msg_3a, sizeof(comm_msg_3_t<rbytes>));
          //{
          //  std::ofstream ofs("msg_3.txt");
          //  boost::archive::text_oarchive oa(ofs);
          //  oa << msg_3a;
          //}

          //{
          //  std::ifstream ifs("msg_3.txt");
          //  boost::archive::text_iarchive ia(ifs);
          //  ia >> msg_3b;
          //}

          success = success && bob.msg2(msg_4a.c0, msg_4a.c1, msg_3b.ch, msg0, msg1);
          msg_4a.sid = msg_3b.sid;
        }
      if (success)
        {
            memcpy(&msg_4b, &msg_4a, sizeof(comm_msg_4_t<cipher_t>));
          //{
          //  std::ofstream ofs("msg_4.txt");
          //  boost::archive::text_oarchive oa(ofs);
          //  oa << msg_4a;
          //}

          //{
          //  std::ifstream ifs("msg_4.txt");
          //  boost::archive::text_iarchive ia(ifs);
          //  ia >> msg_4b;
          //}

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
      //{
      //  std::ofstream ofs("msg_1.txt");
      //  boost::archive::text_oarchive oa(ofs);
      //  oa << msg_1a;
      //}

      //{
      //  std::ifstream ifs("msg_1.txt");
      //  boost::archive::text_iarchive ia(ifs);
      //  ia >> msg_1b;
      //}

      bob.msg1(msg_2a.pS, msg_2a.signal0, msg_2a.signal1,
               msg_2a.u0, msg_2a.u1, msg_2a.a0, msg_2a.a1,
               msg_1b.sid,
               msg_1b.p0, msg_1b.r_sid, m);
      msg_2a.sid = msg_1b.sid;

      memcpy(&msg_2b, &msg_2a, sizeof(comm_msg_2_t<P, rbytes, bbytes, cipher_t>));
      //{
      //  std::ofstream ofs("msg_2.txt");
      //  boost::archive::text_oarchive oa(ofs);
      //  oa << msg_2a;
      //}

      //{
      //  std::ifstream ifs("msg_2.txt");
      //  boost::archive::text_iarchive ia(ifs);
      //  ia >> msg_2b;
      //}

      success = success && alice.msg2(msg_3a.ch, msg_2b.sid, msg_2b.pS,
                                      msg_2b.signal0, msg_2b.signal1,
                                      msg_2b.a0, msg_2b.a1,
                                      msg_2b.u0, msg_2b.u1);
      msg_3a.sid = msg_2b.sid;

      //if (b == 0)
      //  {
      //    CU_ASSERT(pol_equal(alice.skR, bob.skS0));
      //    CU_ASSERT(std::equal(&alice.bskR[0],
      //                         &alice.bskR[bbytes],
      //                         &bob.bskS0[0]));
      //    CU_ASSERT(std::equal(&alice.xb[0],
      //                         &alice.xb[rbytes],
      //                         &bob.w0[0]));
      //    CU_ASSERT(std::equal(&alice.bxb[0],
      //                         &alice.bxb[rbytes],
      //                         &bob.bw0[0]));

      //    CU_ASSERT(std::equal(&alice.xbb[0],
      //                         &alice.xbb[rbytes],
      //                         &bob.w1[0]));
      //    CU_ASSERT(std::equal(&alice.bskRbb[0],
      //                         &alice.bskRbb[bbytes],
      //                         &bob.bskS1[0]));
      //    CU_ASSERT(std::equal(&alice.ybb[0],
      //                         &alice.ybb[rbytes],
      //                         &bob.z1[0]));

      //    CU_ASSERT(std::equal(&alice.yb[0],
      //                         &alice.yb[rbytes],
      //                         &bob.z0[0]));
      //  }
      //else
      //  {
      //    CU_ASSERT(pol_equal(alice.skR, bob.skS1));
      //    CU_ASSERT(std::equal(&alice.bskR[0],
      //                         &alice.bskR[bbytes],
      //                         &bob.bskS1[0]));
      //    CU_ASSERT(std::equal(&alice.xb[0],
      //                         &alice.xb[rbytes],
      //                         &bob.w1[0]));
      //    CU_ASSERT(std::equal(&alice.bxb[0],
      //                         &alice.bxb[rbytes],
      //                         &bob.bw1[0]));

      //    CU_ASSERT(std::equal(&alice.xbb[0],
      //                         &alice.xbb[rbytes],
      //                         &bob.w0[0]));
      //    CU_ASSERT(std::equal(&alice.bskRbb[0],
      //                         &alice.bskRbb[bbytes],
      //                         &bob.bskS0[0]));
      //    CU_ASSERT(std::equal(&alice.ybb[0],
      //                         &alice.ybb[rbytes],
      //                         &bob.z0[0]));

      //    CU_ASSERT(std::equal(&alice.yb[0],
      //                         &alice.yb[rbytes],
      //                         &bob.z1[0]));
      //  }
      if (success)
        {
            memcpy(&msg_3b, &msg_3a, sizeof(comm_msg_3_t<rbytes>));
          //{
          //  std::ofstream ofs("msg_3.txt");
          //  boost::archive::text_oarchive oa(ofs);
          //  oa << msg_3a;
          //}

          //{
          //  std::ifstream ifs("msg_3.txt");
          //  boost::archive::text_iarchive ia(ifs);
          //  ia >> msg_3b;
          //}

          success = success && bob.msg2(msg_4a.c0, msg_4a.c1, msg_3b.ch, msg0, msg1);
          msg_4a.sid = msg_3b.sid;
        }
      if (success)
        {
            memcpy(&msg_4b, &msg_4a, sizeof(comm_msg_4_t<cipher_t>));
          //{
          //  std::ofstream ofs("msg_4.txt");
          //  boost::archive::text_oarchive oa(ofs);
          //  oa << msg_4a;
          //}

          //{
          //  std::ifstream ifs("msg_4.txt");
          //  boost::archive::text_iarchive ia(ifs);
          //  ia >> msg_4b;
          //}

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
      //{
      //  std::ofstream ofs("msg_1.txt");
      //  boost::archive::text_oarchive oa(ofs);
      //  oa << msg_1a;
      //}

      //{
      //  std::ifstream ifs("msg_1.txt");
      //  boost::archive::text_iarchive ia(ifs);
      //  ia >> msg_1b;
      //}

      bob.msg1(msg_2a.pS, msg_2a.signal0, msg_2a.signal1,
               msg_2a.u, msg_2a.hma0, msg_2a.hma1,
               msg_1b.sid,
           msg_1b.hS0, msg_1b.hS1,
               msg_1b.p0, msg_1b.r_sid, m);
      msg_2a.sid = msg_1b.sid;

      memcpy(&msg_2b, &msg_2a, sizeof(comm_rot_msg_2_t<P, bbytes, HASHSIZE>));
      //{
      //  std::ofstream ofs("msg_2.txt");
      //  boost::archive::text_oarchive oa(ofs);
      //  oa << msg_2a;
      //}

      //{
      //  std::ifstream ifs("msg_2.txt");
      //  boost::archive::text_iarchive ia(ifs);
      //  ia >> msg_2b;
      //}

      success = success && alice.msg2(msgb, b, msg_3a.S0, msg_3a.S1,
                      msg_2b.sid, msg_2b.pS,
                                      msg_2b.signal0, msg_2b.signal1,
                                      msg_2b.hma0, msg_2b.hma1,
                                      msg_2b.u);
      msg_3a.sid = msg_2b.sid;

      //if (alice.b1 == 0)
      //  {
      //    CU_ASSERT(pol_equal(alice.skR, bob.skS0));
      //  }
      //else
      //  {
      //    CU_ASSERT(pol_equal(alice.skR, bob.skS1));
      //  }
      if (success)
        {
            memcpy(&msg_3b, &msg_3a, sizeof(comm_rot_msg_3_t<bbytes>));
          //{
          //  std::ofstream ofs("msg_3.txt");
          //  boost::archive::text_oarchive oa(ofs);
          //  oa << msg_3a;
          //}

          //{
          //  std::ifstream ifs("msg_3.txt");
          //  boost::archive::text_iarchive ia(ifs);
          //  ia >> msg_3b;
          //}

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

void rot_test()
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
      P p0, pS, signal0, signal1;
      uint8_t u[bbytes];
      uint8_t r_sid[sizeof(sid) + rbytes];
      uint8_t a0[HASHSIZE], a1[HASHSIZE];
      uint8_t msg0[HASHSIZE], msg1[HASHSIZE], msgb[HASHSIZE];
      uint8_t hS0[HASHSIZE];
      uint8_t hS1[HASHSIZE];
      uint8_t S0[bbytes];
      uint8_t S1[bbytes];
      int b;

      bool success = true;
      alice.msg1(p0, r_sid, hS0, hS1, sid, m);
      bob.msg1(pS, signal0, signal1,
               u, a0, a1,
               sid,
           hS0, hS1,
               p0, r_sid, m);
      success = success && alice.msg2(msgb, b, S0, S1,
                      sid, pS,
                                      signal0, signal1,
                                      a0, a1,
                                      u);
      //if (alice.b1 == 0)
      //  {
      //    CU_ASSERT(pol_equal(alice.skR, bob.skS0));
      //  }
      //else
      //  {
      //    CU_ASSERT(pol_equal(alice.skR, bob.skS1));
      //  }
      if (success)
        {
          success = success && bob.msg2(msg0, msg1, sid, S0, S1);
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
      //CU_ASSERT(success);
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

      uint8_t iv[16];
      memset(iv, 0, sizeof(uint8_t) * 16);

      symenc.SEncIV(c, in, r, key, iv);
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

  //CU_pSuite suite2 = CU_add_suite("RLWEOT", NULL, NULL);
  //if (suite2 == NULL) abort();

  //if ((NULL == CU_add_test(suite2, "rot_test", ot_test)))
  //  {
  //    abort();
  //  }

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
