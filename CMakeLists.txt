cmake_minimum_required(VERSION 3.16)

project(bat_desktop LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS_RELEASE "-O3" )
set(CMAKE_CXX_FLAGS_DEBUG "-fsanitize=address -fsanitize=leak -fsanitize=undefined -fsanitize-address-use-after-scope" )
find_package(PkgConfig REQUIRED )

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)


find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)
set(CMAKE_AUTOMOC ON)

qt_standard_project_setup()

add_compile_options("-Wall" )

qt_add_executable(bat_desktop bat_desktop.cpp create_image.h window.h window.cpp renderarea.h renderarea.cpp  create_image_meow.c flo_pixmap.h flo_file.h)
target_link_libraries(bat_desktop PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets)
# target_link_libraries(bat_desktop PRIVATE Threads::Threads)

