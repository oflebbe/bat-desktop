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

// Manage the subtextures of a texture
typedef struct
{
    int num;
    int width;
    GLuint *left;
    GLuint *right;
    GLuint *correlation;
} textures_t;

textures_t textures_alloc(int num, int width)
{
    GLuint *left = calloc(num, sizeof(GLuint));
    if (!left)
    {
        return (textures_t){0};
    }

    GLuint *right = calloc(num, sizeof(GLuint));
    if (!right)
    {
        free(left);
        return (textures_t){0};
    }

    GLuint *correlation = calloc(num, sizeof(GLuint));
    if (!correlation)
    {
        free(left);
        free(right);
        return (textures_t){0};
    }

    glGenTextures(num, left);
    glGenTextures(num, right);
    glGenTextures(num, correlation);

    return (textures_t){.num = num, .width = width, .left = left, .right = right, .correlation = correlation};
}

void textures_free(textures_t t[static 1])
{
    glDeleteTextures(t->num, t->left);
    glDeleteTextures(t->num, t->right);
    glDeleteTextures(t->num, t->correlation);
    free(t->left);
    free(t->right);
    free(t->correlation);
}

static uint16_t grey(float v, float s, float l)
{
    return flo_hsvToRgb565(0.0f, 0.0f, v);
}

static void calculate_texture_line(const flo_matrix_t *mat, int num_tex_line, GLuint texture_id[num_tex_line], float sat, float lit, uint16_t (*f)(float, float, float))
{
    assert(mat);
    int height = mat->height;

    flo_pixmap_t *p = flo_pixmap_create(TEXTURE_WIDTH, height);
    for (int i = 0; i < num_tex_line; i++)
    {
        for (int w = 0; w < TEXTURE_WIDTH; w++)
        {
            for (int h = 0; h < height; h++)
            {
                flo_pixmap_set_pixel(p, w, h, f(1.0f - flo_matrix_get_value(mat, i * TEXTURE_WIDTH + w, h), sat, lit)); // flo_hsvToRgb565(1.0f - flo_matrix_get_value(mat, i * TEXTURE_WIDTH + w, h), sat, lit));
            }
        }
        glBindTexture(GL_TEXTURE_2D, texture_id[i]);
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, TEXTURE_WIDTH, height, 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5,
                     p->buf);
    }
    free(p);
}

static void calculate_texture(const stereo_result_t result[static 1], const textures_t textures[static 1], float sat, float lit)
{
    int num_tex_line = result->left->width / TEXTURE_WIDTH;
    assert(num_tex_line <= textures->num);

    calculate_texture_line(result->left, num_tex_line, textures->left, sat, lit, flo_hsvToRgb565);

    calculate_texture_line(result->right, num_tex_line, textures->right, sat, lit, flo_hsvToRgb565);

    calculate_texture_line(result->correlation, num_tex_line, textures->correlation, sat, lit, grey);
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
    int rw = 0, rh = 0;
    SDL_Renderer *render = SDL_GetRenderer(win);
    SDL_GetRendererOutputSize(render, &rw, &rh);
    char *ptr = SDL_GetError();

    float widthScale = (float)rw / (float)win_width;
    float heightScale = (float)rh / (float)win_height;
    if (widthScale != heightScale)
    {
        fprintf(stderr, "WARNING: width scale != height scale\n");
    }

    SDL_RenderSetScale(render, widthScale, heightScale);

    /* OpenGL setup */
    glViewport(0, 0, win_width, win_height);
    glewExperimental = 1;
    if (glewInit() != GLEW_OK)
    {
        fprintf(stderr, "Failed to setup GLEW\n");
        exit(1);
    }

    ctx = nk_sdl_init(win);
    /* Load Fonts: if none of these are loaded a default font will be used  */
    /* Load Cursor: if you uncomment cursor loading please hide the cursor */
    {
        struct nk_font_atlas *atlas;

        nk_sdl_font_stash_begin(&atlas);
        struct nk_font *droid = nk_font_atlas_add_from_file(atlas, "DroidSans.ttf", 20, 0);
        nk_sdl_font_stash_end();
        /*nk_style_load_all_cursors(ctx, atlas->cursors);*/
        nk_style_set_font(ctx, &droid->handle);
    }

    bg.r = 0.10f, bg.g = 0.18f, bg.b = 0.24f, bg.a = 1.0f;
    // During init, enable debug output
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(MessageCallback, 0);
    stereo_result_t result = create_stereo_image_meow(size, raw_file, 512, 0.1);

    bool changed = true;
    textures_t textures = textures_alloc(result.left->width / TEXTURE_WIDTH, TEXTURE_WIDTH);
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
        int height = result.left->height;
        int width = result.left->width;

        /* GUI */
        // Start a new UI frame
        if (nk_begin(ctx, "Image Display", nk_rect(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT * 3 + 100),
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
                calculate_texture(&result, &textures, sat, lit);
                changed = false;
            }
            ctx->style.window.spacing = nk_vec2(0.f, 0.f);
            // Use nk_image to display the texture
            nk_layout_row_static(ctx, height, textures.width, textures.num);
            for (int i = 0; i < textures.num; i++)
            {
                nk_image(ctx, nk_image_id(textures.left[i]));
            }
            nk_layout_row_end(ctx);

            nk_layout_row_static(ctx, height, textures.width, textures.num);
            for (int i = 0; i < textures.num; i++)
            {
                nk_image(ctx, nk_image_id(textures.right[i]));
            }
            nk_layout_row_end(ctx);

            nk_layout_row_static(ctx, height, textures.width, textures.num);
            for (int i = 0; i < textures.num; i++)
            {
                nk_image(ctx, nk_image_id(textures.correlation[i]));
            }
            nk_layout_row_end(ctx);
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
    free(result.left);
    free(result.right);
    free(result.correlation);

    textures_free(&textures);
    nk_sdl_shutdown();
    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
