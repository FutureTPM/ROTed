cmake_minimum_required(VERSION 2.8)
project(rel_art CXX)

find_package(OpenSSL REQUIRED)
find_package(Boost REQUIRED)

include_directories(include)

file(GLOB RELART_HDRS_0 include/*.hpp include/cpucycles.h)

install(FILES ${RELART_HDRS_0} DESTINATION include)

file(GLOB RELART_SRC_0 src/*)

add_executable(main_rel ${RELART_SRC_0})
target_link_libraries(main_rel ${OPENSSL_LIBRARIES} ${CUNIT_LIBRARY})
