/* nuklear - 1.32.0 - public domain */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <limits.h>
#include <time.h>
#include <stdbool.h>
#include <GL/glew.h>
#include <omp.h>

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_INCLUDE_STANDARD_BOOL
#include "nuklear.h"
#include "nuklear_sdl_gl3.h"

#define FLO_PIXMAP_IMPLEMENTATION
#include "flo_pixmap.h"
#define FLO_MATRIX_IMPLEMENTATION
#include "flo_matrix.h"

#include "create_image.h"
#define FLO_FILE_IMPLEMENTATION
#include "flo_file.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#define FLO_SHADER_IMPLEMENTATION
#include "flo_shader.h"

#define WINDOW_WIDTH 2000
#define WINDOW_HEIGHT 256

#define TEXTURE_WIDTH 4096

#define STEREO 1

#define MAX_VERTEX_MEMORY 512 * 1024
#define MAX_ELEMENT_MEMORY 128 * 1024

void GLAPIENTRY
MessageCallback(GLenum source,
                GLenum type,
                GLuint id,
                GLenum severity,
                GLsizei length,
                const GLchar *message,
                const void *userParam)
{
    fprintf(stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
            (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""),
            type, severity, message);
}

typedef struct results
{
    flo_matrix_t *matrix_l;
    flo_matrix_t *matrix_r;
} result_t;

// Function to generate a simple image (random colors)
result_t generate_image(long size, const uint16_t raw_file[size], int fft_size, float overlap_percent)
{
    flo_matrix_t *result[2];

    // #pragma omp parallel
    for (int i = 0; i < 2; i++)
    {
        result[i] = create_image_meow(size, raw_file, 2, i, fft_size, overlap_percent);
    }

    return (result_t){.matrix_l = result[0], .matrix_r = result[1]};
}

typedef struct
{
    int num;
    GLuint texs[];
} textures_t;

textures_t *textures_alloc(int num_texs)
{
    textures_t *self = malloc(sizeof(textures_t) + num_texs * sizeof(GLuint));
    glGenTextures(num_texs, self->texs);
    self->num = num_texs;
    return self;
}

void textures_free(textures_t *t)
{
    if (t == NULL)
    {
        return;
    }
    glDeleteTextures(t->num, t->texs);
    free(t);
}

void calculate_texture(const result_t *result, const textures_t *textures, float sat, float lit)
{

    int num_tex_line = result->matrix_l->width / TEXTURE_WIDTH;
    /*
    textures_t *textures = textures_alloc( num_tex_line * 2);
    */
    int height = result->matrix_l->height;
    int width = result->matrix_l->width;

    flo_pixmap_t *p = flo_pixmap_create(TEXTURE_WIDTH, height);
    for (int i = 0; i < num_tex_line; i++)
    {
        for (int w = 0; w < TEXTURE_WIDTH; w++)
        {
            for (int h = 0; h < height; h++)
            {
                flo_pixmap_set_pixel(p, w, h, flo_hsvToRgb565(1.0f - flo_matrix_get_value(result->matrix_l, i * TEXTURE_WIDTH + w, h), sat, lit));
            }
        }
        glBindTexture(GL_TEXTURE_2D, textures->texs[i]);
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, TEXTURE_WIDTH, height, 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5,
                     p->buf);
    }

    for (int i = 0; i < num_tex_line; i++)
    {

        // #pragma omp parallel
        for (int w = 0; w < TEXTURE_WIDTH; w++)
        {
            for (int h = 0; h < height; h++)
            {
                flo_pixmap_set_pixel(p, w, h, flo_hsvToRgb565(1.0f - flo_matrix_get_value(result->matrix_r, i * TEXTURE_WIDTH + w, h), sat, lit));
            }
        }
        glBindTexture(GL_TEXTURE_2D, textures->texs[i + num_tex_line]);
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, TEXTURE_WIDTH, height, 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5,
                     &p->buf);
    }
    free(p);
}

/* ===============================================================
 *
 *                          DEMO
 *
 * ===============================================================*/
