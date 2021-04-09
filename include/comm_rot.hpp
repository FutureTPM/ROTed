#ifndef __COMM_ROT_HPP__
#define __COMM_ROT_HPP__

#include <cstdint>
#include "symenc.hpp"

// #include <boost/archive/text_oarchive.hpp>
// #include <boost/archive/text_iarchive.hpp>

#include <nfl.hpp>

template<typename P, size_t rbytes, size_t HASHSIZE>
struct comm_rot_msg_1_t
{
  uint32_t sid;
  P p0;
  uint8_t r_sid[sizeof(sid) + rbytes];
  uint8_t hS0[HASHSIZE], hS1[HASHSIZE];

  // template<class Archive>
  // void serialize(Archive & ar, const unsigned int version)
  // {
  //   ar & sid;
  //   ar & p0;
  //   ar & r_sid;
  //   ar & hS0;
  //   ar & hS1;
  // }
};

template<typename P, size_t bbytes, size_t HASHSIZE>
struct comm_rot_msg_2_t
{
  uint32_t sid;
  P pS;
  P signal0, signal1;
  uint8_t u[bbytes];
  uint8_t hma0[HASHSIZE],
    hma1[HASHSIZE];

  // template<class Archive>
  // void serialize(Archive &ar,
  //                const unsigned int version)
  // {
  //   ar & sid;
  //   ar & pS;
  //   ar & signal0;
  //   ar & signal1;
  //   ar & hma0;
  //   ar & hma1;
  //   ar & u;
  // }
};

template<size_t bbytes>
struct comm_rot_msg_3_t
{
  uint8_t S0[bbytes], S1[bbytes];
  uint32_t sid;
  // template<class Archive>
  // void serialize(Archive &ar,
  //                const unsigned int version)
  // {
  //   ar & S0;
  //   ar & S1;
  // }
};

#endif
