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

/** Converts binary polynomial into array. If size of the array is shorter
    that polynomial degree, coefficients are hashed

    @tparam P NFL Polynomial type
    @tparam bbytes Size of the array
    @param arr Returned array
    @param P Input polynomial
*/
template<typename P, size_t bbytes>
void convPtoArray(uint8_t arr[bbytes], const P &pol)
{
  if (bbytes*8 > P::degree)
    {
      uint8_t coeffs[CEILING(P::degree, 8)];
      memset(coeffs, 0, sizeof(coeffs));

      for (size_t i = 0; i < P::degree; i++)
	{
	  size_t iLower = i & 7;
	  coeffs[i / 8] |= pol(0, i) << iLower;
	}

      rom2_t<bbytes> rom2;
      rom_k_O<bbytes> rom2_output(arr);
      rom2(rom2_output, coeffs, sizeof(coeffs));
    }
  else
    {
      memset(arr, 0, bbytes);

      for (size_t i = 0; i < bbytes * 8; i++)
	{
	  size_t iLower = i & 7;
	  arr[i / 8] |= pol(0, i) << iLower;
	}      
    }
}

/** Implements Alice in the OT of [BDGM19]

[BDGM19] Pedro Branco, Jintai Ding, Manuel Goulao, and Paulo Mateus.
A frameworkfor universally composable oblivious transfer from one-round key-exchange.
In Martin Albrecht, editor, Cryptography and Coding, pages 78–101, Cham,2019.
Springer International Publishing

@tparam P NFL Polynomial type
@tparam rbytes Size of symmetric cipher key
@tparam bbytes Size of output of random oracle 
*/
template<typename P, size_t rbytes, size_t bbytes>
struct alice_ot_t
{
  /** Symmetric-key encryption engine */
  sym_enc_t<rbytes, rbytes, bbytes> sym_enc;
  /** Type of symmetric key cipher */
  typedef typename sym_enc_t<rbytes, rbytes, bbytes>::cipher_t cipher_t;

  /**@{*/
  /** Used for RLWE sampling */
  P sR, eR, eR1;
  /**@}*/
  /** Polynomial output of random oracle such that p0 + p1 = h */
  P h;
  /** Chosen OT channel */
  int b;
  /**@{*/
  /** Used for reconciliation with Sender's RLWE sample */
  P kR, skR;
  /**@}*/
  /**@{*/
  /** Used to compute challenge response */
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
  /**@}*/

  /** Type of polynomial coefficients */
  using value_t = typename P::value_type;
  /** Gaussian Noise Sampler */
  nfl::FastGaussianNoise<uint8_t, value_t, 2> *g_prng;

  /**@{*/
  /** Auxiliary structures for the implementation of Random Oracles */
  rom1_t<P> rom1;
  rom_P_O<P> rom1_output;
  rom2_t<bbytes> rom2;
  rom_k_O<bbytes> rom2_output;
  rom3_t<rbytes, bbytes> rom3;
  rom_k_O<2*rbytes + bbytes> rom3_output0;
  rom_k_O<2*rbytes + bbytes> rom3_output1;
  rom4_t<rbytes> rom4;
  /**@}*/

  /** Constructor of Alice

      @param _g_prng Gaussian Noise sampler */
  alice_ot_t(nfl::FastGaussianNoise<uint8_t, value_t, 2> *_g_prng)
    : g_prng(_g_prng),
      rom1_output(h),
      rom2_output(bskR),
      rom3_output0(bxb),
      rom3_output1(bxbb)
  {
  }

  /** Implements first Alice message in [BDGM19]

      @param p0 Return Alice RLWE sample for "channel 0"
      @param r_sid Concatenation of Session ID and random value of size 'rbytes'
      @param b1 Chosen OT channel
      @param sid Session ID
      @param m Common polynomial */
  void msg1(P &p0, uint8_t *r_sid, int b1, uint32_t sid, const P &m)
  {
    b = b1;
    sR = nfl::gaussian<uint8_t, value_t, 2>(g_prng);
    eR = nfl::gaussian<uint8_t, value_t, 2>(g_prng, 2);
    eR1 = nfl::gaussian<uint8_t, value_t, 2>(g_prng, 2);
    sR.ntt_pow_phi();
    eR.ntt_pow_phi();
    p0 = m * sR + eR;

    memcpy(&r_sid[0], &sid, sizeof(sid));
    nfl::fastrandombytes(&r_sid[sizeof(sid)], rbytes);

    if (b == 1) {
        rom1(rom1_output, r_sid, rbytes + sizeof(uint32_t));
        p0 = p0 - h;
    }
  }

