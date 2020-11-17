cmake_minimum_required(VERSION 2.8)
project(rlweot CXX)

if(APPLE)
  cmake_policy(SET CMP0042 NEW)
endif()

include(CMakeCompilers.txt)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
find_package(GMP REQUIRED)
find_package(MPFR REQUIRED)
find_package(CUNIT REQUIRED)
find_package(Boost REQUIRED serialization)
find_package(OpenSSL REQUIRED)

#add_subdirectory(crypto_project)
add_subdirectory(nfl)
add_subdirectory(rel_art)
add_subdirectory(BLAKE3/c)

include_directories(include nfl/include
    # crypto_project/include
    ${Boost_INCLUDE_DIR} BLAKE3/c ${OpenSSL_INCLUDE_DIR})

file(GLOB RLWEOT_HDRS_0 include/*.hpp include/cpucycles.h)

install(FILES ${RLWEOT_HDRS_0} DESTINATION include)

add_executable(main src/main.cpp)
target_link_libraries(main nfllib_static
    #cryptolib_static
    blake3_static ${GMP_LIBRARY} ${MPFR_LIBRARY} ${CUNIT_LIBRARY} ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES})
