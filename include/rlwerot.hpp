#ifndef __RLWEROT_HPP__
#define __RLWEROT_HPP__
#include <iostream>
#include <algorithm>
#include <iterator>
#include "rlweke.hpp"
#include "roms.hpp"
#include <cstring>
#include "symenc.hpp"
#include "macros.hpp"
#include "rlweot.hpp"

/** Outputs a random bit using NFL random byte generator

    @return Random bit */
int random_bit()
{
  uint8_t b;
  nfl::fastrandombytes(&b, 1);
  return b & 1;
}

/** Produces a hash of the inputted polynomial

    @param pol Inputted polynomial
    @param out Hash of the polynomial
    @tparam P NFL Polynomial type
*/
template<typename P>
void hash_polynomial(P &pol, uint8_t* out)
{
  blake3(out, (const uint8_t*)pol.get_coeffs(), pol.get_coeffs_size_bytes());
}

/** Implements Alice of the Proposed ROT

    @tparam P NFL Polynomial type
    @tparam rbytes Size of random value r
    @tparam bbytes Size of random masks
    @tparam HASHSIZE Size of random oracle
*/ 
template<typename P, size_t rbytes, size_t bbytes, size_t HASHSIZE>
struct alice_rot_t
{
  /**@{*/
  /** Used for RLWE sampling */
  P sR, eR, eR1;
  /**@}*/
  /** Polynomial output of random oracle such that p0 + p1 = h */
  P h;
  /** Random OT channel */
  int b1;
  /**@{*/
  /** Used for reconciliation with Sender's RLWE sample */
  P kR, skR;
  /**@}*/
  
  /** Type of polynomial coefficients */
  using value_t = typename P::value_type;
  /** Gaussian Noise Sampler */
  nfl::FastGaussianNoise<uint8_t, value_t, 2>* g_prng;
  static_assert(HASHSIZE == 32); //since we are using Blake3

  /**@{*/
  /** Auxiliary structures for the implementation of Random Oracles */
  rom1_t<P> rom1;
  rom_P_O<P> rom1_output;

  rom2_t<HASHSIZE> romSi;
  uint8_t S0[bbytes];
  uint8_t S1[bbytes];

  rom2_t<HASHSIZE> romMc;
  uint8_t romMc_output[HASHSIZE];
  uint8_t hMc[HASHSIZE];

   rom2_t<HASHSIZE> romFinalM;
  /**@}*/

  /** Constructor of Alice

      @param _g_prng Gaussian Noise sampler */
  alice_rot_t(nfl::FastGaussianNoise<uint8_t, value_t, 2> *_g_prng)
    : g_prng(_g_prng),
      rom1_output(h)
  {
  }

  /** Implements first Alice message in proposed ROT

      @param p0 Return Alice RLWE sample for "channel 0"
      @param r_sid Concatenation of Session ID and random value of size 'rbytes'
      @param hS0 Commitment to mask of channel 0
      @param hS1 Commitment to mask of channel 1
      @param sid Session ID
      @param m Common polynomial */
  void msg1(P &p0, uint8_t *r_sid,
	    uint8_t hS0[HASHSIZE], uint8_t hS1[HASHSIZE],
	    uint32_t sid, const P &m)
  {
    b1 = random_bit();

    sR = nfl::gaussian<uint8_t, value_t, 2>(g_prng);
    eR = nfl::gaussian<uint8_t, value_t, 2>(g_prng, 2);
    eR1 = nfl::gaussian<uint8_t, value_t, 2>(g_prng, 2);
    sR.ntt_pow_phi();
    eR.ntt_pow_phi();
    p0 = m * sR + eR;

    memcpy(&r_sid[0], &sid, sizeof(sid));
    nfl::fastrandombytes(&r_sid[sizeof(sid)], rbytes);

    nfl::fastrandombytes(&S0[0], sizeof(S0));
    nfl::fastrandombytes(&S1[0], sizeof(S1));

    blake3(&hS0[0], &S0[0], sizeof(S0));
    blake3(&hS1[0], &S1[0], sizeof(S1));

    if (b1 == 1)
      {
        rom1(rom1_output, r_sid, rbytes + sizeof(uint32_t));
        p0 = p0 - h;
      }
  }

