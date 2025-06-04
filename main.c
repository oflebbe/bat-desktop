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

// Using OpenMP
#include <omp.h>

#include "glad.h"

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_INCLUDE_STANDARD_BOOL
#define NK_INCLUDE_COMMAND_USERDATA

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include "nuklear.h"
#pragma GCC diagnostic pop
#include "nuklear_sdl_gl3.h"

#define FLO_PIXMAP_IMPLEMENTATION
#include "flo_pixmap.h"
#define FLO_MATRIX_IMPLEMENTATION
#include "flo_matrix.h"
#define FLO_TIME_IMPLEMENTATION
#include "flo_time.h"

#include "create_image.h"
#define FLO_FILE_IMPLEMENTATION
#include "flo_file.h"
// #define STB_IMAGE_WRITE_IMPLEMENTATION
// #include "stb_image_write.h"


#define WINDOW_WIDTH 2000

#include "sub_texture.h"

#define MAX_VERTEX_MEMORY 512 * 1024
#define MAX_ELEMENT_MEMORY 128 * 1024


typedef struct {
    sub_texture_t *left;
    sub_texture_t *right;
    sub_texture_t *correlation;
} textures_t;

static inline unsigned int min(unsigned int a, unsigned int b)
{
    return a < b ? a : b;
}

textures_t result2textures( const stereo_result_t s) {
    unsigned int width = s.left->width;
    sub_texture_t *left = sub_texture_alloc(width);
    sub_texture_set(left, s.left);
    sub_texture_t *right = NULL;
    if (s.right) {
        right = sub_texture_alloc(width);
        sub_texture_set(right, s.right);
    }
    sub_texture_t *correlation = NULL;
    if (s.correlation) {
        correlation = sub_texture_alloc(width);
        sub_texture_set(left, s.correlation);
    }
    return (textures_t){.left = left, .right = right, .correlation = correlation};
}


void textures_show(struct nk_context *ctx, textures_t t, float mag, bool black_white, float rot)
{
    static float previous_mag = 1.0f;

    assert(t.left);
    assert((t.right == NULL && t.correlation == NULL) || ( (t.right != NULL && t.correlation != NULL)));
    if (t.right != NULL) {
        assert(t.left->height == t.right->height);
        assert(t.left->width == t.right->width);
        assert(t.left->num == t.right->num);
        assert(t.left->height == t.correlation->height);
        assert(t.left->width == t.correlation->width);
        assert(t.left->num == t.correlation->num);
    }

    unsigned int height = t.left->height;
    unsigned int width = t.left->width;
    nk_handle userdata = {0};
    unsigned int num_height = t.right == NULL ? 1 : 3;

    if (nk_begin(ctx, "Image Display", nk_rect(0, 0, WINDOW_WIDTH, height * num_height + 100),
        NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE | NK_WINDOW_MINIMIZABLE | NK_WINDOW_TITLE))
    {
        nk_set_user_data(ctx, userdata);
        ctx->style.window.spacing = nk_vec2(0.f, 0.f);
        // Use nk_image to display the texture
        // layout repeats itself, so only have to give it once
        struct nk_panel *layout = ctx->current->layout;
        // hack the horizontal slider to stay at current position when changing size
        if (layout->offset_x != NULL && mag != previous_mag)
        {
            *layout->offset_x *= (mag / previous_mag);
            previous_mag = mag;
        }

        sub_texture_display(ctx, t.left, black_white, mag, rot);
        sub_texture_display(ctx, t.right, black_white, mag, rot);
        sub_texture_display(ctx, t.correlation, true, mag, 0.);

        // new height: fontsize 20
        size_t num_label = 100;
        nk_layout_row_static(ctx, 20.0f, WINDOW_WIDTH, num_label );
        nk_set_user_data(ctx, userdata);
        for (size_t i = 0; i < num_label; i++)
        {
            char sbuffer[20] = {0};
            const int n = snprintf(sbuffer, sizeof(sbuffer), "%f s", (float)(i * width) / 250000.0 * 512 * (1.0 - 0.1));
            nk_set_user_data(ctx, userdata);
            nk_text(ctx, sbuffer, n, NK_TEXT_LEFT);
        }
        struct nk_window *win = ctx->current;
        struct nk_command_buffer *buf = &win->buffer;

        nk_fill_rect ( buf, nk_rect ( 100, 100, 200, 200 ), 2, nk_rgb_f ( 0.5, 0., 0.5 ) );
    }
    nk_end(ctx);
}

