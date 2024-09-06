cmake_minimum_required(VERSION 3.16)

project(bat_desktop LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS_RELEASE "-O3" )



find_package(PkgConfig REQUIRED )
pkg_search_module(FFTW REQUIRED fftw3f IMPORTED_TARGET)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)
find_package(FMT REQUIRED fmt)
set(CMAKE_AUTOMOC ON)

qt_standard_project_setup()

add_compile_options("-Wall" )



qt_add_executable(bat_desktop mainf.cpp mylabel.h mylabel.cpp)

target_link_libraries(bat_desktop PRIVATE PkgConfig::FFTW Qt6::Core Qt6::Gui Qt6::Widgets fmt)