set(CUNIT_PREFIX "" CACHE PATH "The path to the previx of an CUNIT installation")

find_path(CUNIT_INCLUDE_DIR CUnit/CUnit.h
PATHS ${CUNIT_PREFIX}/include /usr/include /usr/local/include)

find_library(CUNIT_LIBRARY NAMES cunit libcunit cunitlib
PATHS ${CUNIT_PREFIX}/lib /usr/lib /usr/local/lib)

if(CUNIT_INCLUDE_DIR AND CUNIT_LIBRARY)
get_filename_component(CUNIT_LIBRARY_DIR ${CUNIT_LIBRARY} PATH)
set(CUNIT_FOUND TRUE)
endif()


if(CUNIT_FOUND)
if(NOT CUNIT_FIND_QUIETLY)
MESSAGE(STATUS "Found CUNIT: ${CUNIT_LIBRARY}")
endif()
elseif(CUNIT_FOUND)
if(CUNIT_FIND_REQUIRED)
message(FATAL_ERROR "Could not find CUNIT")
endif()
endif()