void textures_free( textures_t t) {
    free( t.left);
    if (t.right) {
        free(t.right);
    }
    if (t.correlation) {
        free(t.correlation);
    }
}

/* ===============================================================
 *
 *                          DEMO
 *
 * ===============================================================*/
#ifdef _WIN32
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
#else
int main(int argc, char *argv[])
#endif
{
    /* Platform */
    SDL_Window *win;
    SDL_GLContext glContext;

    int win_width, win_height;
    int running = 1;
    #ifdef _WIN32
    int argc = 1;
    char *argv[] = {"capture.raw"};
    #endif
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

    unsigned int fft_size = 512;

    /* SDL setup */
    SDL_SetHint(SDL_HINT_VIDEO_HIGHDPI_DISABLED, "0");
    // Prevent DBus memory leak
    #ifdef SDL_HINT_SHUTDOWN_DBUS_ON_QUIT
    SDL_SetHint(SDL_HINT_SHUTDOWN_DBUS_ON_QUIT, "1");
    #endif
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_EVENTS);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    win = SDL_CreateWindow("Bat Desktop",
                           SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                           WINDOW_WIDTH, (int)fft_size / 2, SDL_WINDOW_OPENGL | SDL_WINDOW_MAXIMIZED | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    glContext = SDL_GL_CreateContext(win);
    int version = gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress);
    printf("GL %d.%d\n", GLAD_VERSION_MAJOR(version), GLAD_VERSION_MINOR(version));

    SDL_GetWindowSize(win, &win_width, &win_height);
    int rw = 0, rh = 0;
    SDL_Renderer *render = SDL_GetRenderer(win);
    SDL_GetRendererOutputSize(render, &rw, &rh);
    // const char *ptr = SDL_GetError();

    float widthScale = (float)rw / (float)win_width;
    float heightScale = (float)rh / (float)win_height;
    if (widthScale != heightScale)
    {
        fprintf(stderr, "WARNING: width scale != height scale\n");
    }

    SDL_RenderSetScale(render, widthScale, heightScale);

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
    stereo_result_t result = create_image_meow(size, raw_file, 2, 0, fft_size, 0.1, false);
    textures_t tex = result2textures( result);

    float mag = 1.0f;
    float rot = 0.0f;
    bool next_wait = false;
    nk_bool black_white = false;
    // for adapting the scrollbar when changing mag
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
        else
        {
            while (SDL_PollEvent(&evt))
            {
                if (evt.type == SDL_QUIT)
                {
                    goto cleanup;
                }
                nk_sdl_handle_event(&evt);
            }
            next_wait = true;
        }


        nk_sdl_handle_grab(); /* optional grabbing behavior */
        nk_input_end(ctx);
        unsigned int height = result.left->height;
        // unsigned int width = result.left->width;

        /* GUI */
        // Start a new UI frame
        textures_show(ctx, tex, mag, black_white, rot);

        unsigned int num = tex.right == NULL ? 1 : 3;

        if (nk_begin(ctx, "Settings", nk_rect(0, fft_size / 2 * num + 100, 400, 400),
            NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE | NK_WINDOW_MINIMIZABLE | NK_WINDOW_TITLE))
        {
            nk_layout_row_begin(ctx, NK_STATIC, 50, 2);
            {
                nk_layout_row_push(ctx, 50);
                nk_label(ctx, "Magnification:", NK_TEXT_LEFT);
                nk_layout_row_push(ctx, 200);
                nk_slider_float(ctx, 1.0f, &mag, 4.0f, 0.01f);
            }
            nk_layout_row_begin(ctx, NK_STATIC, 50, 2);
            {
                if (black_white)
                {
                    nk_widget_disable_begin(ctx);
                }
                nk_layout_row_push(ctx, 50);
                nk_label(ctx, "Hue Rotation:", NK_TEXT_LEFT);
                nk_layout_row_push(ctx, 200);
                nk_knob_float(ctx, 0.0f, &rot, 1.0f, 0.01f, NK_UP, 1.f);

                if (black_white)
                {
                    nk_widget_disable_end(ctx);
                }
            }
            nk_layout_row_begin(ctx, NK_STATIC, 50, 2);
            {
                nk_layout_row_push(ctx, 50);
                nk_checkbox_label_align(ctx, "B/W images", &black_white, NK_WIDGET_RIGHT, NK_TEXT_LEFT);
            }
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
    stereo_result_free(result);
    textures_free(tex);


    nk_sdl_shutdown();
    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
