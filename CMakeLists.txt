cmake_minimum_required(VERSION 2.8)
project(GridWorks)

set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_C_COMPILER clang)

set(SHARED_FLAGS "-DDEBUG -Wall -Wextra -O0 -g -fcolor-diagnostics")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${SHARED_FLAGS} -stdlib=libc++ -std=c++11 -fpic -DMKL_ILP64")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${SHARED_FLAGS} -std=c11")

include_directories(
  core
  /opt/intel/mkl/include
)

link_directories(
  /opt/intel/mkl/lib/intel64
  /opt/intel/lib/intel64
) 

set(MKL_LIBS mkl_intel_ilp64 mkl_core mkl_intel_thread iomp5)

add_subdirectory(core)
add_subdirectory(examples)
