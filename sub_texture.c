#include <stdlib.h>
#include <stddef.h>
#include "sub_texture.h"
#include "flo_matrix.h"

// return width of n-th texture tile (0..<num)
unsigned int sub_texture_width(const sub_texture_t *t, unsigned int n)
{
    if (!t)
    {
        return 0;
    }
    if (n + 1 < t->num)
    {
        return SUB_TEXTURE_WIDTH;
    }
    return t->width % SUB_TEXTURE_WIDTH;
}

// alloc num subtextures
const sub_texture_t *sub_texture_init(const flo_matrix_t *p)
{
    if (!p) {
        return nullptr;
    }
    int num = (int)(p->width / SUB_TEXTURE_WIDTH);
    if (num % SUB_TEXTURE_WIDTH != 0) {
        num++;
    }
    size_t len = sizeof(sub_texture_t) + (size_t) num * sizeof(GLuint);
    sub_texture_t *self = calloc(len, 1);
    assert(self);
    *self = (sub_texture_t){.num = num, .width = p->width, .height = p->height};

    glGenTextures(num, self->sub_texture_id);
    
    for (size_t i = 0; i < num; i++)
    {
        glBindTexture(GL_TEXTURE_2D, self->sub_texture_id[i]);
        glPixelStorei(GL_PACK_ALIGNMENT, 4);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, (int) p->width);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
        GLsizei t_width = (GLsizei) sub_texture_width(self, i); // last one is usually smaller

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, t_width, p->height, 0, GL_RED, GL_FLOAT, p->buf + i * t_width);
       //     glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, t_width, p->height, GL_RED, GL_FLOAT,p->buf + i * t_width);
    }
    return self;
}


void sub_texture_display(struct nk_context *ctx, const sub_texture_t *t, bool black_white, float mag, float rot)
{
    if (!t || !ctx) {
        return;
    }
    unsigned int w = sub_texture_width(t, 0);
    nk_layout_row_static(ctx, t->height * mag, (int)(w * mag), (int)t->num);
    for (size_t i = 0; i < t->num; i++)
    {
        nk_set_user_data(ctx, (nk_handle){.id = black_white ? 2 : 1} );
        nk_image_color(ctx, nk_image_id((int)t->sub_texture_id[i]), nk_rgba_f(rot, 0., 0., 1.));
    }
}

void sub_texture_release(const sub_texture_t *t)
{
    if (!t)
    {
        return;
    }
    glDeleteTextures(t->num, t->sub_texture_id);
    free((void *)t);
}
