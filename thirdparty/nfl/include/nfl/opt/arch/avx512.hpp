#ifndef DEF_NFLImplems_avx512
#define DEF_NFLImplems_avx512

#include <immintrin.h>
#include "nfl/opt/arch/sse.hpp"
#include "nfl/opt/arch/avx2.hpp"

namespace nfl {

namespace simd {

struct avx512 {
  template <class T>
  static inline __m512i load(T const* p) { return _mm512_load_si512((__m512i const*) p); }

  template <class T>
  static inline void store(T* p, __m512i const v) { _mm512_store_si512((__m512i*) p, v); }

  template <class T>
  struct elt_count
  {
    static constexpr size_t value = 64/sizeof(T);
  };

  static constexpr int mode = 4;
};

} // simd

template<> struct common_mode<simd::avx512, simd::serial> { using type = simd::serial;};
template<> struct common_mode<simd::avx512, simd::avx2> { using type = simd::avx2;};
template<> struct common_mode<simd::avx2, simd::avx512> { using type = simd::avx2;};
template<> struct common_mode<simd::serial, simd::avx512> { using type = simd::serial;};

namespace ops {

//
// TOOLS
//

static inline __m512i avx512_mulhi_epu32(__m512i avx_a, __m512i avx_b)
{
  const __m512i mullow = _mm512_srli_epi64(_mm512_mul_epu32(avx_a, avx_b), 32);
  _MM_PERM_ENUM mask_shuffle = _MM_PERM_CDAB; // reinterpret_cast<_MM_PERM_ENUM>(1 | (0 << 2) | (3 << 4) | (2 << 6));
  avx_a = _mm512_shuffle_epi32(avx_a, mask_shuffle);
  avx_b = _mm512_shuffle_epi32(avx_b, mask_shuffle);
  const __m512i mulhigh = _mm512_mul_epu32(avx_a, avx_b);
  return reinterpret_cast<__m512i>(_mm512_mask_blend_ps((__mmask16) 0b1010101010101010, reinterpret_cast<__m512>(mullow), reinterpret_cast<__m512>(mulhigh)));
}

template <class Integer>
static inline void assert_strict_mod_avx512(__m512i const avx_v, Integer const p)
{
  assert_vec_values<simd::avx512, Integer>(avx_v, [p](Integer const v) { return v < p; });
}

template <class Integer>
static inline void assert_addmod_input_avx512(__m512i const x, __m512i const y, Integer const p)
{
  assert_addmod_input<simd::avx512>(x, y, p);
}


//
// BASIC OPS
//

template<class T>
struct addmod<T, simd::avx512> : addmod<T, simd::avx2> {};

template<>
struct addmod<uint32_t, simd::avx512>
{
  using simd_mode = simd::avx512;
  __m512i operator()(__m512i x, __m512i y, size_t const cm) const {
    auto const p = params<uint32_t>::P[cm];
    assert_addmod_input_avx512<uint32_t>(x, y, p);

    __m512i avx_p = _mm512_set1_epi32(p);
    __m512i avx_pc = _mm512_set1_epi32(p - 0x80000000 - 1);
    __m512i avx_80 = _mm512_set1_epi32(0x80000000);
    __m512i zeros = _mm512_set1_epi32(0);
    const __m512i z = _mm512_add_epi32(x, y);
    const __mmask16 avx_cmp = _mm512_cmpgt_epi32_mask(_mm512_sub_epi32(z, avx_80), avx_pc);
    const __m512i avx_res = _mm512_sub_epi32(z, _mm512_mask_blend_epi32(avx_cmp, zeros, avx_p));

    assert_strict_mod_avx512<uint32_t>(avx_res, p);
    return avx_res;
  }
};

template<>
struct addmod<uint16_t, simd::avx512>
{
  using simd_mode = simd::avx512;
  __m512i operator()(__m512i x, __m512i y, size_t const cm) const {
    auto const p = params<uint16_t>::P[cm];
    assert_addmod_input_avx512<uint16_t>(x, y, p);

    __m512i avx_p  = _mm512_set1_epi16(p);
    __m512i avx_pc = _mm512_set1_epi16(p - 0x8000 - 1);
    __m512i avx_80 = _mm512_set1_epi16(0x8000);
    __m512i zeros = _mm512_set1_epi16(0);
    const __m512i z = _mm512_add_epi16(x, y);
    const __mmask32 avx_cmp = _mm512_cmpgt_epi16_mask(_mm512_sub_epi16(z, avx_80), avx_pc);
    const __m512i avx_res = _mm512_sub_epi16(z, _mm512_mask_blend_epi16(avx_cmp, zeros, avx_p));

    assert_strict_mod_avx512<uint16_t>(avx_res, p);
    return avx_res;
  }
};

template<class T>
struct mulmod<T, simd::avx512> : mulmod<T, simd::avx2> {};

template<class T>
struct submod<T, simd::avx512> : submod<T, simd::avx2> {};

template<>
struct submod<uint16_t, simd::avx512>
{
  using simd_mode = simd::avx512;
  __m512i operator()(__m512i const x, __m512i const y, size_t const cm) const {
    auto const p = params<uint16_t>::P[cm];
    assert_strict_mod_avx512<uint16_t>(x, p);
    assert_strict_mod_avx512<uint16_t>(y, p);

    auto const avx_p = _mm512_set1_epi16(p);
    auto const avx_res = addmod<uint16_t, simd::avx512>{}(x, _mm512_sub_epi16(avx_p, y), cm);

    assert_strict_mod_avx512<uint16_t>(avx_res, p);
    return avx_res;
  }
};

template<>
struct submod<uint32_t, simd::avx512>
{
  using simd_mode = simd::avx512;
  __m512i operator()(__m512i const x, __m512i const y, size_t const cm) const {
    auto const p = params<uint32_t>::P[cm];
    assert_strict_mod_avx512<uint32_t>(x, p);
    assert_strict_mod_avx512<uint32_t>(y, p);

    auto const avx_p = _mm512_set1_epi32(p);
    auto const avx_res = addmod<uint32_t, simd::avx512>{}(x, _mm512_sub_epi32(avx_p, y), cm);

    assert_strict_mod_avx512<uint32_t>(avx_res, p);
    return avx_res;
  }
};

template<class T>
struct muladd<T, simd::avx512> : muladd<T, simd::avx2> {};

//
// NTT
//

template <class poly, class T>
struct ntt_loop<simd::avx512, poly, T>: public ntt_loop<simd::avx2, poly, T>
{ };

template<class poly>
struct ntt_loop_body<simd::avx512, poly, uint32_t> {
  using value_type = typename poly::value_type;
  using greater_value_type = typename poly::greater_value_type;
  using simd_type = simd::avx512;

