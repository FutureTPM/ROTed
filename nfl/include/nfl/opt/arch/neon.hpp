#ifndef NFL_ARCH_NEON_HPP
#define NFL_ARCH_NEON_HPP

#include "nfl/arch/common.hpp"
#include "nfl/algos.hpp"
#include <arm_neon.h>
#include "nfl/arch/common.hpp"
#include <iostream>

namespace nfl {

namespace simd {

struct neon
{
  template <class T>
  static inline uint32x4_t load(T const* p) {
      switch (sizeof(T)) {
          case 2:
              return vreinterpretq_u32_u16(vld1q_u16((uint16_t const*)p));
          case 4:
              return vld1q_u32((uint32_t const*)p);
          case 8:
              return vreinterpretq_u32_u64(vld1q_u64((uint64_t const*)p));
      }
  }

  template <class uint16_t>
  static inline void store(uint16_t* p, uint16x8_t const v) {
      vst1q_u16((uint16_t*)p, v);
  }

  template <class int16_t>
  static inline void store(int16_t* p, int16x8_t const v) {
      vst1q_s16((short int*)p, v);
  }

  template <class uint32_t>
  static inline void store(uint32_t* p, uint32x4_t const v) {
      vst1q_u32((uint32_t*)p, v);
  }

  template <class uint64_t>
  static inline void store(uint64_t* p, uint64x2_t const v) {
      vst1q_u64((uint64_t*)p, v);
  }

  template <class T>
  struct elt_count
  {
    static constexpr size_t value = 16/sizeof(T);
  };

