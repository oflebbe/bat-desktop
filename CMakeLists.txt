cmake_minimum_required(VERSION 3.16)
#
project(bat_desktop LANGUAGES  C)

find_package(SDL2 REQUIRED)

#add_compile_options( "-fsanitize=address" "-fsanitize=leak" "-fsanitize=undefined" "-fsanitize-address-use-after-scope"  )
add_compile_options("-std=c23" "-Wall" "-pedantic" "-fopenmp")
#   add_link_options("-fsanitize=address" "-fsanitize=leak" "-fsanitize=undefined" "-fsanitize-address-use-after-scope")
add_link_options("-fopenmp"  )
add_executable(bat_desktop main.c lib.c create_image.h create_image_meow.c flo_shader.h flo_pixmap.h flo_file.h)
target_link_libraries(bat_desktop PRIVATE SDL2::SDL2 m)


add_executable(test1 test1.c create_image.h create_image_meow.c flo_pixmap.h flo_file.h)
target_link_libraries(test1 PRIVATE m)

