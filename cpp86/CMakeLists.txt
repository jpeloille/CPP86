cmake_minimum_required(VERSION 3.17)
project(x86__)

set(CMAKE_CXX_STANDARD 14)

add_executable(x86__ main.cpp src/intel.h src/iapx86.cpp src/iapx86.h)