  static constexpr int mode = 1;
};

} // simd

template<> struct common_mode<simd::neon, simd::serial> { using type = simd::serial;};
template<> struct common_mode<simd::serial, simd::neon> { using type = simd::serial;};


namespace ops {

//
// TOOLS
//

static inline uint32x4_t shuffle_epi32(uint32x4_t const v, uint8_t const imm)
{
    uint32x4_t ret;
    ret = vmovq_n_u32(vgetq_lane_u32(v, (imm) &0x3));
    ret = vsetq_lane_u32(vgetq_lane_u32(v, ((imm) >> 2) & 0x3), ret, 1);
    ret = vsetq_lane_u32(vgetq_lane_u32(v, ((imm) >> 4) & 0x3), ret, 2);
    ret = vsetq_lane_u32(vgetq_lane_u32(v, ((imm) >> 6) & 0x3), ret, 3);
    return ret;
}

// Adapted from https://github.com/DLTcollab/sse2neon/blob/69d1d901c751ccfdb797579b4ca6a4936166b3a7/sse2neon.h#L2026
static inline uint64x2_t mul_epu32(uint32x4_t a, uint32x4_t b)
{
    uint64x2_t a_casted = vreinterpretq_u64_u32(a);
    uint64x2_t b_casted = vreinterpretq_u64_u32(b);

    uint32x2_t a_lo = vmovn_u64(a_casted);
    uint32x2_t b_lo = vmovn_u64(b_casted);
    return vmull_u32(a_lo, b_lo);
}

// Adapted from https://github.com/DLTcollab/sse2neon/blob/69d1d901c751ccfdb797579b4ca6a4936166b3a7/sse2neon.h#L1276
static inline uint32x4_t blend_ps32_u64(uint64x2_t a, uint64x2_t b, uint8_t imm)
{
    const uint32_t mask[4] = {
        ((imm) & (1 << 0)) ? 0xFFFFFFFF : 0x00000000,
        ((imm) & (1 << 1)) ? 0xFFFFFFFF : 0x00000000,
        ((imm) & (1 << 2)) ? 0xFFFFFFFF : 0x00000000,
        ((imm) & (1 << 3)) ? 0xFFFFFFFF : 0x00000000
    };

    uint32x4_t a_casted = vreinterpretq_u32_u64(a);
    uint32x4_t b_casted = vreinterpretq_u32_u64(b);

    uint32x4_t mask_vec = vld1q_u32(mask);
    return vbslq_u32(mask_vec, b_casted, a_casted);
}

static inline uint32x4_t mulhi_epu32(uint32x4_t neon_a, uint32x4_t neon_b)
{
  const int64x2_t mullow = vshrq_n_s64((int64x2_t) mul_epu32(neon_a, neon_b), 32);
  neon_a = shuffle_epi32(neon_a, 1 | (0 << 2) | (3 << 4) | (2 << 6));
  neon_b = shuffle_epi32(neon_b, 1 | (0 << 2) | (3 << 4) | (2 << 6));
  const uint64x2_t mulhigh = mul_epu32(neon_a, neon_b);
  return blend_ps32_u64((uint64x2_t)mullow, mulhigh, 0b1010);
}

static inline uint16x8_t mulhi_epu16(uint16x8_t neon_a, uint16x8_t neon_b)
{
    uint16x4_t a3210  = vget_low_u16(neon_a);
    uint16x4_t b3210  = vget_low_u16(neon_b);
    uint32x4_t ab3210 = vmull_u16(a3210, b3210); /* 3333222211110000 */
    uint16x4_t a7654  = vget_high_u16(neon_a);
    uint16x4_t b7654  = vget_high_u16(neon_b);
    uint32x4_t ab7654 = vmull_u16(a7654, b7654); /* 7777666655554444 */

    uint16x8x2_t r = vuzpq_u16(
            vreinterpretq_u16_u32(ab3210),
            vreinterpretq_u16_u32(ab7654)
            );
    return r.val[1];
}

template <class Integer>
static inline void assert_strict_mod_neon(uint32x4_t const neon_v, Integer const p)
{
  assert_vec_values<simd::neon, Integer>(neon_v, [p](Integer const v) { return v < p; });
}

template <class Integer>
static inline void assert_strict_mod_neon(uint16x8_t const neon_v, Integer const p)
{
  assert_vec_values<simd::neon, Integer>(neon_v, [p](Integer const v) { return v < p; });
}

template <class Integer>
static inline void assert_strict_mod_neon(int32x4_t const neon_v, Integer const p)
{
  assert_vec_values<simd::neon, Integer>(neon_v, [p](Integer const v) { return v < p; });
}

template <class Integer>
static inline void assert_strict_mod_neon(int16x8_t const neon_v, Integer const p)
{
  assert_vec_values<simd::neon, Integer>(neon_v, [p](Integer const v) { return v < p; });
}

template <class Integer>
static inline void assert_addmod_input_neon(uint32x4_t const x, uint32x4_t const y, Integer const p)
{
  assert_addmod_input<simd::neon>(x, y, p);
}

template <class Integer>
static inline void assert_addmod_input_neon(uint16x8_t const x, uint16x8_t const y, Integer const p)
{
  assert_addmod_input<simd::neon>(x, y, p);
}

template <class Integer>
static inline void assert_addmod_input_neon(int32x4_t const x, int32x4_t const y, Integer const p)
{
  assert_addmod_input<simd::neon>(x, y, p);
}

template <class Integer>
static inline void assert_addmod_input_neon(int16x8_t const x, int16x8_t const y, Integer const p)
{
  assert_addmod_input<simd::neon>(x, y, p);
}

//
// BASIC OPS
//
template<class T>
struct addmod<T, simd::neon> : addmod<T, simd::serial> {};

template<>
struct addmod<uint32_t, simd::neon>
{
  using simd_mode = simd::neon;
  int32x4_t operator()(int32x4_t const x, int32x4_t const y, size_t const cm) const {
    auto const p = params<uint32_t>::P[cm];
    assert_addmod_input_neon<uint32_t>(x, y, p);

    int32x4_t neon_p  = vdupq_n_s32(p);
    int32x4_t neon_pc = vdupq_n_s32(p - 0x80000000 - 1);
    int32x4_t neon_80 = vdupq_n_s32(0x80000000);
    const int32x4_t z = vaddq_s32(x, y);
    const int32x4_t neon_cmp = (int32x4_t) vcgtq_s32(vsubq_s32(z, neon_80), neon_pc);
    const int32x4_t neon_res = vsubq_s32(z, vandq_s32(neon_cmp, neon_p));

    assert_strict_mod_neon<uint32_t>(neon_res, p);
    return neon_res;
  }
};

template<>
struct addmod<uint16_t, simd::neon>
{
  using simd_mode = simd::neon;
  int16x8_t operator()(int16x8_t x, int16x8_t y, size_t const cm) const {
    auto const p = params<uint16_t>::P[cm];
    assert_addmod_input_neon<uint16_t>(x, y, p);

    int16x8_t neon_p  = vdupq_n_s16(p);
    int16x8_t neon_pc = vdupq_n_s16(p - 0x8000 - 1);
    int16x8_t neon_80 = vdupq_n_s16(0x8000);
    const int16x8_t z = vaddq_s16(x, y);
    const int16x8_t neon_cmp = (int16x8_t) vcgtq_s16(vsubq_s16(z, neon_80), neon_pc);
    const int16x8_t neon_res = vsubq_s16(z, vandq_s16(neon_cmp, neon_p));

    assert_strict_mod_neon<uint16_t>(neon_res, p);
    return neon_res;
  }
};

template<class T>
struct mulmod<T, simd::neon> : mulmod<T, simd::serial> {};

template<class T>
struct submod<T, simd::neon> : submod<T, simd::serial> {};

template<>
struct submod<uint16_t, simd::neon>
{
  using simd_mode = simd::neon;
  int16x8_t operator()(int16x8_t const x, int16x8_t const y, size_t const cm) const {
    auto const p = params<uint16_t>::P[cm];
    assert_strict_mod_neon<uint16_t>(x, p);
    assert_strict_mod_neon<uint16_t>(y, p);

    auto const neon_p = vdupq_n_s16(p);
    auto const neon_res = addmod<uint16_t, simd::neon>{}(x, vsubq_s16(neon_p, y), cm);

    assert_strict_mod_neon<uint16_t>(neon_res, p);
    return neon_res;
  }
};

template<>
struct submod<uint32_t, simd::neon>
{
  using simd_mode = simd::neon;
  int32x4_t operator()(int32x4_t const x, int32x4_t const y, size_t const cm) const {
    auto const p = params<uint32_t>::P[cm];
    assert_strict_mod_neon<uint32_t>(x, p);
    assert_strict_mod_neon<uint32_t>(y, p);

    auto const neon_p = vdupq_n_s32(p);
    auto const neon_res = addmod<uint32_t, simd::neon>{}(x, vsubq_s32(neon_p, y), cm);

    assert_strict_mod_neon<uint32_t>(neon_res, p);
    return neon_res;
  }
};

template<class T>
struct muladd<T, simd::neon> : muladd<T, simd::serial> {};

//
// NTT
//

template<class poly>
struct ntt_loop_body<simd::neon, poly, uint32_t>
{
  using value_type = typename poly::value_type;
  using greater_value_type = typename poly::greater_value_type;
  using simd_mode = simd::neon;