  /** Implements second Alice message in proposed ROT

      @param Mb Outputted message
      @param b Outputted channel
      @param bS0 Mask for channel 0
      @param bS1 Mask for channel 1
      @param sid Session ID
      @param pS Sender's RLWE sample
      @param signal0 Hint signal for key exchange in channel 0
      @param signal1 Hint signal for key exchange in channel 1
      @param ha0 Commitment to key in channel 0 of KE
      @param ha1 Commitment to key in channel 1 of KE
      @param u Sender's mask
      @return Returns true when all checks are successful */  
  bool msg2(uint8_t Mb[HASHSIZE],
	    int &b,
	    uint8_t bS0[bbytes], uint8_t bS1[bbytes],
	    uint32_t sid, const P &pS, const P &signal0, const P &signal1,
            const uint8_t ha0[HASHSIZE], const uint8_t ha1[HASHSIZE],
            const uint8_t u[bbytes])
  {
    kR = pS * sR;
    kR.invntt_pow_invphi();
    kR = kR + eR1;

    if (b1 == 0)
      {
        ke_t<P>::mod2(skR, kR, signal0);
      }
    else
      {
        ke_t<P>::mod2(skR, kR, signal1);
      }

    hash_polynomial(skR, romMc_output);
    memcpy(hMc, romMc_output, sizeof(uint8_t) * HASHSIZE);

    if (memcmp(ha0, hMc, HASHSIZE) == 0) {
      b = 0;
    } else if (memcmp(ha1, hMc, HASHSIZE) == 0) {
      b = 1;
    } else {
      return false;
    }

    int sid_size = sizeof(sid);
    uint8_t Mb_sid[sid_size + bbytes];
    memcpy(&Mb_sid[0], &sid, sid_size);
    
    convPtoArray<P, bbytes>(&Mb_sid[sid_size], skR);

    if (b1 == 0)
      {
	for (int i = 0; i < bbytes; i++)
	  {
	    Mb_sid[sid_size + i] ^= S0[i] ^ u[i];
	  }
      }
    else
      {
	for (int i = 0; i < bbytes; i++)
	  {
	    Mb_sid[sid_size + i] ^= S1[i] ^ u[i];
	  }
      }

    blake3(Mb, &Mb_sid[0], sizeof(Mb_sid));
    
    memcpy(bS0, S0, bbytes);
    memcpy(bS1, S1, bbytes);
    return true;
  }
};

/** Implements Bob of the Proposed ROT

    @tparam P NFL Polynomial type
    @tparam rbytes Size of random value r
    @tparam bbytes Size of random masks
    @tparam HASHSIZE Size of random oracle
*/ 
template<typename P, size_t rbytes, size_t bbytes, size_t HASHSIZE>
struct bob_rot_t
{
  static_assert(HASHSIZE == 32); //since we are using BLAKE
  
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
  /** Keys shared under base KE */
  P skS0, skS1;
  /**@}*/
  /** Random flipping of channels */
  int a1;

  /**@{*/
  /** Commitments to receiver's masks */
  uint8_t hS0[HASHSIZE];
  uint8_t hS1[HASHSIZE];
  uint8_t hS0b[HASHSIZE];
  uint8_t hS1b[HASHSIZE];
  /**@}*/

  /** Random mask */
  uint8_t u[bbytes];

  /** Coefficient type */
  using value_t = typename P::value_type;
  /** Gaussian noise sampler */
  nfl::FastGaussianNoise<uint8_t, value_t, 2>* g_prng;

  /**@{*/
  /** Auxiliary structures for the implementation of Random Oracles */
  rom1_t<P> rom1;
  rom_P_O<P> rom1_output;

  rom2_t<HASHSIZE> romM;

  rom_k_O<HASHSIZE> romhS0b_output;
  rom_k_O<HASHSIZE> romhS1b_output;
  /**@}*/

  /** Constructor of Bob

      @param _g_prng Gaussian Noise sampler */
  bob_rot_t(nfl::FastGaussianNoise<uint8_t, value_t, 2> *_g_prng)
    : g_prng(_g_prng),
      rom1_output(h),
      romhS0b_output(hS0b),
      romhS1b_output(hS1b)
  {
  }

