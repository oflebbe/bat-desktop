#ifndef SUB_TEXTURE_H
#define SUB_TEXTURE_H

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_INCLUDE_STANDARD_BOOL
#define NK_INCLUDE_COMMAND_USERDATA

#include "nuklear.h"
#include "glad.h"

#include "flo_matrix.h"

// maximum Texture Width
#define SUB_TEXTURE_WIDTH 4096

// Manage the subtextures of a texture
typedef struct
{
    int num;
    unsigned int width; // complete image
    unsigned int height;
    GLuint sub_texture_id[]; // flexible array member of texture_ids
} sub_texture_t;


unsigned int sub_texture_width(const sub_texture_t *t, unsigned int n);

// allocates and init sub_texture, return NULL if p NULL
const sub_texture_t *sub_texture_init(const flo_matrix_t *p);
void sub_texture_display(struct nk_context *ctx, const sub_texture_t *t, bool black_white, float mag, float rot);
// free subtextures
void sub_texture_release(const sub_texture_t *t);
#endif