int main(int argc, char *argv[])
{
    /* Platform */
    SDL_Window *win;
    SDL_GLContext glContext;

    int win_width, win_height;
    int running = 1;
    if (argc < 2)
    {
        fprintf(stderr, "Need 1 filename argument\n");
        exit(1);
    }

    FILE *fp = fopen(argv[1], "r");
    if (!fp)
    {
        fprintf(stderr, "could not open %s\n", argv[1]);
        exit(1);
    }
    size_t size;
    uint16_t *raw_file = (uint16_t *)flo_mapfile(fp, &size);
    fclose(fp);
    if (!raw_file)
    {
        fprintf(stderr, "could not map %s\n", argv[1]);
        exit(1);
    }

    size /= 2;
    if (size % 2 == 1)
    {
        raw_file = NULL;
        fprintf(stderr, "could not map %s\n", argv[1]);
        exit(1);
    }

    /* GUI */
    struct nk_context *ctx = NULL;
    struct nk_colorf bg;

    /* SDL setup */
    SDL_SetHint(SDL_HINT_VIDEO_HIGHDPI_DISABLED, "0");
    // Prevent DBus memory leak
    SDL_SetHint(SDL_HINT_SHUTDOWN_DBUS_ON_QUIT, "1");

    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_EVENTS);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    win = SDL_CreateWindow("Demo",
                           SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                           WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_OPENGL | SDL_WINDOW_MAXIMIZED | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    glContext = SDL_GL_CreateContext(win);
    SDL_GetWindowSize(win, &win_width, &win_height);
    /* OpenGL setup */
    glViewport(0, 0, win_width, win_height);
    glewExperimental = 1;
    if (glewInit() != GLEW_OK)
    {
        fprintf(stderr, "Failed to setup GLEW\n");
        exit(1);
    }

    const char *vertex_p = flo_readall("vertex_shader.glsl");
    if (!vertex_p)
    {
        fprintf(stderr, "vertex_shader.glsl could not be read\n");
        exit(1);
    }

    const char *frag_p = flo_readall("fragment_shader.glsl");
    if (!frag_p)
    {
        fprintf(stderr, "fragment_shader.glsl could not be read\n");
        exit(1);
    }

    GLuint program = create_shader_program(vertex_p, frag_p);
    free((char *)frag_p);
    free((char *)vertex_p);

    ctx = nk_sdl_init(win);
    /* Load Fonts: if none of these are loaded a default font will be used  */
    /* Load Cursor: if you uncomment cursor loading please hide the cursor */
    {
        struct nk_font_atlas *atlas;

        nk_sdl_font_stash_begin(&atlas);
        struct nk_font *droid = nk_font_atlas_add_from_file(atlas, "DroidSans.ttf", 20, 0);
        /*struct nk_font *roboto = nk_font_atlas_add_from_file(atlas, "../../../extra_font/Roboto-Regular.ttf", 16, 0);*/
        /*struct nk_font *future = nk_font_atlas_add_from_file(atlas, "../../../extra_font/kenvector_future_thin.ttf", 13, 0);*/
        /*struct nk_font *clean = nk_font_atlas_add_from_file(atlas, "../../../extra_font/ProggyClean.ttf", 12, 0);*/
        /*struct nk_font *tiny = nk_font_atlas_add_from_file(atlas, "../../../extra_font/ProggyTiny.ttf", 10, 0);*/
        /*struct nk_font *cousine = nk_font_atlas_add_from_file(atlas, "../../../extra_font/Cousine-Regular.ttf", 13, 0);*/
        nk_sdl_font_stash_end();
        /*nk_style_load_all_cursors(ctx, atlas->cursors);*/
        nk_style_set_font(ctx, &droid->handle);
    }

    bg.r = 0.10f, bg.g = 0.18f, bg.b = 0.24f, bg.a = 1.0f;
    // During init, enable debug output
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(MessageCallback, 0);
    result_t result = generate_image(size, raw_file, 512, 0.1);

    bool changed = true;
    textures_t *textures = textures_alloc(result.matrix_l->width / TEXTURE_WIDTH * 2);
    float sat = 0.9f;
    float lit = 0.8f;
    bool next_wait = false;
    while (running)
    {
        /* Input */
        SDL_Event evt;
        nk_input_begin(ctx);
        if (next_wait)
        {
            SDL_WaitEvent(&evt);
            if (evt.type == SDL_QUIT)
            {
                goto cleanup;
            }
            nk_sdl_handle_event(&evt);
            next_wait = false;
        }
        while (SDL_PollEvent(&evt))
        {
            if (evt.type == SDL_QUIT)
            {
                goto cleanup;
            }
            nk_sdl_handle_event(&evt);
        }
        next_wait = true;

        nk_sdl_handle_grab(); /* optional grabbing behavior */
        nk_input_end(ctx);
        int height = result.matrix_l->height;
        int width = result.matrix_l->width;
        /* GUI */
        // Start a new UI frame
        if (nk_begin(ctx, "Image Display", nk_rect(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT * 2 + 40),
                     NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE | NK_WINDOW_MINIMIZABLE | NK_WINDOW_TITLE))
        {

            nk_layout_row_begin(ctx, NK_STATIC, 50, 2);
            {
                nk_layout_row_push(ctx, 50);
                nk_label(ctx, "Saturation:", NK_TEXT_LEFT);
                nk_layout_row_push(ctx, 110);
                changed = nk_slider_float(ctx, 0.0f, &sat, 1.0f, 0.1f) || changed;
            }
            nk_layout_row_end(ctx);

            if (changed)
            {
                /* if (textures != NULL) {
                    textures_free( textures);
                }*/

                calculate_texture(&result, textures, sat, lit);
                changed = false;
            }
            // Use nk_image to display the texture
            nk_layout_row_begin(ctx, NK_STATIC, height, textures->num / 2);
            for (int i = 0; i < textures->num / 2; i++)
            {
                nk_layout_row_push(ctx, TEXTURE_WIDTH);

                nk_image(ctx, nk_image_id(textures->texs[i]));
            }
            nk_layout_row_end(ctx);

            nk_layout_row_begin(ctx, NK_STATIC, height, textures->num / 2);
            for (int i = 0; i < textures->num / 2; i++)
            {
                nk_layout_row_push(ctx, TEXTURE_WIDTH);
                nk_image(ctx, nk_image_id(textures->texs[textures->num / 2 + i]));
            }
            nk_layout_row_end(ctx);

            struct nk_vec2 sz = nk_window_get_size(ctx);
        }
        nk_end(ctx);

        /* Draw */
        SDL_GetWindowSize(win, &win_width, &win_height);
        glViewport(0, 0, win_width, win_height);
        glClear(GL_COLOR_BUFFER_BIT);
        glClearColor(bg.r, bg.g, bg.b, bg.a);
        /* IMPORTANT: `nk_sdl_render` modifies some global OpenGL state
         * with blending, scissor, face culling, depth test and viewport and
         * defaults everything back into a default state.
         * Make sure to either a.) save and restore or b.) reset your own state after
         * rendering the UI. */
        nk_sdl_render(NK_ANTI_ALIASING_ON, MAX_VERTEX_MEMORY, MAX_ELEMENT_MEMORY);
        SDL_GL_SwapWindow(win);
    }

cleanup:
    free(result.matrix_l);
    free(result.matrix_r);
    textures_free(textures);
    nk_sdl_shutdown();
    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
