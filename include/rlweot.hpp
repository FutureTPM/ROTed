#ifndef __RLWEOT_HPP__
#define __RLWEOT_HPP__
#include <iostream>
#include <algorithm>
#include <iterator>
#include "rlweke.hpp"
#include "roms.hpp"
#include <cstring>
#include "symenc.hpp"
#include "macros.hpp"

template<typename P, size_t bbytes>
void convPtoArray(uint8_t arr[bbytes], const P &pol)
{
  for (size_t i = 0; i < MIN(bbytes, P::degree); i++)
    {
      arr[i] = pol(0, i);
    }
  for (size_t i = MIN(bbytes, P::degree); i < bbytes; i++)
    {
      arr[0] = 0;
    }
}

template<typename P, size_t rbytes, size_t bbytes>
struct alice_ot_t
{
  sym_enc_t<rbytes, rbytes, bbytes> sym_enc;
  typedef typename sym_enc_t<rbytes, rbytes, bbytes>::cipher_t cipher_t;
  
  P sR, eR, eR1;
  P h;
  int b;
  P kR, skR;
  uint8_t bskR[bbytes];
  uint8_t xb[rbytes];
  uint8_t bxb[2*rbytes + bbytes];

  uint8_t xbb[rbytes];
  uint8_t bskRbb[bbytes];
  uint8_t ybb[rbytes];

  cipher_t abb;
  cipher_t ab;

  uint8_t bxbb[2*rbytes + bbytes];

  uint8_t xb1[rbytes];
  uint8_t bskRb[bbytes];
  uint8_t yb[rbytes];
  
  using value_t = typename P::value_type;
  nfl::FastGaussianNoise<uint8_t, value_t, 2> g_prng;
  
  rom1_t<P> rom1;
  rom_P_O<P> rom1_output;
  rom2_t<bbytes> rom2;
  rom_k_O<bbytes> rom2_output;
  rom3_t<rbytes, bbytes> rom3;
  rom_k_O<2*rbytes + bbytes> rom3_output0;
  rom_k_O<2*rbytes + bbytes> rom3_output1;
  rom4_t<rbytes> rom4;

  alice_ot_t(double sigma, unsigned int security, unsigned int samples)
    : g_prng(sigma, security, samples),
      rom1_output(h),
      rom2_output(bskR),
      rom3_output0(bxb),
      rom3_output1(bxbb)
  {
  }
  
  void msg1(P &p0, uint8_t *r_sid, int b1, uint32_t sid, const P &m)
  {
    b = b1;
    sR = nfl::gaussian<uint8_t, value_t, 2>(&g_prng);
    eR = nfl::gaussian<uint8_t, value_t, 2>(&g_prng, 2);
    eR1 = nfl::gaussian<uint8_t, value_t, 2>(&g_prng, 2);
    sR.ntt_pow_phi();
    eR.ntt_pow_phi();
    p0 = m * sR + eR;
    
    memcpy(&r_sid[0], &sid, sizeof(sid));
    nfl::fastrandombytes(&r_sid[sizeof(sid)], rbytes);
    
    if (b == 1)
      {	
	rom1(rom1_output, r_sid, rbytes + sizeof(uint32_t));
	p0 = p0 - h;
      }
  }