  /** Implements second Alice message in [BDGM19]

      @param ch Returned challenge response
      @param sid Session ID
      @param pS Sender's RLWE sample
      @param signal0 Hint signal for key exchange in channel 0
      @param signal1 Hint signal for key exchange in channel 1
      @param a0 Part of the challenge
      @param a1 Part of the challenge
      @param u0 Part of the challenge
      @param u1 Part of the challenge
      @return Returns true when all checks are successful */
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
	sym_enc.SEnc(abb, xbb, ybb, bskRbb);
	if (memcmp(abb.buf, a1.buf, sizeof(abb.buf)) != 0)
	  return false;
      }
    else
      {
	sym_enc.SEnc(abb, xbb, ybb, bskRbb);
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
	sym_enc.SEnc(ab, xb, yb, bskRb);
	if (memcmp(ab.buf, a0.buf, sizeof(ab.buf)) != 0)
	  return false;
      }
    else
      {
	sym_enc.SEnc(ab, xb, yb, bskRb);
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

  /** Implements third Alice message in [BDGM19]

      @param msgb Returned message
      @param c0 Cipher for message in channel 0
      @param c1 Cipher for message in channel 1
  */
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

/** Implements Bob in the OT of [BDGM19]

[BDGM19] Pedro Branco, Jintai Ding, Manuel Goulao, and Paulo Mateus.
A frameworkfor universally composable oblivious transfer from one-round key-exchange.
In Martin Albrecht, editor, Cryptography and Coding, pages 78–101, Cham,2019.
Springer International Publishing

@tparam P NFL Polynomial type
@tparam rbytes Size of symmetric cipher key
@tparam bbytes Size of output of random oracle 
*/
template<typename P, size_t rbytes, size_t bbytes>
struct bob_ot_t
{
  /**@{*/
  /** Used for RLWE sampling */
  P sS, eS, eS1;
  /**@}*/
  /** Polynomial output of random oracle such that p0 + p1 = h */
  P h;
  /** p1 corresponding to Alice's RLWE sample on channel 1 */
  P p1;
  
  /**@{*/
  /** Used for RLWE key exchange in channel 0,1 */
  P kS0, kS1;
  /**@}*/
  /**@{*/
  /** Hint signals */
  P signal0, signal1;
  /**@}*/
  
  /**@{*/
  /** Used to produce challenge */
  P skS0, skS1;
  uint8_t w0[rbytes], w1[rbytes];
  uint8_t z0[rbytes], z1[rbytes];
  uint8_t bskS0[bbytes], bskS1[bbytes];

  uint8_t bw0[2*rbytes + bbytes], bw1[2*rbytes + bbytes];
  uint8_t ch[rbytes];
  /**@}*/

  /** Polynomial coefficient type */
  using value_t = typename P::value_type;
  /** Gaussian Noise Sampler */
  nfl::FastGaussianNoise<uint8_t, value_t, 2> *g_prng;

  /**@{*/
  /** Auxiliary structures for the implementation of Random Oracles */
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
  /**@}*/

  /** Symmetric-key encryption engine */
  sym_enc_t<rbytes, rbytes, bbytes> sym_enc;
  /** Type of symmetric key cipher */
  typedef typename sym_enc_t<rbytes, rbytes, bbytes>::cipher_t cipher_t;

  /** Constructor of Bob

      @param _g_prng Gaussian Noise sampler */
  bob_ot_t(nfl::FastGaussianNoise<uint8_t, value_t, 2> *_g_prng)
    : g_prng(_g_prng),
      rom1_output(h),
      rom2_output0(bskS0),
      rom2_output1(bskS1),
      rom3_output0(bw0),
      rom3_output1(bw1),
      rom4_output(ch)
  {
  }

  /** Implements first Bob message in [BDGM19]

      @param pS Bob's RLWE sample
      @param signal0 Outputted hint signal for "channel 0"
      @param signal1 Outputted hint signal for "channel 1"
      @param u0 Part of challenge
      @param u1 Part of challenge
      @param a0 Part of challenge
      @param a1 Part of challenge
      @param sid Session ID
      @param p0 Alice RLWE sample for "channel 0"
      @param r_sid Concatenation of Session ID and random value of size 'rbytes'
      @param m Common polynomial */
  void msg1(P &pS, P &signal0, P &signal1,
	    uint8_t u0[2*rbytes + bbytes], uint8_t u1[2*rbytes + bbytes],
	    cipher_t &a0, cipher_t &a1,
	    uint32_t sid,
	    const P &p0, const uint8_t *r_sid, const P &m)
  {
    sS = nfl::gaussian<uint8_t, value_t, 2>(g_prng);
    eS = nfl::gaussian<uint8_t, value_t, 2>(g_prng, 2);
    eS1 = nfl::gaussian<uint8_t, value_t, 2>(g_prng, 2);
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
	    rom2_inputj[sizeof(sid) + (i>>3)] = rom2_inputj[sizeof(sid) + (i>>3)] | (skSj(0, i) << (i & 7));
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

  /** Implements second Bob message in [BDGM19]

      @param c0 Returned cipher for message in channel 0
      @param c1 Returned cipher for message in channel 1
      @param ch1 Alice's challenge response
      @param msg0 Message to be encrypted in channel 0
      @param msg1 Message to be encrypted in channel 1
      @return Returns true if challenge response has expected value
  */
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
