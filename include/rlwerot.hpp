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

int random_bit()
{
  uint8_t b;
  nfl::fastrandombytes(&b, 1);
  return b & 1;
}

//template<typename P, typename R, typename RO>
template<typename P>
void hash_polynomial(P &pol, uint8_t* out)
{
  // uint8_t buf[P::degree];
  // constexpr size_t len = sizeof(buf);
  // convPtoArray<P, len>(buf, pol);
  // rom(rom_output, buf, len);

  blake3(out, (const uint8_t*)pol.get_coeffs(), pol.get_coeffs_size_bytes());
}

template<typename P, size_t rbytes, size_t bbytes, size_t HASHSIZE>
struct alice_rot_t
{
  P sR, eR, eR1;
  P h;
  int b1;
  P kR, skR;

  using value_t = typename P::value_type;
  nfl::FastGaussianNoise<uint8_t, value_t, 2>* g_prng;

  rom1_t<P> rom1;
  rom_P_O<P> rom1_output;

  rom2_t<HASHSIZE> romSi;
  uint8_t S0[bbytes];
  uint8_t S1[bbytes];

  rom2_t<HASHSIZE> romMc;
  //rom_k_O<HASHSIZE> romMc_output;
  uint8_t romMc_output[HASHSIZE];
  uint8_t hMc[HASHSIZE];

   rom2_t<HASHSIZE> romFinalM;

  alice_rot_t(nfl::FastGaussianNoise<uint8_t, value_t, 2> *_g_prng)
    : g_prng(_g_prng),
      rom1_output(h)
      //romMc_output(hMc)
  {
  }

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

    rom_k_O<HASHSIZE> romS0_output(hS0);
    rom_k_O<HASHSIZE> romS1_output(hS1);

    romSi(romS0_output, S0, sizeof(S0));
    romSi(romS1_output, S1, sizeof(S1));

    if (b1 == 1)
      {
        rom1(rom1_output, r_sid, rbytes + sizeof(uint32_t));
        p0 = p0 - h;
      }
  }

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

    //hash_polynomial(romMc, skR, romMc_output);
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

    rom_k_O<HASHSIZE> romFinalM_output(Mb);
    romFinalM(romFinalM_output, Mb_sid, sizeof(Mb_sid));
    
    memcpy(bS0, S0, bbytes);
    memcpy(bS1, S1, bbytes);
    return true;
  }
};

template<typename P, size_t rbytes, size_t bbytes, size_t HASHSIZE>
struct bob_rot_t
{
  P sS, eS, eS1;
  P h;
  P p1;
  P kS0, kS1;
  P signal0, signal1;
  P skS0, skS1;

  int a1;

  uint8_t hS0[HASHSIZE];
  uint8_t hS1[HASHSIZE];
  uint8_t hS0b[HASHSIZE];
  uint8_t hS1b[HASHSIZE];

  uint8_t u[bbytes];

  using value_t = typename P::value_type;
  nfl::FastGaussianNoise<uint8_t, value_t, 2>* g_prng;

  rom1_t<P> rom1;
  rom_P_O<P> rom1_output;

  rom2_t<HASHSIZE> romM;

  rom_k_O<HASHSIZE> romhS0b_output;
  rom_k_O<HASHSIZE> romhS1b_output;

  bob_rot_t(nfl::FastGaussianNoise<uint8_t, value_t, 2> *_g_prng)
    : g_prng(_g_prng),
      rom1_output(h),
      romhS0b_output(hS0b),
      romhS1b_output(hS1b)
  {
  }

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
      //rom_k_O<HASHSIZE> romMa0_output(hma0);
      //rom_k_O<HASHSIZE> romMa1_output(hma1);
      //hash_polynomial(romM, skS0, romMa0_output);
      //hash_polynomial(romM, skS1, romMa1_output);
      uint8_t romMa0_output[HASHSIZE];
      uint8_t romMa1_output[HASHSIZE];
      hash_polynomial(skS0, romMa0_output);
      hash_polynomial(skS1, romMa1_output);
      memcpy(hma0, romMa0_output, HASHSIZE * sizeof(uint8_t));
      memcpy(hma1, romMa1_output, HASHSIZE * sizeof(uint8_t));
    } else {
      //rom_k_O<HASHSIZE> romMa0_output(hma1);
      //rom_k_O<HASHSIZE> romMa1_output(hma0);
      //hash_polynomial(romM, skS0, romMa0_output);
      //hash_polynomial(romM, skS1, romMa1_output);
      uint8_t romMa0_output[HASHSIZE];
      uint8_t romMa1_output[HASHSIZE];
      hash_polynomial(skS0, romMa0_output);
      hash_polynomial(skS1, romMa1_output);
      memcpy(hma1, romMa0_output, HASHSIZE * sizeof(uint8_t));
      memcpy(hma0, romMa1_output, HASHSIZE * sizeof(uint8_t));
    }
  }

  bool msg2(uint8_t msg0[HASHSIZE], uint8_t msg1[HASHSIZE],
	    uint32_t sid,
	    const uint8_t S0[bbytes], const uint8_t S1[bbytes])
  {
    romM(romhS0b_output, S0, bbytes);
    romM(romhS1b_output, S1, bbytes);

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
    
    rom_k_O<HASHSIZE> rommsg0_output(msg0);
    rom_k_O<HASHSIZE> rommsg1_output(msg1);
    romM(rommsg0_output, msg0_sid, sizeof(msg0_sid));
    romM(rommsg1_output, msg1_sid, sizeof(msg1_sid));

    return true;
  }
};

#endif