  ntt_loop_body(value_type const p)
  {
    _neon_2p  = vdupq_n_s32(p << 1);
    _neon_p   = vdupq_n_s32(p);
    _neon_80  = vdupq_n_s32(0x80000000);
    _neon_2pc = vdupq_n_s32((2*p) - 0x80000000 - 1);
  }

  inline void operator()(value_type* x0, value_type* x1, value_type const* winvtab, value_type const* wtab) const
  {
    int32x4_t neon_u0, neon_u1, neon_winvtab, neon_wtab, neon_t0, neon_t1, neon_q, neon_t2, neon_cmp;
    neon_u0      = uvld1q_u32(x0);
    neon_u1      = uvld1q_u32(x1);
    neon_winvtab = uvld1q_u32(winvtab);
    neon_wtab    = uvld1q_u32(wtab);

    neon_t1 = vaddq_s32(_neon_2p, vsubq_s32(neon_u0, neon_u1));

    neon_q  = mulhi_epu32((uint32x4_t)neon_t1, (uint32x4_t)neon_winvtab);
    neon_t2 = vsubq_s32(
            vmulq_s32(neon_t1, neon_wtab),
            vmulq_s32(neon_q, _neon_p)
        );

    neon_t0  = vaddq_s32(neon_u0, (int32x4_t)neon_u1);
    neon_cmp = vcgtq_s32(vsubq_s32(neon_t0, _neon_80), _neon_2pc);
    neon_t0  = vsubq_s32(neon_t0, vandq_s32(neon_cmp, _neon_2p));

    vst1q_s32(x0, neon_t0);
    vst1q_s32(x1, neon_t2);
  }

  int32x4_t _neon_2p;
  int32x4_t _neon_p;
  int32x4_t _neon_80;
  int32x4_t _neon_2pc;
};

template<class poly>
struct ntt_loop_body<simd::neon, poly, uint16_t>
{
  using value_type = typename poly::value_type;
  using greater_value_type = typename poly::greater_value_type;
  using simd_mode = simd::neon;

  ntt_loop_body(value_type const p)
  {
    _neon_2p  = vdupq_n_s16(2*p);
    _neon_p   = vdupq_n_s16(p);
    _neon_80  = vdupq_n_s16(0x8000);
    _neon_2pc = vdupq_n_s16((2*p) - 0x8000 - 1);
  }