  bool msg2(uint8_t ch[rbytes],
	    uint32_t sid, const P &pS, const P &signal0, const P &signal1,
	    const cipher_t &a0, const cipher_t &a1,
	    const uint8_t u0[2*rbytes + bbytes], const uint8_t u1[2*rbytes + bbytes])
  {
    kR = pS * sR;
    kR.invntt_pow_invphi();
    kR = kR + eR1;

    if (b == 0)
      {
	ke_t<P>::mod2(skR, kR, signal0);
      }
    else
      {
	ke_t<P>::mod2(skR, kR, signal1);
      }

    uint8_t rom2_inputj[sizeof(sid) + CEILING(P::degree, 8)];
    memcpy(&rom2_inputj[0], &sid, sizeof(sid));
    memset(&rom2_inputj[sizeof(sid)], 0, CEILING(P::degree, 8));
    for (size_t i = 0; i < P::degree; i++)
      {
	rom2_inputj[sizeof(sid) + (i>>3)] =
	  rom2_inputj[sizeof(sid) + (i>>3)] |
	  (skR(0, i) << (i & 7));
      }
    
    rom2(rom2_output, rom2_inputj, sizeof(sid) + CEILING(P::degree, 8));

    if (b == 0)
      {
	sym_enc.SDec(xb, a0, bskR);
      }
    else
      {
	sym_enc.SDec(xb, a1, bskR);
      }

    uint8_t rom3_inputj[sizeof(sid) + rbytes];
    memcpy(&rom3_inputj[0], &sid, sizeof(sid));
    memcpy(&rom3_inputj[sizeof(sid)], xb, rbytes);
    rom3(rom3_output0, rom3_inputj, sizeof(sid) + rbytes);

    if (b == 0)
      {
	for (size_t i = 0; i < rbytes; i++)
	  {
	    xbb[i] = u0[i] ^ bxb[i];
	  }
	for (size_t i = 0; i < bbytes; i++)
	  {
	    bskRbb[i] = u0[i + rbytes] ^ bxb[i + rbytes];
	  }
	for (size_t i = 0; i < rbytes; i++)
	  {
	    ybb[i] = u0[i + rbytes + bbytes] ^ bxb[i + rbytes + bbytes];
	  }
      }
    else
      {
	for (size_t i = 0; i < rbytes; i++)
	  {
	    xbb[i] = u1[i] ^ bxb[i];
	  }
	for (size_t i = 0; i < bbytes; i++)
	  {
	    bskRbb[i] = u1[i + rbytes] ^ bxb[i + rbytes];
	  }
	for (size_t i = 0; i < rbytes; i++)
	  {
	    ybb[i] = u1[i + rbytes + bbytes] ^ bxb[i + rbytes + bbytes];
	  }
      }

    if (b == 0)
      {
	sym_enc.SEncIV(abb, xbb, ybb, bskRbb, a1.iv);
	if (memcmp(abb.buf, a1.buf, sizeof(abb.buf)) != 0)
	  return false;
      }
    else
      {
	sym_enc.SEncIV(abb, xbb, ybb, bskRbb, a0.iv);
	if (memcmp(abb.buf, a0.buf, sizeof(abb.buf)) != 0)
	  return false;
      }

    memcpy(&rom3_inputj[0], &sid, sizeof(sid));
    memcpy(&rom3_inputj[sizeof(sid)], xbb, rbytes);
    rom3(rom3_output1, rom3_inputj, sizeof(sid) + rbytes);

    if (b == 0)
      {
	for (size_t i = 0; i < rbytes; i++)
	  {
	    xb1[i] = u1[i] ^ bxbb[i];
	  }
	for (size_t i = 0; i < bbytes; i++)
	  {
	    bskRb[i] = u1[i + rbytes] ^ bxbb[i + rbytes];
	  }
	for (size_t i = 0; i < rbytes; i++)
	  {
	    yb[i] = u1[i + rbytes + bbytes] ^ bxbb[i + rbytes + bbytes];
	  }
      }
    else
      {
	for (size_t i = 0; i < rbytes; i++)
	  {
	    xb1[i] = u0[i] ^ bxbb[i];
	  }
	for (size_t i = 0; i < bbytes; i++)
	  {
	    bskRb[i] = u0[i + rbytes] ^ bxbb[i + rbytes];
	  }
	for (size_t i = 0; i < rbytes; i++)
	  {
	    yb[i] = u0[i + rbytes + bbytes] ^ bxbb[i + rbytes + bbytes];
	  }
      }

    if (memcmp(xb1, xb, rbytes) != 0) return false;
    if (memcmp(bskRb, bskR, bbytes) != 0) return false;

    if (b == 0)
      {
	sym_enc.SEncIV(ab, xb, yb, bskRb, a0.iv);
	if (memcmp(ab.buf, a0.buf, sizeof(ab.buf)) != 0)
	  return false;	
      }
    else
      {
	sym_enc.SEncIV(ab, xb, yb, bskRb, a1.iv);
	if (memcmp(ab.buf, a1.buf, sizeof(ab.buf)) != 0)
	  return false;	
      }

    rom_k_O<rbytes> rom4_output(ch);
    
    uint8_t rom4_input[sizeof(sid) + 4*rbytes];
    memcpy(&rom4_input[0], &sid, sizeof(sid));
    if (b == 0)
      {
	memcpy(&rom4_input[sizeof(sid)], &xb[0], rbytes);
	memcpy(&rom4_input[sizeof(sid) + rbytes], &xbb[0], rbytes);
	memcpy(&rom4_input[sizeof(sid) + 2*rbytes], &yb[0], rbytes);
	memcpy(&rom4_input[sizeof(sid) + 3*rbytes], &ybb[0], rbytes);
      }
    else
      {
	memcpy(&rom4_input[sizeof(sid)], &xbb[0], rbytes);
	memcpy(&rom4_input[sizeof(sid) + rbytes], &xb[0], rbytes);
	memcpy(&rom4_input[sizeof(sid) + 2*rbytes], &ybb[0], rbytes);
	memcpy(&rom4_input[sizeof(sid) + 3*rbytes], &yb[0], rbytes);
      }
    rom4(rom4_output, rom4_input, sizeof(sid) + 4*rbytes);

    return true;
  }