  ntt_loop_body(value_type const p)
  {
    _avx_2p = _mm512_set1_epi32(p << 1);
    _avx_p = _mm512_set1_epi32(p);
    _avx_80 = _mm512_set1_epi32(0x80000000);
    _avx_2pc = _mm512_set1_epi32((2*p) - 0x80000000 - 1);
  }

  inline void operator()(value_type* x0, value_type* x1, value_type const* winvtab, value_type const* wtab) const
  {
    __m512i avx_u0, avx_u1, avx_winvtab, avx_wtab, avx_t0, avx_t1, avx_q, avx_t2;
    __mmask16 avx_cmp;
    avx_u0 = _mm512_load_si512((__m512i const*) x0);
    avx_u1 = _mm512_load_si512((__m512i const*) x1);
    avx_winvtab = _mm512_load_si512((__m512i const*) winvtab);
    avx_wtab = _mm512_load_si512((__m512i const*) wtab);

    avx_t1 = _mm512_add_epi32(_avx_2p, _mm512_sub_epi32(avx_u0, avx_u1));

    avx_q = avx512_mulhi_epu32(avx_t1, avx_winvtab);
    avx_t2 = _mm512_sub_epi32(_mm512_mullo_epi32(avx_t1, avx_wtab),
        _mm512_mullo_epi32(avx_q, _avx_p));

    avx_t0 = _mm512_add_epi32(avx_u0, avx_u1);
    __m512i zeros = _mm512_set1_epi32(0);
    avx_cmp = _mm512_cmpgt_epi32_mask(_mm512_sub_epi32(avx_t0, _avx_80), _avx_2pc);
    avx_t0 = _mm512_sub_epi32(avx_t0, _mm512_mask_blend_epi32(avx_cmp, zeros, _avx_2p));

    _mm512_store_si512((__m512i*) x0, avx_t0);
    _mm512_store_si512((__m512i*) x1, avx_t2);
  }

