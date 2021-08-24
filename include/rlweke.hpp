#ifndef __RLWEKE_HPP__
#define __RLWEKE_HPP__
#include <nfl.hpp>
#include <cstdint>

/** Implements helping functions for the key exchange in [DXL12]

[DXL12] Jintai Ding, Xiang Xie, and Xiaodong Lin. A simple provably
secure key exchange scheme based on the learning with errors
problem. Cryptology ePrint Archive, Report 2012/688, 2012.
https://eprint.iacr.org/2012/688.

@tparam P NFL polynomial type
 */
template<typename P>
struct ke_t
{
  /** Coefficient type */
  using value_t = typename P::value_type;
  /** Signed coefficient type */
  using signed_value_t = typename P::signed_value_type;
  /**@{*/
  /** Polynomial ring defined in Zq[X]/(X^degree + 1) */
  static constexpr signed_value_t q = P::get_modulus(0);
  static constexpr signed_value_t qov2 = q / 2;
  static constexpr signed_value_t qov4 = q / 4;
  static constexpr signed_value_t qp1ov4 = (q + 1) / 4;
  static constexpr size_t degree = P::degree;
  /**@}*/

  /** Extracts random byte with NFL

      @return Random byte
   */
  static unsigned char random_bit()
  {
    unsigned char r;
    nfl::fastrandombytes(&r, 1);
    return r;
  }

  /** Implements hint signal function as per [DXL12]

[DXL12] Jintai Ding, Xiang Xie, and Xiaodong Lin. A simple provably
secure key exchange scheme based on the learning with errors
problem. Cryptology ePrint Archive, Report 2012/688, 2012.
https://eprint.iacr.org/2012/688.

      @param sig Returned signal value
      @param k Input polynomial
      @param r Random bit
  */
  static void signal(P &sig, const P &k, unsigned char r)
  {
    sig = (value_t)0;

    switch (r & 1)
      {
      case 0:
	for (int i = 0; i < degree; i++)
	  {
	    signed_value_t ki = (k(0, i) <= qov2 ? k(0, i) :
				 (signed_value_t)k(0, i) - q);

	    if (ki < -qov4 || ki > qov4)
	      sig(0, i) = 1;
	  }
	break;
      case 1:
	for (int i = 0; i < degree; i++)
	  {
	    signed_value_t ki = (k(0, i) <= qov2 ? k(0, i) :
				 (signed_value_t)k(0, i) - q);

	    if (ki < -qp1ov4 || ki > qp1ov4)
	      sig(0, i) = 1;
	  }
	break;
      default:
	break;
      }
  }

  /** Implements robust extractor as per [DXL12]

[DXL12] Jintai Ding, Xiang Xie, and Xiaodong Lin. A simple provably
secure key exchange scheme based on the learning with errors
problem. Cryptology ePrint Archive, Report 2012/688, 2012.
https://eprint.iacr.org/2012/688.

     @param sk Returned value
     @param k Input polynomial
     @param sig Hint signal polynomial
  */
  static void mod2(P &sk, const P &k, const P &sig) {
      sk = (value_t)0;

      for (int i = 0; i < degree; i++) {
          if (sig(0, i) == 1) {
              sk(0, i) = qov2;
          }
      }
      sk = sk + k;
      for (int i = 0; i < degree; i++) {
          signed_value_t ski = (sk(0, i) <= qov2 ? sk(0, i) : (signed_value_t)sk(0, i) - q);
          sk(0, i) = ski & 1;
      }
  }
};

/** Implements Alice for the key exchange in [DXL12]

[DXL12] Jintai Ding, Xiang Xie, and Xiaodong Lin. A simple provably
secure key exchange scheme based on the learning with errors
problem. Cryptology ePrint Archive, Report 2012/688, 2012.
https://eprint.iacr.org/2012/688.

@tparam P NFL polynomial type
 */
template<typename P>
struct alice_ke_t
{
  /**@{*/
  /** Polynomials used for RLWE sampling */
  P sA, eA1, eA2, kA;
  P sk;
  /**@}*/

  /** Coefficient type */
  using value_t = typename P::value_type;
  /** Gaussian Noise sampler */
  nfl::FastGaussianNoise<uint8_t, value_t, 2> *g_prng;

  /** Alice constructor

      @param _g_prng Gaussian Noise sampler
  */
  alice_ke_t(nfl::FastGaussianNoise<uint8_t, value_t, 2> *_g_prng)
    : g_prng(_g_prng)
  {
  }

  /** Implements Alice's first message in the key exchange in [DXL12]

      @param pA Returned Alice RLWE sample
      @param m Common polynomial m
  */
  void msg(P &pA, const P &m)
  {
    sA = nfl::gaussian<uint8_t, value_t, 2>(g_prng);
    eA1 = nfl::gaussian<uint8_t, value_t, 2>(g_prng, 2);
    eA2 = nfl::gaussian<uint8_t, value_t, 2>(g_prng, 2);
    sA.ntt_pow_phi();
    eA1.ntt_pow_phi();

    pA = m * sA + eA1;
  }

  /** Combines Bob's RLWE sample with Alice's to produce common key.
      Resulting key is stored in sK.

      @param pB Bob's RLWE sample
      @param signal Hint signal
  */
  void reconciliate(const P &pB, const P& signal)
  {
    kA = sA * pB;
    kA.invntt_pow_invphi();
    kA = kA + eA2;

    ke_t<P>::mod2(sk, kA, signal);
  }
};

/** Implements Bob for the key exchange in [DXL12]

[DXL12] Jintai Ding, Xiang Xie, and Xiaodong Lin. A simple provably
secure key exchange scheme based on the learning with errors
problem. Cryptology ePrint Archive, Report 2012/688, 2012.
https://eprint.iacr.org/2012/688.

@tparam P NFL polynomial type
 */
template<typename P>
struct bob_ke_t
{
  /**@{*/
  /** Polynomials used for RLWE sampling */
  P sB, eB1, eB2, kB;
  P sk;
  /**@}*/

  /** Coefficient type */
  using value_t = typename P::value_type;
  /** Gaussian Noise sampler */
  nfl::FastGaussianNoise<uint8_t, value_t, 2> *g_prng;

  /** Bob constructor

      @param _g_prng Gaussian Noise sampler
  */
  bob_ke_t(nfl::FastGaussianNoise<uint8_t, value_t, 2> *_g_prng)
    : g_prng(_g_prng)
  {
  }

  /** Implements Bob's message in the key exchange in [DXL12].
      Resulting key is stored in sK.

      @param pB Returned Bob RLWE sample
      @param signal Return hint signal
      @param pA Alice RLWE sample
      @param m Common polynomial
  */
  void msg(P &pB, P &signal, const P &pA, const P &m)
  {
    sB = nfl::gaussian<uint8_t, value_t, 2>(g_prng);
    eB1 = nfl::gaussian<uint8_t, value_t, 2>(g_prng, 2);
    eB2 = nfl::gaussian<uint8_t, value_t, 2>(g_prng, 2);
    sB.ntt_pow_phi();
    eB1.ntt_pow_phi();

    pB = m * sB + eB1;

    kB = pA * sB;
    kB.invntt_pow_invphi();
    kB = kB + eB2;

    ke_t<P>::signal(signal, kB, ke_t<P>::random_bit());
    ke_t<P>::mod2(sk, kB, signal);
  }
};

#endif
