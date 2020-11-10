#ifndef __COMM_HPP__
#define __COMM_HPP__

#include <cstdint>
#include "symenc.hpp"

#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>

#include <nfl.hpp>
#include "symenc.hpp"


template<typename P, size_t rbytes>
struct comm_msg_1_t
{
  uint32_t sid;
  P p0;
  uint8_t r_sid[sizeof(sid) + rbytes];

  template<class Archive>
  void serialize(Archive & ar, const unsigned int version)
  {
    ar & sid;
    ar & p0;
    ar & r_sid;
  }
};

template<typename P, size_t rbytes, size_t bbytes, typename cipher_t>
struct comm_msg_2_t
{
  uint32_t sid;
  P pS;
  P signal0, signal1;
  cipher_t a0, a1;
  uint8_t u0[2*rbytes + bbytes],
    u1[2*rbytes + bbytes];

  template<class Archive>
  void serialize(Archive &ar,
                 const unsigned int version)
  {
    ar & sid;
    ar & pS;
    ar & signal0;
    ar & signal1;
    ar & a0;
    ar & a1;
    ar & u0;
    ar & u1;
  }
};

template<size_t rbytes>
struct comm_msg_3_t
{
  uint8_t ch[rbytes];
  uint32_t sid;
  template<class Archive>
  void serialize(Archive &ar,
                 const unsigned int version)
  {
    ar & sid;
    ar & ch;
  }
};

template<typename cipher_t>
struct comm_msg_4_t
{
  cipher_t c0, c1;
  uint32_t sid;

  template<class Archive>
  void serialize(Archive &ar,
                 const unsigned int version)
  {
    ar & c0;
    ar & c1;
    ar & sid;
  }
};

#endif
