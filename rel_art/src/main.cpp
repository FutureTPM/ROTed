#include <CUnit/Basic.h>
#include <iostream>
#include <stdlib.h>
#include "ddhot.hpp"
#include "ddhcry.hpp"
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int.hpp>
#include <boost/random/variate_generator.hpp>
#include "cpucycles.h"
#include "blake3.h"

void blake3(uint8_t *out, const uint8_t *in, size_t len)
{
  blake3_hasher hasher;
  blake3_hasher_init(&hasher);
  blake3_hasher_update(&hasher, in, len);
  blake3_hasher_finalize(&hasher, out, BLAKE3_OUT_LEN);
}

void ddhot_test()
{
  const size_t numtests = 1000;
  ec_params_t params(NID_X9_62_prime256v1);
  crs_t crs(params, 32);

  EC_POINT *g, *h, *u, *v, *u1, *v1;
  int b;
  BIGNUM *m, *m1, *m2;

  boost::mt19937 rng;
  boost::uniform_int<> dist(0,1);
  boost::variate_generator<boost::mt19937&, boost::uniform_int<> >
      bit(rng, dist);

  g = EC_POINT_new(params.group);
  h = EC_POINT_new(params.group);
  u = EC_POINT_new(params.group);
  v = EC_POINT_new(params.group);
  u1 = EC_POINT_new(params.group);
  v1 = EC_POINT_new(params.group);
  m = BN_new();
  m1 = BN_new();
  m2 = BN_new();

  //uint8_t dump[4096];
  //BN_CTX* dump2 = BN_CTX_new();

  for (size_t i = 0; i < numtests; i++) {
      //long long start = cpucycles_amd64cpuinfo();
      alice_ot_t alice(crs);
      bob_ot_t bob(crs);
      BN_rand(m, 32, BN_RAND_TOP_ANY, BN_RAND_BOTTOM_ANY);
      BN_rand(m1, 32, BN_RAND_TOP_ANY, BN_RAND_BOTTOM_ANY);

      b = bit();

      alice.msg1(g, h, b);
      bob.msg1(u, v,
              u1, v1,
              g, h,
              m, m1);
      alice.msg2(m2,
              u, v,
              u1, v1);

      BIGNUM *tmp = nullptr;
      if (b == 0) {
          tmp = m;
      } else {
          tmp = m1;
      }
      BN_sub(m2, m2, tmp);

      //CU_ASSERT(BN_is_zero(m2));
      //long long end = cpucycles_amd64cpuinfo();
      //printf("Clock cycles elapsed: %lld\n", end - start);
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

void ddhrot_test()
{
  const size_t numtests = 1000;
  ec_params_t params(NID_X9_62_prime256v1);
  crs_t crs(params, 32);

  EC_POINT *g, *h, *u, *v, *u1, *v1;
  int b, c_sender;
  uint8_t r0_receiver[32], r1_receiver[32], r_sender[32];
  uint8_t r0_receiver_hash[32], r1_receiver_hash[32], c_pipe_r_sender_hash[32], c_pipe_r_receiver_hash[32], c_pipe_r_receiver_hash_generated[32];
  uint8_t r0_sender_hash[32], r1_sender_hash[32];
  BIGNUM *m, *m1, *m2, *check;

  boost::mt19937 rng;
  boost::uniform_int<> dist(0,1);
  boost::variate_generator<boost::mt19937&, boost::uniform_int<> >
      bit(rng, dist);

  boost::uniform_int<> dist_char(0,255);
  boost::variate_generator<boost::mt19937&, boost::uniform_int<> >
      byte(rng, dist);

  g = EC_POINT_new(params.group);
  h = EC_POINT_new(params.group);
  u = EC_POINT_new(params.group);
  v = EC_POINT_new(params.group);
  u1 = EC_POINT_new(params.group);
  v1 = EC_POINT_new(params.group);
  m = BN_new();
  m1 = BN_new();
  m2 = BN_new();
  check = BN_new();

  for (size_t i = 0; i < numtests; i++) {
      // Sender
      c_sender = bit();

      // Receiver
      for (size_t j = 0; j < 32; j++) {
          r0_receiver[j] = byte();
          r1_receiver[j] = byte();
      }
      blake3(r0_receiver_hash, r0_receiver, 32);
      blake3(r1_receiver_hash, r1_receiver, 32);

      uint8_t r0_sender_hash[32], r1_sender_hash[32];
      memcpy(r0_sender_hash, r0_receiver_hash, 32);
      memcpy(r1_sender_hash, r1_receiver_hash, 32);

      /*
       * START Standard OT
       */
      //long long start = cpucycles_amd64cpuinfo();
      alice_ot_t alice(crs);
      bob_ot_t bob(crs);
      BN_rand(m, 32, BN_RAND_TOP_ANY, BN_RAND_BOTTOM_ANY);
      BN_rand(m1, 32, BN_RAND_TOP_ANY, BN_RAND_BOTTOM_ANY);

      b = bit();

      alice.msg1(g, h, b);
      bob.msg1(u, v,
              u1, v1,
              g, h,
              m, m1);
      alice.msg2(m2,
              u, v,
              u1, v1);

      BIGNUM *tmp = nullptr;
      if (b == 0) {
          tmp = m;
      } else {
          tmp = m1;
      }
      BN_sub(check, m2, tmp);

      CU_ASSERT(BN_is_zero(check));
      //long long end = cpucycles_amd64cpuinfo();
      //printf("Clock cycles elapsed: %lld\n", end - start);
      /*
       * END Standard OT
       */
      int m_bin_size = BN_num_bytes(m);
      int m1_bin_size = BN_num_bytes(m1);

      CU_ASSERT(m_bin_size <= 32);
      CU_ASSERT(m1_bin_size <= 32);

      // Sender
      // Receive r0 and r1
      uint8_t r0_sender[32], r1_sender[32];
      memcpy(r0_sender, r0_receiver, 32);
      memcpy(r1_sender, r1_receiver, 32);

      uint8_t rc_Mc_xor_result[32], other_rc_Mc_xor_result[32];
      uint8_t m_bin_sender[32], m1_bin_sender[32];

      blake3(r0_sender_hash, r0_sender, 32);
      blake3(r1_sender_hash, r1_sender, 32);

      bool r0_all_equal = true;
      bool r1_all_equal = true;
      for (size_t j = 0; j < 32; j++) {
          r0_all_equal &= (r0_sender_hash[j] == r0_receiver_hash[j]);
          r1_all_equal &= (r1_sender_hash[j] == r1_receiver_hash[j]);

          m_bin_sender[j] = 0;
          m1_bin_sender[j] = 0;
      }
      CU_ASSERT(r0_all_equal);
      CU_ASSERT(r1_all_equal);

      BN_bn2bin(m, m_bin_sender);
      BN_bn2bin(m1, m1_bin_sender);

      if (c_sender == 0) {
          for (size_t j = 0; j < 32; j++) {
              rc_Mc_xor_result[j] = r0_sender[j] ^ m_bin_sender[j];
              other_rc_Mc_xor_result[j] = r1_sender[j] ^ m1_bin_sender[j];
          }
      } else {
          for (size_t j = 0; j < 32; j++) {
              rc_Mc_xor_result[j] = r1_sender[j] ^ m1_bin_sender[j];
              other_rc_Mc_xor_result[j] = r0_sender[j] ^ m_bin_sender[j];
          }
      }

      // r_{c ^ 0} ^ M_{c ^ 0}
      // r_{c ^ 1} ^ M_{c ^ 1}

      // Receiver
      int c_xor_b_result;
      uint8_t rb_Mb_xor_result[32], m2_bin_receiver[32];

      int c_receiver;
      memcpy(&c_receiver, &c_sender, 4);

      memset(m2_bin_receiver, 0x00, 32);
      BN_bn2bin(m2, m2_bin_receiver);

      // c ^ b
      c_xor_b_result = c_receiver ^ b;
      // r_b ^ M_b
      if (b == 0) {
          for (size_t j = 0; j < 32; j++) {
              rb_Mb_xor_result[j] = r0_receiver[j] ^ m2_bin_receiver[j];
          }
      } else {
          for (size_t j = 0; j < 32; j++) {
              rb_Mb_xor_result[j] = r1_receiver[j] ^ m2_bin_receiver[j];
          }
      }

      // Final Check
      bool m_all_equal = true;
      if (c_xor_b_result == 0) {
          for (size_t j = 0; j < 32; j++) {
              m_all_equal &= (rb_Mb_xor_result[j] == rc_Mc_xor_result[j]);
          }
      } else {
          for (size_t j = 0; j < 32; j++) {
              m_all_equal &= (rb_Mb_xor_result[j] == other_rc_Mc_xor_result[j]);
          }
      }
      CU_ASSERT(m_all_equal);
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
  BN_free(check);
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

  //CU_pSuite suite = CU_add_suite("DDHCRY", NULL, NULL);
  //if (suite == NULL) abort();

  //if ((NULL == CU_add_test(suite, "ddhcry", ddhcry_test)))
  //  {
  //    abort();
  //  }

#ifdef OT_TEST
  CU_pSuite suite1 = CU_add_suite("DDHOT", NULL, NULL);
  if (suite1 == NULL) abort();

  if ((NULL == CU_add_test(suite1, "ddhot", ddhot_test)))
    {
      abort();
    }
#else
  CU_pSuite suite2 = CU_add_suite("DDHROT", NULL, NULL);
  if (suite2 == NULL) abort();

  if ((NULL == CU_add_test(suite2, "ddhrot", ddhrot_test)))
    {
      abort();
    }
#endif

  CU_basic_set_mode(CU_BRM_VERBOSE);
  CU_basic_run_tests();
  CU_cleanup_registry();
  return CU_get_error();
}
