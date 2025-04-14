cmake_minimum_required(VERSION 3.16)
#
project(bat_desktop LANGUAGES  C)


SET(CMAKE_C_FLAGS_RELEASE "-O3")
SET(CMAKE_C_FLAGS_RELWITHDEBINFO "-O2" "-g")

find_package(SDL2 REQUIRED)

add_compile_options("-std=gnu23" "-Wall" "-pedantic" "-fopenmp") 
add_compile_options("-Wformat" "-Wformat=2" "-Wsign-conversion" "-Werror=format-security" "-Wimplicit-fallthrough")
#add_compile_options("-fstack-clash-protection" "-fstack-protector-strong" )
add_compile_options("-Werror=implicit" "-Werror=incompatible-pointer-types" "-Werror=int-conversion")
add_compile_options("-fexceptions")
#add_compile_options( "-fsanitize=address" "-fsanitize=leak"  "-fsanitize=undefined" "-fsanitize-address-use-after-scope"  )


add_link_options("-fopenmp")
#add_link_options("-fstack-clash-protection" "-fstack-protector-strong" )
add_link_options("-Wl,-z,nodlopen" "-Wl,-z,noexecstack" "-Wl,-z,relro" "-Wl,-z,now"  "-Wl,--as-needed" "-Wl,--no-copy-dt-needed-entries")
#add_link_options("-fsanitize=address" "-fsanitize=leak" "-fsanitize=undefined" "-fsanitize-address-use-after-scope")
add_executable(bat_desktop main.c lib.c create_image.h create_image_meow.c flo_shader.h flo_pixmap.h flo_file.h)
target_link_libraries(bat_desktop PRIVATE SDL2::SDL2 m)


add_executable(test1 test1.c create_image.h create_image_meow.c flo_pixmap.h flo_file.h)
target_link_libraries(test1 PRIVATE m)