  __m512i _avx_2p;
  __m512i _avx_p;
  __m512i _avx_80;
  __m512i _avx_2pc;
};

template<class poly>
struct ntt_loop_body<simd::avx512, poly, uint16_t> {
  using value_type = typename poly::value_type;
  using greater_value_type = typename poly::greater_value_type;
  using simd_type = simd::avx512;

  ntt_loop_body(value_type const p)
  {
    _avx_2p  = _mm512_set1_epi16(p << 1);
    _avx_p   = _mm512_set1_epi16(p);
    _avx_80  = _mm512_set1_epi16(0x8000);
    _avx_2pc = _mm512_set1_epi16((2*p) - 0x8000 - 1);
  }

  inline void operator()(value_type* x0, value_type* x1, value_type const* winvtab, value_type const* wtab) const
  {
    __m512i avx_u0, avx_u1, avx_winvtab, avx_wtab, avx_t0, avx_t1, avx_q, avx_t2;
    __mmask32 avx_cmp;
    __m512i zeros = _mm512_set1_epi16(0);
    avx_u0        = _mm512_load_si512((__m512i const*) x0);
    avx_u1        = _mm512_load_si512((__m512i const*) x1);
    avx_winvtab   = _mm512_load_si512((__m512i const*) winvtab);
    avx_wtab      = _mm512_load_si512((__m512i const*) wtab);

    avx_t1 = _mm512_add_epi16(_avx_2p, _mm512_sub_epi16(avx_u0, avx_u1));

    avx_q  = _mm512_mulhi_epu16(avx_t1, avx_winvtab);
    avx_t2 = _mm512_sub_epi16(
                _mm512_mullo_epi16(avx_t1, avx_wtab),
                _mm512_mullo_epi16(avx_q, _avx_p)
             );

    avx_t0  = _mm512_add_epi16(avx_u0, avx_u1);
    avx_cmp = _mm512_cmpgt_epi16_mask(_mm512_sub_epi16(avx_t0, _avx_80), _avx_2pc);
    avx_t0  = _mm512_sub_epi16(avx_t0, _mm512_mask_blend_epi16(avx_cmp, zeros, _avx_2p));

    _mm512_store_si512((__m512i*) x0, avx_t0);
    _mm512_store_si512((__m512i*) x1, avx_t2);
  }

  __m512i _avx_2p;
  __m512i _avx_p;
  __m512i _avx_80;
  __m512i _avx_2pc;
};

template<class poly>
struct ntt_loop_avx512_unrolled {
  using value_type = typename poly::value_type;
  using greater_value_type = typename poly::greater_value_type;
  using simd_type = simd::avx512;

