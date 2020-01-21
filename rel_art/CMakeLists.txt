cmake_minimum_required(VERSION 2.8)
project(rel_art CXX)

find_package(OpenSSL REQUIRED)

include_directories(include)

file(GLOB RLWEOT_HDRS_0 include/*.hpp)

install(FILES ${REL_ART_HDRS_0} DESTINATION include)

add_executable(main_rel src/main.cpp)
target_link_libraries(main_rel ${OPENSSL_LIBRARIES})