  inline void operator()(value_type* x0, value_type* x1, value_type const* winvtab, value_type const* wtab) const
  {
    int16x8_t neon_u0, neon_u1, neon_winvtab, neon_wtab, neon_t0, neon_t1, neon_q, neon_t2, neon_cmp;
    neon_u0      = (int16x8_t) vld1q_u16(x0);
    neon_u1      = (int16x8_t) vld1q_u16(x1);
    neon_winvtab = (int16x8_t) vld1q_u16(winvtab);
    neon_wtab    = (int16x8_t) vld1q_u16(wtab);

    neon_t1 = vaddq_s16(_neon_2p, vsubq_s16(neon_u0, neon_u1));
    neon_q  = (int16x8_t)mulhi_epu16((uint16x8_t)neon_t1, (uint16x8_t)neon_winvtab);

    neon_t2 = vsubq_s16(
            vmulq_s16(neon_t1, neon_wtab),
            vmulq_s16(neon_q, _neon_p)
        );

    neon_t0  = vaddq_s16(neon_u0, neon_u1);
    neon_cmp = (int16x8_t) vcgtq_s16(vsubq_s16(neon_t0, _neon_80), _neon_2pc);
    neon_t0  = vsubq_s16(neon_t0, vandq_s16(neon_cmp, _neon_2p));

    vst1q_u16(x0, neon_t0);
    vst1q_u16(x1, neon_t2);
  }

  int16x8_t _neon_2p;
  int16x8_t _neon_p;
  int16x8_t _neon_80;
  int16x8_t _neon_2pc;
};

// Default implementation
template <class poly, class T>
struct ntt_loop<simd::neon, poly, T>: ntt_loop<simd::serial, poly, T> {};

template<class poly>
struct ntt_loop_neon_unrolled {
  using value_type = typename poly::value_type;
  using greater_value_type = typename poly::greater_value_type;
  using simd_mode = simd::neon;

  static constexpr size_t J = static_log2<poly::degree>::value-2;

#ifdef __GNUC__
  __attribute__((optimize("no-tree-vectorize")))
#endif
  static size_t run(value_type* x, const value_type* &wtab, const value_type* &winvtab, const value_type p) {
    ntt_loop_body<simd::neon, poly, value_type> body_neon(p);
    ntt_loop_body<simd::serial, poly, value_type> body(p);

    for (size_t w = 0; w < J-1; w++) {
      const size_t M = 1 << w;
      const size_t N = poly::degree >> w;
      for (size_t r = 0; r < M; r++) {
        for (size_t i = 0; i < N/2; i += simd_mode::elt_count<value_type>::value) {
          body_neon(&x[N * r + i], &x[N * r + i + N/2], &winvtab[i], &wtab[i]);
        }
      }
      wtab += N / 2;
      winvtab += N / 2;
    }

    // Special case for w=J-1: not able to fill a register of data!
    constexpr size_t w = J-1;
    constexpr size_t M = 1 << w;
    constexpr size_t N = poly::degree >> w;
    for (size_t r = 0; r < M; r++) {
      for (size_t i = 0; i < N/2; i++) {
        body(&x[N * r + i], &x[N * r + i + N/2], &winvtab[i], &wtab[i]);
      }
    }
    wtab += N / 2;
    winvtab += N / 2;

    return 1<<J;
  }
};
template<class poly>
struct ntt_loop<simd::neon, poly, uint32_t>: public ntt_loop_neon_unrolled<poly>
{ };

template<class poly>
struct ntt_loop<simd::neon, poly, uint16_t>: public ntt_loop_neon_unrolled<poly>
{ };

//
// MULMOD_SHOUP
//
template<class T>
struct mulmod_shoup<T, simd::neon> : mulmod_shoup<T, simd::serial> {};

#ifdef __ARM_64BIT_STATE
template<>
struct mulmod_shoup<uint32_t, simd::neon>
{
  using value_type = uint32_t;
  using simd_mode = simd::neon;

  constexpr static inline size_t elt_count() { return simd_mode::elt_count<value_type>::value; }

  static inline uint64x2_t finish(uint32x4_t neon_x, uint32x4_t neon_y, uint32x4_t neon_q, uint32x4_t neon_p_32, uint64x2_t neon_p_64, uint64x2_t neon_pc_64, uint64x2_t neon_80)
  {
      //greater_value_type res;
      //res = x * y - q * p;
      //res -= ((res>=p) ? p : 0);
    const int64x2_t neon_res = vsubq_s64(
        mul_epu32(neon_x, neon_y),
        mul_epu32(neon_q, neon_p_32)
      );

    const uint64x2_t neon_cmp = vcgtq_u64(vsubq_u64(neon_res, neon_80), neon_pc_64);
    return vsubq_s64(neon_res, vandq_u64(neon_cmp, neon_p_64));
  }


