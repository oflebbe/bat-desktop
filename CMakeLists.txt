cmake_minimum_required(VERSION 3.16)
#
project(bat_desktop LANGUAGES  C)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
find_package(SDL2 REQUIRED)
find_package(GLEW REQUIRED)
add_compile_options("-Wall" "-pedantic" "-fopenmp" "-fsanitize=address" "-fsanitize=leak" "-fsanitize=undefined" "-fsanitize-address-use-after-scope")
add_link_options("-fopenmp" "-fsanitize=address" "-fsanitize=leak" "-fsanitize=undefined" "-fsanitize-address-use-after-scope")
add_executable(bat_desktop main.c lib.c create_image.h create_image_meow.c create_image_cross.c flo_pixmap.h flo_file.h)
target_link_libraries(bat_desktop PRIVATE ${SDL2_LIBRARIES} GLEW::GLEW GL m)
# target_link_libraries(bat_desktop PRIVATE Threads::Threads)