  /** Implements first Bob message in proposed OT

      @param pS Bob's RLWE sample
      @param signal0 Outputted hint signal for "channel 0"
      @param signal1 Outputted hint signal for "channel 1"
      @param au Random mask
      @param hma0 Commitment to shared secret under one of the KE channels
      @param hma1 Commitment to shared secret under one of the KE channels
      @param sid Session ID
      @param hS0a Commitment to Alice's random mask 0
      @param hS1a Commitment to Alice's random mask 1
      @param p0 Alice RLWE sample for "channel 0"
      @param r_sid Concatenation of Session ID and random value of size 'rbytes'
      @param m Common polynomial */
  void msg1(P &pS, P &signal0, P &signal1,
	    uint8_t au[bbytes],
            uint8_t hma0[HASHSIZE], uint8_t hma1[HASHSIZE],
            uint32_t sid,
	    const uint8_t hS0a[HASHSIZE],
	    const uint8_t hS1a[HASHSIZE],
            const P &p0, const uint8_t *r_sid, const P &m)
  {
    memcpy(&hS0[0], &hS0a[0], sizeof(hS0));
    memcpy(&hS1[0], &hS1a[0], sizeof(hS1));

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

    kS1 = p1 * sS;
    kS1.invntt_pow_invphi();
    kS1 = kS1 + eS1;

    ke_t<P>::signal(signal0, kS0, ke_t<P>::random_bit());
    ke_t<P>::signal(signal1, kS1, ke_t<P>::random_bit());

    ke_t<P>::mod2(skS0, kS0, signal0);
    ke_t<P>::mod2(skS1, kS1, signal1);

    a1 = random_bit();

    nfl::fastrandombytes(&u[0], sizeof(u));

    memcpy(au, u, bbytes);

    if (a1 == 0) {
      uint8_t romMa0_output[HASHSIZE];
      uint8_t romMa1_output[HASHSIZE];
      hash_polynomial(skS0, romMa0_output);
      hash_polynomial(skS1, romMa1_output);
      memcpy(hma0, romMa0_output, HASHSIZE * sizeof(uint8_t));
      memcpy(hma1, romMa1_output, HASHSIZE * sizeof(uint8_t));
    } else {
      uint8_t romMa0_output[HASHSIZE];
      uint8_t romMa1_output[HASHSIZE];
      hash_polynomial(skS0, romMa0_output);
      hash_polynomial(skS1, romMa1_output);
      memcpy(hma1, romMa0_output, HASHSIZE * sizeof(uint8_t));
      memcpy(hma0, romMa1_output, HASHSIZE * sizeof(uint8_t));
    }
  }

  /** Implements second Bob message in proposed ROT

      @param msg0 Returned message in channel 0
      @param msg1 Returned message in channel 1
      @param sid Session ID
      @param S0 Alice's mask 0
      @param S1 Alice's mask 1
      @return Returns true if opening of masks is successful
  */
  bool msg2(uint8_t msg0[HASHSIZE], uint8_t msg1[HASHSIZE],
	    uint32_t sid,
	    const uint8_t S0[bbytes], const uint8_t S1[bbytes])
  {
    blake3(&hS0b[0], &S0[0], bbytes);
    blake3(&hS1b[0], &S1[0], bbytes);

    if ((memcmp(hS0b, hS0, bbytes) != 0) ||
	(memcmp(hS1b, hS1, bbytes) != 0))
      {
	return false;
      }

    int sid_size = sizeof(sid);
    uint8_t msg0_sid[sid_size + bbytes];
    uint8_t msg1_sid[sid_size + bbytes];

    memcpy(&msg0_sid[0], &sid, sid_size);
    memcpy(&msg1_sid[0], &sid, sid_size);
    
    if (a1 == 0)
      {
	convPtoArray<P, bbytes>(&msg0_sid[sid_size], skS0);
	convPtoArray<P, bbytes>(&msg1_sid[sid_size], skS1);

	for (int i = 0; i < bbytes; i++)
	  {
	    msg0_sid[i + sid_size] ^= S0[i] ^ u[i];
	    msg1_sid[i + sid_size] ^= S1[i] ^ u[i];
	  }
      }
    else
      {
	convPtoArray<P, bbytes>(&msg0_sid[sid_size], skS1);
	convPtoArray<P, bbytes>(&msg1_sid[sid_size], skS0);

	for (int i = 0; i < bbytes; i++)
	  {
	    msg0_sid[i + sid_size] ^= S1[i] ^ u[i];
	    msg1_sid[i + sid_size] ^= S0[i] ^ u[i];
	  }
      }
    
    blake3(msg0, &msg0_sid[0], sizeof(msg0_sid));
    blake3(msg1, &msg1_sid[0], sizeof(msg1_sid));

    return true;
  }
};

#endif