  static inline uint32x4_t shuffle_lh(uint32x4_t const v)
  {
    return shuffle_epi32(v, 1 | (0 << 2) | (3 << 4) | (2 << 6));
  }

  inline uint32x4_t operator()(uint32x4_t const neon_x, uint32x4_t const neon_y, uint32x4_t const neon_yprime, size_t const cm) const
  {
    auto const p = params<value_type>::P[cm];
    assert_strict_mod_neon<uint32_t>(neon_x, p);
    assert_strict_mod_neon<uint32_t>(neon_y, p);

    uint32x4_t neon_p_32;
    uint64x2_t neon_p_64;
    uint64x2_t neon_pc_64;
    uint64x2_t neon_80;
    uint64x2_t neon_q, res1, res2;
    neon_p_32  = vdupq_n_u32(p);
    neon_p_64  = vdupq_n_u64(p);
    neon_pc_64 = vdupq_n_u64((uint64_t)(p)-0x8000000000000000ULL-1);
    neon_80    = vdupq_n_u64(0x8000000000000000ULL);
    neon_q     = mulhi_epu32(neon_x, neon_yprime);

    res1 = finish(neon_x, neon_y, neon_q, neon_p_32, neon_p_64, neon_pc_64, neon_80);
    res2 = finish(shuffle_lh(neon_x), shuffle_lh(neon_y), shuffle_lh(neon_q), neon_p_32, neon_p_64, neon_pc_64, neon_80);
	res2 = vshlq_n_u64(res2, 32);
    const uint32x4_t neon_res = blend_ps32_u64(res1, res2, 0b1010);

    assert_strict_mod_neon<uint32_t>(neon_res, p);
    return neon_res;
  }

};
#endif

template<>
struct mulmod_shoup<uint16_t, simd::neon>
{
  using value_type = uint16_t;
  using simd_mode = simd::neon;

  constexpr static inline size_t elt_count() { return simd_mode::elt_count<value_type>::value; }

  static inline uint32x4_t cvtepu16_epi32(uint16x8_t a)
  {
      return vmovl_u16(vget_low_u16(a));
  }

  static inline int32x4_t finish(uint16x8_t neon_x, uint16x8_t neon_y, uint16x8_t neon_q, int32x4_t neon_p_32, int32x4_t neon_pc_32, int32x4_t neon_80)
  {
      //greater_value_type res;
      //res = x * y - q * p;
      //res -= ((res>=p) ? p : 0);

      const int32x4_t neon_x_32l = (const int32x4_t) cvtepu16_epi32(neon_x);
      const int32x4_t neon_y_32l = (const int32x4_t) cvtepu16_epi32(neon_y);
      const int32x4_t neon_q_32l = (const int32x4_t) cvtepu16_epi32(neon_q);

      const int32x4_t neon_res = vsubq_s32(
        vmulq_s32(neon_x_32l, neon_y_32l),
        vmulq_s32(neon_q_32l, neon_p_32)
      );

      const int32x4_t neon_cmp = (int32x4_t) vcgtq_s32(vsubq_s32(neon_res, neon_80), neon_pc_32);
      return vsubq_s32(neon_res, vandq_s32(neon_cmp, neon_p_32));
  }

  static inline uint16x8_t shift8(uint16x8_t const v)
  {
    return vreinterpretq_u16_u8(vextq_u8(vreinterpretq_u8_u16(v), vdupq_n_u8(0), 8));
  }

