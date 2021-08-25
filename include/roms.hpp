#ifndef __ROMS_HPP__
#define __ROMS_HPP__
#include "blake3.h"
#include <algorithm>

/** Provides an interface to Blake3

    @param out Outputted hash
    @param in Inputted string
    @param len Lenght of in
*/
void blake3(uint8_t *out, const uint8_t *in, size_t len)
{
  blake3_hasher hasher;
  blake3_hasher_init(&hasher);
  blake3_hasher_update(&hasher, in, len);
  blake3_hasher_finalize(&hasher, out, BLAKE3_OUT_LEN);
}

/** Implements a Random Oracle. T provides functionality
    to handle internal buffer such as selecting valid outputs.
    O wraps the output buffer

    @tparam O Wraps output buffer
    @tparam T Processes inner buffer according to output
*/
template<typename O,
	 typename T>
struct rom_t
{
  /** Buffer holding temporary "ctr" for generating ROM output */
  uint8_t a[33];
  /** Manipulates 'a' and feeds values into the output */
  T tmp_buffer;

  /** ROM constructor */
  rom_t()
    : tmp_buffer(a)
  {
  }

  /** Produces random oracle in polynomial/array wrapped by b from data

      @param b Outputted value
      @param data ROM input
      @param len Lenght of ROM input
  */
  void operator()(O &b, const uint8_t *data, size_t len)
  {
    blake3(a, data, len);
    b.restart();
    tmp_buffer.restart();

    while (!b.full()) {
        if(tmp_buffer.empty())
            tmp_buffer.refresh();

        typename T::value_t ti = tmp_buffer.next();
        if (tmp_buffer.filter(ti)) {
            b.next() = ti;
        }
    }
  }
};

/** To be used as O for rom_t with P as co-domain

    @tparam P NFL Polynomial type
*/
template<typename P>
struct rom_P_O
{
  /** Reference to polynomial which will hold output of ROM */
  P &pol;
  /** Counter of how many coefficients have been filled */
  int j;

  /** Initializes reference to output polynomial

      @param p Polynomial ROM output
  */
  rom_P_O(P &p)
    : pol(p)
  {
  }

  /** Reinitializes coefficient counter */
  void restart()
  {
    j = 0;
  }

  /** Used to determine when polynomial has been filled

      @return True when polynomial has been filled
  */
  bool full()
  {
    return j == P::degree;
  }

  /** Returns reference to next coefficient to be filled

      @return Reference to next coefficient to be filled
  */
  typename P::value_type &next()
  {
    return pol(0, j++);
  }
};

/** To be used as T for rom_t with P as co-domain

    @tparam P NFL Polynomial type
*/
template<typename P>
struct rom_P_T
{
  /** Indicates how many polynomial coefficients 
      the output of a single Blake3 iteration can fill */
  static constexpr size_t N = 256 / P::nbits;
  /** Type of polynomial coefficient */
  typedef typename P::value_type value_t;

  /** Pointer to internal rom_t array */
  uint8_t *a;
  /** Holds intermediate hashes, computed as HASH(a || j) */
  uint8_t a1[32];
  /** Holds hash-output split into coefficient-size words */
  value_t state[N];
  /** j is used as part of the iterative hashing process
      (namely to compute HASH(a || j) */
  int j;
  /** Counts how many words have been outputted. When k reaches
      N, j is incremented and a1/state are recomputed */
  int k;
  
  /** Initializes pointer to internal rom_t array

      @param b Internal rom_t array */
  rom_P_T(uint8_t *b)
    : a(b)
  {
  }

  /** Reinitializes counter j and starts ROMing process */
  void restart()
  {
    j = 0;
    refresh();
  }

  /** Computes state <- SplitIntoCoeffSizeWords(HASH(a || j)) and
      increments j
  */
  void refresh()
  {
    a[32] = j;
    blake3(a1, a, 33);

    for (size_t i = 0; i < N; i++) {
        state[i] = 0;
        size_t offset = i * P::nbits;
        for (size_t l = 0; l < P::nbits; l++) {
            size_t nword = (offset + l) >> 3;
            size_t nshift = (offset + l) & 7;
            typename P::value_type bit = (a1[nword] >> nshift) & 1;
            state[i] = state[i] | (bit << l);
        }
    }

    j++;
    k = 0;
  }

  /** Returns true when no more words can be outputted without calling
      refresh again

      @return True when all words in state have been consumed */
  bool empty()
  {
    return k == N;
  }

  /** Returns next coefficient-size word produced

      @return ROM coefficient-size word */
  value_t next()
  {
    return state[k++];
  }

  /** Returns true when v is a valid coefficient
      (non-negative valid < p)

      @param v Tested value
      @return True when v < p
  */
  bool filter(value_t v)
  {
    return v < P::get_modulus(0);
  }
};

/** Random oracle to output polynomials */
template<typename P>
using rom1_t = rom_t<rom_P_O<P>, rom_P_T<P>>;

/** Used as O in rom_t when output is an array

    @tparam k Size of outputted array
*/
template<size_t k>
struct rom_k_O
{
  /** Pointer to array holding the output of ROM */
  uint8_t *out;
  /** Counter of filled coefficients */
  int j;

  /** Initializes pointer to output

      @param a Pointer to ROM array output */
  rom_k_O(uint8_t *a)
    : out(a)
  {
  }

  /** Reinitializes j */
  void restart()
  {
    j = 0;
  }

  /** Returns true when the entire array has been filled

      @return True when array has been filled */
  bool full()
  {
    return j == k;
  }

  /** Returns reference to next array coefficient. Updates j

      @return Reference to next array element */
  uint8_t &next()
  {
    return out[j++];
  }
};

/** To b used as T in rom_t when output is array */
struct rom_k_T
{
  /** Pointer to internal rom_t array */
  uint8_t *a;
  /** Used as intermediate state (i.e. HASH(a || j)) */
  uint8_t a1[32];
  /** Contains the value of the current iteration in the ROM
      hashing process */
  int j;
  /** Counts words of a1 consumed */
  int k;

  /** Type of a1 coefficients */
  typedef uint8_t value_t;

  /** Initializes pointer to internal rom_t array

      @param b Pointer to internal rom_t array */
  rom_k_T(uint8_t *b)
    : a(b)
  {
  }

  /** Resets j = 0, and kicks off iterative hashing process */
  void restart()
  {
    j = 0;
    refresh();
  }

  /** Computes a1 <- HASH(a || j) and increments j */
  void refresh()
  {
    a[32] = j;
    blake3(a1, a, 33);

    j++;
    k = 0;
  }

  /** Array is considered empty when all words have been consumed

      @return True when all words have been consumed */
  bool empty()
  {
    return k == 32;
  }

  /** Returns next word of a1 for consumption

      @return Next word of ROM */
  uint8_t next()
  {
    return a1[k++];
  }

  /** No filtering is needed

      @return Always true */
  bool filter(uint8_t x)
  {
    return true;
  }
};

/** ROM for array with output of size bbytes */
template<size_t bbytes>
using rom2_t = rom_t<rom_k_O<bbytes>, rom_k_T>;

/** ROM for array with output of size 2 * rbytes + bbytes */
template<size_t rbytes, size_t bbytes>
using rom3_t = rom_t<rom_k_O<2 * rbytes + bbytes>,
		     rom_k_T>;

/** ROM for array with output of size rbytes (same as rom2_t) */
template<size_t rbytes>
using rom4_t = rom_t<rom_k_O<rbytes>, rom_k_T>;
#endif
