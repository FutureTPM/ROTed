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

template<typename P, typename R, typename RO>
void hash_polynomial(R &rom, P &pol, RO &rom_output)
{
  uint8_t buf[P::degree];
  constexpr size_t len = sizeof(buf);
  convPtoArray<P, len>(buf, pol);
  rom(rom_output, buf, len);
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
  rom_k_O<HASHSIZE> romMc_output;
  uint8_t hMc[HASHSIZE];

  alice_rot_t(nfl::FastGaussianNoise<uint8_t, value_t, 2> *_g_prng)
    : g_prng(_g_prng),
      rom1_output(h),
      romMc_output(hMc)
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

  bool msg2(uint8_t Mb[bbytes],
	    int &b,
	    uint8_t bS0[bbytes], uint8_t bS1[bbytes],
	    uint32_t sid, const P &pS, const P &signal0, const P &signal1,
            const uint8_t ha0[HASHSIZE], const uint8_t ha1[HASHSIZE],
            const uint8_t u0[bbytes], const uint8_t u1[bbytes])
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

    hash_polynomial(romMc, skR, romMc_output);

    if (memcmp(ha0, hMc, HASHSIZE) == 0) {
      b = 0;
    } else if (memcmp(ha1, hMc, HASHSIZE) == 0) {
      b = 1;
    } else {
      return false;
    }

    convPtoArray<P, bbytes>(Mb, skR);

    if (b1 == 0)
      {
	for (int i = 0; i < bbytes; i++)
	  {
	    Mb[i] ^= S0[i] ^ u0[i];
	  }
      }
    else
      {
	for (int i = 0; i < bbytes; i++)
	  {
	    Mb[i] ^= S1[i] ^ u1[i];
	  }
      }

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

  uint8_t u0[bbytes];
  uint8_t u1[bbytes];

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
	    uint8_t au0[bbytes], uint8_t au1[bbytes],
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

    nfl::fastrandombytes(&u0[0], sizeof(u0));
    nfl::fastrandombytes(&u1[0], sizeof(u1));

    memcpy(au0, u0, bbytes);
    memcpy(au1, u1, bbytes);

    if (a1 == 0) {
      rom_k_O<HASHSIZE> romMa0_output(hma0);
      rom_k_O<HASHSIZE> romMa1_output(hma1);
      hash_polynomial(romM, skS0, romMa0_output);
      hash_polynomial(romM, skS1, romMa1_output);
    } else {
      rom_k_O<HASHSIZE> romMa0_output(hma1);
      rom_k_O<HASHSIZE> romMa1_output(hma0);
      hash_polynomial(romM, skS0, romMa0_output);
      hash_polynomial(romM, skS1, romMa1_output);
    }
  }

  bool msg2(uint8_t msg0[bbytes], uint8_t msg1[bbytes],
	    const uint8_t S0[bbytes], const uint8_t S1[bbytes])
  {
    romM(romhS0b_output, S0, bbytes);
    romM(romhS1b_output, S1, bbytes);

    if ((memcmp(hS0b, hS0, bbytes) != 0) ||
	(memcmp(hS1b, hS1, bbytes) != 0))
      {
	return false;
      }

    if (a1 == 0)
      {
	convPtoArray<P, bbytes>(msg0, skS0);
	convPtoArray<P, bbytes>(msg1, skS1);

	for (int i = 0; i < bbytes; i++)
	  {
	    msg0[i] ^= S0[i] ^ u0[i];
	    msg1[i] ^= S1[i] ^ u1[i];
	  }
      }
    else
      {
	convPtoArray<P, bbytes>(msg0, skS1);
	convPtoArray<P, bbytes>(msg1, skS0);

	for (int i = 0; i < bbytes; i++)
	  {
	    msg0[i] ^= S1[i] ^ u1[i];
	    msg1[i] ^= S0[i] ^ u0[i];
	  }
      }

    return true;
  }
};

#endif