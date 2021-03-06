#include <iostream>
#include <nfl/poly_p.hpp>
#include "tools.h"

template <size_t degree, size_t modulus, class T>
bool run() {
  using poly_p = nfl::poly_p_from_modulus<T, degree, modulus>;
  using poly_t = nfl::poly_from_modulus<T, degree, modulus>;

  bool ret = true;

  // Define polynomials
  poly_p a{nfl::uniform()};
  poly_p b{nfl::uniform()};

#ifdef __LP64__
  poly_t& A = *alloc_aligned<poly_t, 64>(1, a.poly_obj());
  poly_t& B = *alloc_aligned<poly_t, 64>(1, b.poly_obj());
#else
  poly_t& A = *alloc_aligned<poly_t, 32>(1, a.poly_obj());
  poly_t& B = *alloc_aligned<poly_t, 32>(1, b.poly_obj());
#endif

  // Test addition
#ifdef __LP64__
  poly_t& add = *alloc_aligned<poly_t, 64>(1, A + B);
#else
  poly_t& add = *alloc_aligned<poly_t, 32>(1, A + B);
#endif
  poly_p add_p{a + b};
  ret &= (add_p == add);

  // Test substraction
#ifdef __LP64__
  poly_t& sub = *alloc_aligned<poly_t, 64>(1, A - B);
#else
  poly_t& sub = *alloc_aligned<poly_t, 32>(1, A - B);
#endif
  poly_p sub_p{a - b};
  ret &= (sub_p == sub);

  // Test multiplication
#ifdef __LP64__
  poly_t& mul = *alloc_aligned<poly_t, 64>(1, A * B);
#else
  poly_t& mul = *alloc_aligned<poly_t, 32>(1, A * B);
#endif
  poly_p mul_p{a * b};
  ret &= (mul_p == mul);

  // Test detach
  poly_p c{b};
  ret &= (c == b);

  // Test assign
  c = {1};
  ret &= (c != b);

  // Shoup
  poly_p bshoup = nfl::compute_shoup(b);
#ifdef __LP64__
  poly_t& Bshoup = *alloc_aligned<poly_t, 64>(1, nfl::compute_shoup(B));
#else
  poly_t& Bshoup = *alloc_aligned<poly_t, 32>(1, nfl::compute_shoup(B));
#endif
  ret &= (bshoup == Bshoup);

  poly_p mul2_p = nfl::shoup(a * b, bshoup);
#ifdef __LP64__
  poly_t& mul2 = *alloc_aligned<poly_t, 64>(1, nfl::shoup(A * B, Bshoup));
#else
  poly_t& mul2 = *alloc_aligned<poly_t, 32>(1, nfl::shoup(A * B, Bshoup));
#endif
  ret &= (mul2_p == mul2);

  // NTT and invNTT
  a.ntt_pow_phi();
  A.ntt_pow_phi();
  ret &= (a == A);

  b.invntt_pow_invphi();
  B.invntt_pow_invphi();
  ret &= (b == B);

  // Operators overloads with expression templates
  // (test == operator in both directions: poly_p == poly_t
  // and poly_t == poly_p)
  poly_p tmp_p = a + b * add_p;
  ret &= (tmp_p == A + B * add);
#ifdef __LP64__
  poly_t& tmp = *alloc_aligned<poly_t, 64>(1, A + B * add);
#else
  poly_t& tmp = *alloc_aligned<poly_t, 32>(1, A + B * add);
#endif
  ret &= (tmp == a + b * add_p);

  // Cleaning
  free_aligned(1, &add);
  free_aligned(1, &sub);
  free_aligned(1, &mul);
  free_aligned(1, &mul2);
  free_aligned(1, &Bshoup);
  free_aligned(1, &tmp);

  return ret;
}

int main(int argc, char const* argv[]) {
  return not run<CONFIG>();
}