  static constexpr size_t J = static_log2<poly::degree>::value-2;
  static constexpr size_t elt_count = simd_type::elt_count<value_type>::value;
  static constexpr size_t elt_count_avx2 = simd::avx2::elt_count<value_type>::value;
  static constexpr size_t elt_count_sse = simd::sse::elt_count<value_type>::value;

#ifdef __GNUC__
  __attribute__((optimize("no-tree-vectorize")))
#endif
  static size_t run(value_type* x, const value_type* &wtab,
      const value_type* &winvtab, const value_type p) {
    ntt_loop_body<simd::avx512, poly, value_type> body_avx512(p);
    ntt_loop_body<simd::avx2, poly, value_type> body_avx2(p);
    ntt_loop_body<simd::sse, poly, value_type> body_sse(p);
    ntt_loop_body<simd::serial, poly, value_type> body(p);

    for (size_t w = 0; w < J-1; w++) {
      const size_t M = 1 << w;
      const size_t N = poly::degree >> w;
      for (size_t r = 0; r < M; r++) {
        const size_t Navx512 = ((N/2)/elt_count)*elt_count;
        for (size_t i = 0; i < Navx512; i += elt_count) {
          body_avx512(&x[N * r + i], &x[N * r + i + N/2], &winvtab[i], &wtab[i]);
        }
        if (Navx512 != (N/2) && (N/2) == elt_count/2) {
          const size_t i = Navx512;
          body_avx2(&x[N * r + i], &x[N * r + i + N/2], &winvtab[i], &wtab[i]);
        }
        if (Navx512 != (N/2) && (N/2) == elt_count/4) {
          const size_t i = Navx512;
          body_sse(&x[N * r + i], &x[N * r + i + N/2], &winvtab[i], &wtab[i]);
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
struct ntt_loop<simd::avx512, poly, uint32_t>: public ntt_loop_avx512_unrolled<poly>
{ };

template<class poly>
struct ntt_loop<simd::avx512, poly, uint16_t>: public ntt_loop_avx512_unrolled<poly>
{ };

//
// MULMOD_SHOUP
//

template<class T>
struct mulmod_shoup<T, simd::avx512> : mulmod_shoup<T, simd::avx2> {};

template<>
struct mulmod_shoup<uint16_t, simd::avx512>
{
  using value_type = uint16_t;
  // AG: this is wanted, as loads/stores are still done using AVX registers
  using simd_mode = simd::avx2;

  constexpr static inline size_t elt_count() { return simd_mode::elt_count<value_type>::value; }

  static inline __m256i finish(__m256i avx_x, __m256i avx_y, __m256i avx_q, __m512i avx_p_32, __m512i avx_pc_32, __m512i avx_80)
  {
    const __m512i avx_x_32 = _mm512_cvtepu16_epi32(avx_x);
    const __m512i avx_y_32 = _mm512_cvtepu16_epi32(avx_y);
    const __m512i avx_q_32 = _mm512_cvtepu16_epi32(avx_q);
    const __m512i zeros    = _mm512_set1_epi32(0);

    const __m512i avx_res = _mm512_sub_epi32(
        _mm512_mullo_epi32(avx_x_32, avx_y_32),
        _mm512_mullo_epi32(avx_q_32, avx_p_32)
        );

    __mmask16 avx_cmp = _mm512_cmpgt_epi32_mask(_mm512_sub_epi32(avx_res, avx_80), avx_pc_32);
    __m512i tmp1      = _mm512_sub_epi32(avx_res, _mm512_mask_blend_epi32(avx_cmp, zeros, avx_p_32));
    __m256i tmp1_high = _mm512_extracti32x8_epi32(tmp1, 1);
    const __m256i tmp1_high_swapped = _mm256_permute2x128_si256(tmp1_high, tmp1_high, 1);
    __m256i tmp1_low  = _mm512_extracti32x8_epi32(tmp1, 0);
    const __m256i tmp1_low_swapped = _mm256_permute2x128_si256(tmp1_low, tmp1_low, 1);
    __m512i tmp2_tmp  = _mm512_inserti32x8(zeros, tmp1_low_swapped, 1);
    __m512i tmp2      = _mm512_inserti32x8(tmp2_tmp, tmp1_high_swapped, 0);
    const __m256i ret_tmp = _mm256_packus_epi32(_mm512_castsi512_si256(tmp1), _mm512_castsi512_si256(tmp2));
    // Reorder ret ABCD => ACDB (64 bit words)
    const __m256i ret = _mm256_permute4x64_epi64(ret_tmp, 0b01111000);


    return ret;
  }

#if __GNUC__ == 4 && __GNUC_MINOR__ == 8
  // AG: there is a weird bug with GCC 4.8 that seems to generate incorrect code here.
  // adding a noinline attribute fixes the bug, but we need to get to the bottom of this...
  // This works fine with GCC 4.9 and clang 3.5!
  __attribute__((noinline))
#endif
  __m256i operator()(__m256i const avx_x, __m256i const avx_y, __m256i const avx_yprime, size_t const cm) const
  {
    auto const p = params<value_type>::P[cm];
    assert_strict_mod_avx2<value_type>(avx_x, p);
    assert_strict_mod_avx2<value_type>(avx_y, p);

    __m512i avx_p_32;
    __m512i avx_pc_32;
    __m512i avx_80;
    avx_p_32 = _mm512_set1_epi32(p);
    avx_pc_32 = _mm512_set1_epi32((uint32_t)(p)-0x80000000-1);
    avx_80 = _mm512_set1_epi32(0x80000000);

    __m256i avx_q = _mm256_mulhi_epu16(avx_x, avx_yprime);
    __m256i avx_res = finish(avx_x, avx_y, avx_q, avx_p_32, avx_pc_32, avx_80);

    assert_strict_mod_avx2<value_type>(avx_res, p);
    return avx_res;
  }

};

//
// MULADD_SHOUP
//

template<class T>
struct muladd_shoup<T, simd::avx512> : muladd_shoup<T, simd::avx2> {};

template<>
struct muladd_shoup<uint16_t, simd::avx512>
{
  using value_type = uint16_t;
  // AG: this is wanted, as loads/stores are still done using AVX registers
  using simd_mode = simd::avx2;

  constexpr static inline size_t elt_count() { return simd_mode::elt_count<value_type>::value; }

  static inline __m256i finish(__m256i avx_rop, __m256i avx_x, __m256i avx_y, __m256i avx_q, __m512i avx_p_32, __m512i avx_pc_32, __m512i avx_80)
  {
    const __m512i avx_x_32 = _mm512_cvtepu16_epi32(avx_x);
    const __m512i avx_y_32 = _mm512_cvtepu16_epi32(avx_y);
    const __m512i avx_q_32 = _mm512_cvtepu16_epi32(avx_q);
    const __m512i avx_rop_32 = _mm512_cvtepu16_epi32(avx_rop);

    const __m512i avx_res = _mm512_add_epi32(avx_rop_32,
      _mm512_sub_epi32(
        _mm512_mullo_epi32(avx_x_32, avx_y_32),
        _mm512_mullo_epi32(avx_q_32, avx_p_32)
      )
    );

    __m512i zeros     = _mm512_set1_epi32(0);
    __mmask16 avx_cmp = _mm512_cmpgt_epi32_mask(_mm512_sub_epi32(avx_res, avx_80), avx_pc_32);
    __m512i tmp1      = _mm512_sub_epi32(avx_res, _mm512_mask_blend_epi32(avx_cmp, zeros, avx_p_32));
    __m256i tmp1_high = _mm512_extracti32x8_epi32(tmp1, 1);
    const __m256i tmp1_high_swapped = _mm256_permute2x128_si256(tmp1_high, tmp1_high, 1);
    __m256i tmp1_low  = _mm512_extracti32x8_epi32(tmp1, 0);
    const __m256i tmp1_low_swapped = _mm256_permute2x128_si256(tmp1_low, tmp1_low, 1);
    __m512i tmp2_tmp  = _mm512_inserti32x8(zeros, tmp1_low_swapped, 1);
    __m512i tmp2      = _mm512_inserti32x8(tmp2_tmp, tmp1_high_swapped, 0);
    const __m256i ret_tmp = _mm256_packus_epi32(_mm512_castsi512_si256(tmp1), _mm512_castsi512_si256(tmp2));
    // Reorder ret ABCD => ACDB (64 bit words)
    const __m256i ret = _mm256_permute4x64_epi64(ret_tmp, 0b01111000);
    return ret;
  }

  inline __m256i operator()(__m256i const rop, __m256i const avx_x, __m256i const avx_y, __m256i const avx_yprime, size_t const cm) const
  {
    auto const p = params<value_type>::P[cm];
    assert_strict_mod_avx2<value_type>(avx_x, p);
    assert_strict_mod_avx2<value_type>(avx_y, p);
    assert_strict_mod_avx2<value_type>(rop, p);

    __m512i avx_p_32;
    __m512i avx_pc_32;
    __m512i avx_80;
    avx_p_32 = _mm512_set1_epi32(p);
    avx_pc_32 = _mm512_set1_epi32((uint32_t)(p)-0x80000000-1);
    avx_80 = _mm512_set1_epi32(0x80000000);
    __m256i avx_q = _mm256_mulhi_epu16(avx_x, avx_yprime);
    __m256i avx_res = finish(rop, avx_x, avx_y, avx_q, avx_p_32, avx_pc_32, avx_80);

    assert_strict_mod_avx2<value_type>(avx_res, p);
    return avx_res;
  }

};

template<class T>
struct shoup<T, simd::avx512> {
  using simd_mode = simd::avx512;
};

template<class T>
struct compute_shoup<T, simd::avx512> : compute_shoup<T, simd::avx2> {};

} // ops

} // ntt


#endif

// END

