cmake_minimum_required(VERSION 3.10)
if (CMAKE_BINARY_DIR STREQUAL CMAKE_SOURCE_DIR)
  message(FATAL_ERROR "Building in-source is not supported! Create a build dir and remove ${CMAKE_SOURCE_DIR}/CMakeCache.txt")
endif ()

option(test "Build all tests." ON)

project(NPM VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(LIB
        src/headers/dependency.h
        src/format/package_lock.hpp
        src/proto/http.hpp
        src/util/fs.hpp
        src/util/args.hpp
        src/format/tar.hpp
        src/headers/tar_header.h src/util/regex.h)

set(TP_LIB src/util/thread_pool.hpp)
set(GZIP_LIB
        src/format/gzip/huffman_decoder.h
        src/format/gzip/huffman_decoder.cc
        src/format/gzip/bit_reader.h
        src/format/gzip/bit_reader.cc
        src/format/gzip/decompressor.h
        src/format/gzip/decompressor.cc
)

set(INCLUDES ${LIB} ${GZIP_LIB} ${TP_LIB})

add_executable(npmci src/npmci.cpp ${INCLUDES})
if(WIN32)
  target_link_libraries(npmci wsock32 ws2_32)
else()
  target_link_libraries(npmci pthread pthread)
endif()

#include(CPack)
#set(CPACK_GENERATOR "ZIP")

enable_testing()
add_subdirectory(./test)

