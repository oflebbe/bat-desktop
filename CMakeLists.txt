cmake_minimum_required(VERSION 3.13)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_CXX_FLAGS_RELEASE "-O3" )

project(bat_desktop CXX)

add_compile_options("-Wall" )

find_package(PkgConfig REQUIRED)
pkg_search_module(FFTW REQUIRED fftw3f IMPORTED_TARGET)
include_directories(PkgConfig::FFTW)
link_libraries(PkgConfig::FFTW)

add_executable(bat_desktop mainf.cpp)

