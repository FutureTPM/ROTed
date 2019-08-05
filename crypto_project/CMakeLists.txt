cmake_minimum_required(VERSION 2.8)
project(nfl C)

include_directories(include)

file(GLOB CRYPTO_SRCS lib/*.c)
file(GLOB CRYPTO_HDRS include/*.h)

add_library(cryptolib SHARED ${CRYPTO_SRCS})
install(TARGETS cryptolib LIBRARY DESTINATION lib)

add_library(cryptolib_static STATIC ${CRYPTO_SRCS})
install(TARGETS cryptolib_static ARCHIVE DESTINATION lib)

install(FILES ${CRYPTO_} DESTINATION include)
install(FILES ${CRYPTO_HDRS} DESTINATION include)