  void msg3(uint8_t msgb[rbytes], const cipher_t &c0, const cipher_t &c1)
  {
    uint8_t kb[bbytes];
    convPtoArray<P, bbytes>(kb, skR);
    if (b == 0)
      {
	sym_enc.SDec(msgb, c0, kb);
      }
    else
      {
	sym_enc.SDec(msgb, c1, kb);
      }

  }
};

template<typename P, size_t rbytes, size_t bbytes>
struct bob_ot_t
{  
  P sS, eS, eS1;
  P h;
  P p1;
  P kS0, kS1;
  P signal0, signal1;
  P skS0, skS1;
  uint8_t w0[rbytes], w1[rbytes];
  uint8_t z0[rbytes], z1[rbytes];
  uint8_t bskS0[bbytes], bskS1[bbytes];

  uint8_t bw0[2*rbytes + bbytes], bw1[2*rbytes + bbytes];

  uint8_t ch[rbytes];
  
  using value_t = typename P::value_type;
  nfl::FastGaussianNoise<uint8_t, value_t, 2> g_prng;

  rom1_t<P> rom1;
  rom_P_O<P> rom1_output;
  rom2_t<bbytes> rom2;
  rom_k_O<bbytes> rom2_output0;
  rom_k_O<bbytes> rom2_output1;
  rom3_t<rbytes, bbytes> rom3;
  rom_k_O<2*rbytes + bbytes> rom3_output0;
  rom_k_O<2*rbytes + bbytes> rom3_output1;
  rom4_t<rbytes> rom4;
  rom_k_O<rbytes> rom4_output;

  sym_enc_t<rbytes, rbytes, bbytes> sym_enc;
  typedef typename sym_enc_t<rbytes, rbytes, bbytes>::cipher_t cipher_t;
  
  bob_ot_t(double sigma, unsigned int security, unsigned int samples)
    : g_prng(sigma, security, samples),
      rom1_output(h),
      rom2_output0(bskS0),
      rom2_output1(bskS1),
      rom3_output0(bw0),
      rom3_output1(bw1),
      rom4_output(ch)
  {
  }

