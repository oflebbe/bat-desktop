#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_INCLUDE_STANDARD_BOOL
#define NK_INCLUDE_COMMAND_USERDATA
#define NK_IMPLEMENTATION
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include "nuklear.h"
#pragma GCC diagnostic pop

#define GLAD_GL_IMPLEMENTATION
#include "glad.h"

#define NK_SDL_GL3_IMPLEMENTATION
#include "nuklear_sdl_gl3.h"
