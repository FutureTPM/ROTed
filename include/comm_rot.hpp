#ifndef __COMM_ROT_HPP__
#define __COMM_ROT_HPP__

#include <cstdint>
#include "symenc.hpp"
#include <nfl.hpp>

/**
@file

Structures encoding the messages defined in "ROTed: Random Oblivious Transfer for embedded devices"

*/

/** First message of "ROTed: Random Oblivious Transfer for embedded devices" ROT

@tparam P NFL Polynomial type
@tparam rbytes Size of random seed
@tparam HASHSIZE Size of hash output
*/
template<typename P, size_t rbytes, size_t HASHSIZE>
struct comm_rot_msg_1_t
{
  /** Session ID */
  uint32_t sid;
  /** Receiver's RLWE sample for "branch 0" */
  P p0;
  /** Seed for generation of common polynomial */
  uint8_t r_sid[sizeof(sid) + rbytes];

  /**@{*/
  /** Commitment to receiver's random masks */
  uint8_t hS0[HASHSIZE], hS1[HASHSIZE];
  /**@}*/
};

/** Second message of "ROTed: Random Oblivious Transfer for embedded devices" ROT

@tparam P NFL Polynomial type
@tparam bbytes Size of input to message random oracle
@tparam HASHSIZE Size of hash output
*/
template<typename P, size_t bbytes, size_t HASHSIZE>
struct comm_rot_msg_2_t
{
  /** Session ID */
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

  /** Random sender's mask */
  uint8_t u[bbytes];
  
  /**@{*/
  /** Commitments to messages exchanged via channels 0,1 */
  uint8_t hma0[HASHSIZE],
    hma1[HASHSIZE];
  /**@}*/
};

/** Third message of "ROTed: Random Oblivious Transfer for embedded devices" ROT

@tparam bbytes Size of input to message random oracle
*/
template<size_t bbytes>
struct comm_rot_msg_3_t
{
  /**@{*/
  /** Receiver's masks */
  uint8_t S0[bbytes], S1[bbytes];
  /**@}*/

  /** Session ID */
  uint32_t sid;
};

#endif