  inline uint16x8_t operator()(uint16x8_t const neon_x, uint16x8_t const neon_y, uint16x8_t const neon_yprime, size_t const cm) const
  {
    auto const p = params<value_type>::P[cm];
    assert_strict_mod_neon<uint16_t>(neon_x, p);
    assert_strict_mod_neon<uint16_t>(neon_y, p);

    int32x4_t neon_p_32;
    int32x4_t neon_pc_32;
    int32x4_t neon_80;
    uint16x8_t neon_q;
    int32x4_t res1, res2;

    neon_p_32  = vdupq_n_s32(p);
    neon_pc_32 = vdupq_n_s32((uint32_t)(p)-0x80000000-1);
    neon_80    = vdupq_n_s32(0x80000000);
    neon_q     = mulhi_epu16(neon_x, neon_yprime);

    // Cast to 32-bits and use mullo_epi32. Do it twice and merge!
    res1 = finish(neon_x, neon_y, neon_q, neon_p_32, neon_pc_32, neon_80);
    res2 = finish(shift8(neon_x), shift8(neon_y), shift8(neon_q), neon_p_32, neon_pc_32, neon_80);

    const uint16x8_t neon_res = vcombine_u16(
            vqmovn_u32((uint32x4_t)res1),
            vqmovn_u32((uint32x4_t)res2)
            );
    assert_strict_mod_neon<uint16_t>(neon_res, p);
    return neon_res;
  }

};

//
// MULADD_SHOUP
//

template<class T>
struct muladd_shoup<T, simd::neon> : muladd_shoup<T, simd::serial> {};

template<>
struct muladd_shoup<uint16_t, simd::neon>
{
  using value_type = uint16_t;
  using simd_mode = simd::neon;

  constexpr static inline size_t elt_count() { return simd_mode::elt_count<value_type>::value; }

  static inline uint32x4_t cvtepu16_epi32(uint16x8_t a)
  {
      return vmovl_u16(vget_low_u16(a));
  }

  static inline int32x4_t finish(uint16x8_t neon_rop, uint16x8_t neon_x, uint16x8_t neon_y, uint16x8_t neon_q, int32x4_t neon_p_32, int32x4_t neon_pc_32, int32x4_t neon_80)
  {
      //greater_value_type res;
      //res = x * y - q * p;
      //res -= ((res>=p) ? p : 0);

      const int32x4_t neon_x_32l   = (int32x4_t)cvtepu16_epi32(neon_x);
      const int32x4_t neon_y_32l   = (int32x4_t)cvtepu16_epi32(neon_y);
      const int32x4_t neon_q_32l   = (int32x4_t)cvtepu16_epi32(neon_q);
      const int32x4_t neon_rop_32l = (int32x4_t)cvtepu16_epi32(neon_rop);

      int32x4_t neon_res = vaddq_s32(neon_rop_32l,
        vsubq_s32(
          vmulq_s32(neon_x_32l, neon_y_32l),
          vmulq_s32(neon_q_32l, neon_p_32)
        )
      );

      const int32x4_t neon_cmp = (int32x4_t) vcgtq_s32(vaddq_s32(neon_res, neon_80), neon_pc_32);
      return vsubq_s32(neon_res, vandq_s32(neon_cmp, neon_p_32));
  }

  static inline uint16x8_t shift8(uint16x8_t const v)
  {
    return vreinterpretq_u16_u8(vextq_u8(vreinterpretq_u8_u16(v), vdupq_n_u8(0), 8));
  }

  inline uint16x8_t operator()(uint16x8_t const neon_rop, uint16x8_t const neon_x, uint16x8_t const neon_y, uint16x8_t const neon_yprime, size_t const cm) const
  {
    auto const p = params<uint16_t>::P[cm];
    assert_strict_mod_neon<uint16_t>(neon_x, p);
    assert_strict_mod_neon<uint16_t>(neon_y, p);
    assert_strict_mod_neon<uint16_t>(neon_rop, p);

    int32x4_t _neon_p_32;
    int32x4_t _neon_pc_32;
    int32x4_t _neon_80;
    _neon_p_32  = vdupq_n_s32(p);
    _neon_pc_32 = vdupq_n_s32((uint32_t)(p)-0x80000000-1);
    _neon_80    = vdupq_n_s32(0x80000000);
    uint16x8_t neon_q;
    int32x4_t res1, res2;
    neon_q = mulhi_epu16(neon_x, neon_yprime);

    // Cast to 32-bits and use mullo_epi32. Do it twice and merge!
    res1 = finish(neon_rop, neon_x, neon_y, neon_q, _neon_p_32, _neon_pc_32, _neon_80);
    res2 = finish(shift8(neon_rop), shift8(neon_x), shift8(neon_y), shift8(neon_q), _neon_p_32, _neon_pc_32, _neon_80);

    const uint16x8_t neon_res = vcombine_u16(
            vqmovn_u32((uint32x4_t)res1),
            vqmovn_u32((uint32x4_t)res2)
            );
    assert_strict_mod_neon<uint16_t>(neon_res, p);
    return neon_res;
  }

};

template<class T>
struct shoup<T, simd::neon> {
  using simd_mode = simd::neon;
};

template<class T>
struct compute_shoup<T, simd::neon> : compute_shoup<T, simd::serial> {};

} // ops

} // nfl

#endif
