cmake_minimum_required(VERSION 2.8)
project(pvw CXX)

find_package(OpenSSL REQUIRED)
find_package(Boost REQUIRED)
find_package(CUNIT REQUIRED)

include_directories(include ../thirdparty/BLAKE3/c ${Boost_INCLUDE_DIR}
  ${OPENSSL_INCLUDE_DIR} ${CUNIT_INCLUDE_DIR})

file(GLOB RELART_HDRS_0 include/*.hpp)

install(FILES ${RELART_HDRS_0} DESTINATION include)

file(GLOB RELART_SRC_0 src/*)

add_executable(pvw ${RELART_SRC_0})
target_link_libraries(pvw blake3_static ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES} ${CUNIT_LIBRARY})
