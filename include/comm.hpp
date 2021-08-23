#ifndef __COMM_HPP__
#define __COMM_HPP__

#include <cstdint>
#include <nfl.hpp>
#include "symenc.hpp"

/**
@file

Structures encoding the messages defined in [BDGM19]

[BDGM19] Pedro Branco, Jintai Ding, Manuel Goulao, and Paulo Mateus.
A frameworkfor universally composable oblivious transfer from one-round key-exchange.
In Martin Albrecht, editor, Cryptography and Coding, pages 78–101, Cham,2019.
Springer International Publishing
*/

/** First message of [BDGM19] OT

@tparam P NFL Polynomial type
@tparam rbytes Size of random seed
*/
template<typename P, size_t rbytes>
struct comm_msg_1_t
{
  /** Session ID */
  uint32_t sid;
  /** Receiver's RLWE sample for "branch 0" */
  P p0;
  /** Seed for generation of common polynomial */
  uint8_t r_sid[sizeof(sid) + rbytes];
};

/** Second message of [BDGM19] OT

@tparam P NFL Polynomial type
@tparam rbytes Size of random seed
@tparam bbytes Size of symmetric key
@tparam cipher_t Symmetric-key encryption type
*/
template<typename P, size_t rbytes, size_t bbytes, typename cipher_t>
struct comm_msg_2_t
{
  /** Session id */
  uint32_t sid;
  /** Sender's RLWE sample */
  P pS;
  /**@{*/
  /** Signal as per [DXL12]

[DXL12] Jintai Ding, Xiang Xie, and Xiaodong Lin. A simple provably
secure key exchange scheme based on the learning with errors
problem. Cryptology ePrint Archive, Report 2012/688, 2012.
https://eprint.iacr.org/2012/688.*/
  P signal0, signal1;
  /**@}*/

  /**@{*/
  /** Challenge similar based on [BDD+17]

[BDD+17] Paulo S. L. M. Barreto, Bernardo David, Rafael Dowsley, Kirill Morozov, and Anderson C. A. Nascimento.
A framework for efficient adaptively secure composable oblivious transfer in the rom. Cryptology ePrint
Archive, Report 2017/993, 2017. https://eprint.iacr.org/2017/993, Version 21 December 2017.
*/
  cipher_t a0, a1;
  uint8_t u0[2*rbytes + bbytes],
    u1[2*rbytes + bbytes];
  /**@}*/
};

/** Third message of [BDGM19] OT

@tparam rbytes Size of random seed
*/
template<size_t rbytes>
struct comm_msg_3_t
{
  /** Challenge response */
  uint8_t ch[rbytes];
  /** Session ID */
  uint32_t sid;
};

template<typename cipher_t>
struct comm_msg_4_t
{
  /**@{*/
  /** Messages encrypted under keys shared under RLWE channels 0, 1 */
  cipher_t c0, c1;
  /**@}*/

  /** Session ID */
  uint32_t sid;
};

#endif