  void msg1(P &pS, P &signal0, P &signal1,
	    uint8_t u0[2*rbytes + bbytes], uint8_t u1[2*rbytes + bbytes],
	    cipher_t &a0, cipher_t &a1,
	    uint32_t sid,
	    const P &p0, const uint8_t *r_sid, const P &m)
  {
    sS = nfl::gaussian<uint8_t, value_t, 2>(&g_prng);
    eS = nfl::gaussian<uint8_t, value_t, 2>(&g_prng, 2);
    eS1 = nfl::gaussian<uint8_t, value_t, 2>(&g_prng, 2);
    sS.ntt_pow_phi();
    eS.ntt_pow_phi();
    pS = m * sS + eS;

    rom1(rom1_output, r_sid, rbytes + sizeof(uint32_t));
    p1 = p0 + h;

    kS0 = p0 * sS;
    kS0.invntt_pow_invphi();
    kS0 = kS0 + eS1;

    kS1 = sS * p1;
    kS1.invntt_pow_invphi();
    kS1 = kS1 + eS1;

    ke_t<P>::signal(signal0, kS0, ke_t<P>::random_bit());
    ke_t<P>::signal(signal1, kS1, ke_t<P>::random_bit());

    ke_t<P>::mod2(skS0, kS0, signal0);
    ke_t<P>::mod2(skS1, kS1, signal1);

    nfl::fastrandombytes(&w0[0], rbytes);
    nfl::fastrandombytes(&w1[0], rbytes);
    nfl::fastrandombytes(&z0[0], rbytes);
    nfl::fastrandombytes(&z1[0], rbytes);

    auto hash_concat = [&] (rom_k_O<bbytes> &out,
			    P &skSj) -> void
      {
	uint8_t rom2_inputj[sizeof(sid) + CEILING(P::degree, 8)];
	memcpy(&rom2_inputj[0], &sid, sizeof(sid));
	memset(&rom2_inputj[sizeof(sid)], 0, CEILING(P::degree, 8));
	for (size_t i = 0; i < P::degree; i++)
	  {
	    rom2_inputj[sizeof(sid) + (i>>3)] =
	      rom2_inputj[sizeof(sid) + (i>>3)] |
	      (skSj(0, i) << (i & 7));
	  }

	rom2(out, rom2_inputj, sizeof(sid) + CEILING(P::degree, 8));
      };

    hash_concat(rom2_output0, skS0);
    hash_concat(rom2_output1, skS1);

    sym_enc.SEnc(a0, w0, z0, bskS0);
    sym_enc.SEnc(a1, w1, z1, bskS1);

    auto hash_concat_2 = [&] (rom_k_O<2*rbytes + bbytes> &out,
			      uint8_t wi[rbytes])
      {
	uint8_t rom3_inputj[sizeof(sid) + rbytes];
	memcpy(&rom3_inputj[0], &sid, sizeof(sid));
	memcpy(&rom3_inputj[sizeof(sid)], wi, rbytes);
	rom3(out, rom3_inputj, sizeof(sid) + rbytes);
      };

    hash_concat_2(rom3_output0, w0);
    hash_concat_2(rom3_output1, w1);

    for (size_t i = 0; i < rbytes; i++)
      {
	u0[i] = bw0[i] ^ w1[i];
	u1[i] = bw1[i] ^ w0[i];
      }
    for (size_t i = 0; i < bbytes; i++)
      {
	u0[rbytes + i] = bw0[rbytes + i] ^ bskS1[i];
	u1[rbytes + i] = bw1[rbytes + i] ^ bskS0[i];
      }
    for (size_t i = 0; i < rbytes; i++)
      {
	u0[rbytes + bbytes + i] = bw0[rbytes + bbytes + i] ^ z1[i];
	u1[rbytes + bbytes + i] = bw1[rbytes + bbytes + i] ^ z0[i];
      }

    uint8_t rom4_input[sizeof(sid) + 4*rbytes];
    memcpy(&rom4_input[0], &sid, sizeof(sid));
    memcpy(&rom4_input[sizeof(sid)], &w0[0], rbytes);
    memcpy(&rom4_input[sizeof(sid) + rbytes], &w1[0], rbytes);
    memcpy(&rom4_input[sizeof(sid) + 2*rbytes], &z0[0], rbytes);
    memcpy(&rom4_input[sizeof(sid) + 3*rbytes], &z1[0], rbytes);

    rom4(rom4_output, rom4_input, sizeof(sid) + 4*rbytes);
  }

  bool msg2(cipher_t &c0, cipher_t &c1, const uint8_t ch1[rbytes],
	    const uint8_t msg0[rbytes], const uint8_t msg1[rbytes])
  {
    if (memcmp(ch, ch1, rbytes) != 0)
      return false;

    uint8_t k0[bbytes], k1[bbytes];
    convPtoArray<P, bbytes>(k0, skS0);
    convPtoArray<P, bbytes>(k1, skS1);
    
    uint8_t z0[rbytes], z1[rbytes];
    nfl::fastrandombytes(&z0[0], rbytes);
    nfl::fastrandombytes(&z1[0], rbytes);

    sym_enc.SEnc(c0, msg0, z0, k0);
    sym_enc.SEnc(c1, msg1, z1, k1);

    return true;
  }
};

#endif
