#ifndef __ROMS_HPP__
#define __ROMS_HPP__
#include "sha256.h"
#include <algorithm>

void sha2(uint8_t *out, const uint8_t *in, size_t len)
{
  sha256_ctx_t ctx;
  sha256_init(&ctx);
  sha256_update(&ctx, in, len);
  sha256_finish(&ctx, out);
}

//random oracle with generic codomain
template<typename O,
	 typename T>
struct rom_t
{
  uint8_t a[33];
  T tmp_buffer;

  rom_t()
    : tmp_buffer(a)
  {
  }

  void operator()(O &b, const uint8_t *data, size_t len)
  {
    sha2(a, data, len);
    b.restart();
    tmp_buffer.restart();

    while (!b.full())
      {
	if(tmp_buffer.empty())
	  tmp_buffer.refresh();

	typename T::value_t ti = tmp_buffer.next();
	if (tmp_buffer.filter(ti))
	  {
	    b.next() = ti;
	  }
      }
  }
};

//rom for H1
template<typename P>
struct rom_P_O
{
  P &pol;
  int j;

  rom_P_O(P &p)
    : pol(p)
  {
  }

  void restart()
  {
    j = 0;
  }

  bool full()
  {
    return j == P::degree;
  }

  typename P::value_type &next()
  {
    return pol(0, j++);
  }
};

template<typename P>
struct rom_P_T
{
  static constexpr size_t N = 256 / P::nbits;
  typedef typename P::value_type value_t;

  uint8_t *a;
  uint8_t a1[32];
  value_t state[N];
  int j, k;

  rom_P_T(uint8_t *b)
    : a(b)
  {
  }

  void restart()
  {
    j = 0;
    refresh();
  }

  void refresh()
  {
    a[32] = j;
    sha2(a1, a, 33);

    for (size_t i = 0; i < N; i++)
      {
	state[i] = 0;
	size_t offset = i * P::nbits;
	for (size_t l = 0; l < P::nbits; l++)
	  {
	    size_t nword = (offset + l) >> 3;
	    size_t nshift = (offset + l) & 7;
	    typename P::value_type bit = (a1[nword] >> nshift) & 1;
	    state[i] = state[i] | (bit << l);
	  }
      }

    j++;
    k = 0;
  }

  bool empty()
  {
    return k == N;
  }

  value_t next()
  {
    return state[k++];
  }

  bool filter(value_t v)
  {
    return v < P::get_modulus(0);
  }
};

template<typename P>
using rom1_t = rom_t<rom_P_O<P>, rom_P_T<P>>;

template<size_t k>
struct rom_k_O
{
  uint8_t *out;
  int j;

  rom_k_O(uint8_t *a)
    : out(a)
  {
  }

  void restart()
  {
    j = 0;
  }

  bool full()
  {
    return j == k;
  }

  uint8_t &next()
  {
    return out[j++];
  }
};

struct rom_k_T
{
  uint8_t *a;
  uint8_t a1[32];
  int j, k;

  typedef uint8_t value_t;

  rom_k_T(uint8_t *b)
    : a(b)
  {
  }

  void restart()
  {
    j = 0;
    refresh();
  }

  void refresh()
  {
    a[32] = j;
    sha2(a1, a, 33);

    j++;
    k = 0;
  }

  bool empty()
  {
    return k == 32;
  }

  uint8_t next()
  {
    return a1[k++];
  }

  bool filter(uint8_t x)
  {
    return true;
  }
};

template<size_t bbytes>
using rom2_t = rom_t<rom_k_O<bbytes>, rom_k_T>;

template<size_t rbytes, size_t bbytes>
using rom3_t = rom_t<rom_k_O<2 * rbytes + bbytes>,
		     rom_k_T>;

template<size_t rbytes>
using rom4_t = rom_t<rom_k_O<rbytes>, rom_k_T>;
#endif
