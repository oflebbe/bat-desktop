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
#include <threads.h>
#include <GL/glew.h>

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

#include "create_image.h"
#include "flo_file.h"
#include "flo_queue.h"
#include "stb_image_write.h"

#define WINDOW_WIDTH 1000
#define WINDOW_HEIGHT 256

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
    flo_pixmap_t *pixmap_l;
    flo_pixmap_t *pixmap_r;
} result_t;

// Function to generate a simple image (random colors)
result_t *generate_image(long size, const uint16_t raw_file[size], long start, int width)
{

#ifdef STEREO
    uint16_t *audio_l = malloc(size);
    uint16_t *audio_r = malloc(size);
    int stereo_size = size / 2;
    for (int j = 0; j < stereo_size / 2 - 1; j++)
    {
        audio_l[j] = raw_file[j * 2];
        audio_r[j] = raw_file[j * 2 + 1];
    }

    int fft_size = 512;
    float overlap_percent = 0.1;
    result_t *r = calloc(1, sizeof(result_t));

    r->pixmap_l = create_image_meow(stereo_size, audio_l, start, width, fft_size, overlap_percent);
    free(audio_l);

    r->pixmap_r = create_image_meow(stereo_size, audio_r, start, width, fft_size, overlap_percent);
    free(audio_r);
#endif
    return r;
}

typedef struct bg_parameters
{
    long size;
    uint16_t *raw_file;
    long start;
    int width;
    long new_start;
} bg_t;

typedef struct queues
{
    flo_queue_t *input;
    flo_queue_t *output;
} queues_t;

int bg_func(void *queues)
{
    queues_t *q = (queues_t *)queues;
    while (1)
    {
        bg_t params;
        bg_t *p = flo_queue_pop_block(q->input, &params);
        if (!p)
        {
            flo_queue_free(q->input);
            flo_queue_close(q->output);
            return 0;
        }
        result_t *r = generate_image(p->size, p->raw_file, p->start, p->width); // Generate an image
        flo_queue_push_block(q->output, r);
        free(r);
    }
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
    struct nk_context *ctx;
    struct nk_colorf bg;

    NK_UNUSED(argc);
    NK_UNUSED(argv);

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
    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
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

    GLuint textures[2];
    // stbi_write_png("data.png", pixmap_l->width, pixmap_l->height, 3, rgbbuffer, pixmap_l->width);
    glGenTextures(2, textures);

    glBindTexture(GL_TEXTURE_2D, textures[0]);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB8, WINDOW_WIDTH, WINDOW_HEIGHT);
    glBindTexture(GL_TEXTURE_2D, textures[1]);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB8, WINDOW_WIDTH, WINDOW_HEIGHT);
    long start = 0;
    bg_t params = {
        .size = size,
        .raw_file = raw_file,
        .start = start,
        .width = WINDOW_WIDTH};

    thrd_t bg_thread;
    queues_t queues = {0};
    queues.input = flo_queue_create(1, sizeof(bg_t));
    queues.output = flo_queue_create(1, sizeof(result_t));

    thrd_create(&bg_thread, bg_func, &queues);
    flo_queue_push_block(queues.input, &params);
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
                flo_queue_close(queues.input);
                thrd_join(bg_thread, NULL);
                result_t res;
                while (NULL != flo_queue_pop_block(queues.output, &res))
                {
                };
                flo_queue_free(queues.output);
                goto cleanup;
            }
            nk_sdl_handle_event(&evt);
        }
        while (SDL_PollEvent(&evt))
        {
            if (evt.type == SDL_QUIT)
            {
                flo_queue_close(queues.input);
                thrd_join(bg_thread, NULL);
                result_t res;
                while (NULL != flo_queue_pop_block(queues.output, &res))
                {
                };
                flo_queue_free(queues.output);
                goto cleanup;
            }
            nk_sdl_handle_event(&evt);
        }
        nk_sdl_handle_grab(); /* optional grabbing behavior */
        nk_input_end(ctx);
        if (nk_input_is_key_pressed(&ctx->input, NK_KEY_LEFT))
        {
            start -= size / 100;
            if (start < 0)
            {
                start = 0;
            }
            params.start = start;
            flo_queue_push_block(queues.input, &params);
            next_wait = false;
        }
        else if (nk_input_is_key_pressed(&ctx->input, NK_KEY_RIGHT))
        {
            start += size / 100;
            if (start > size)
            {
                start = size;
            }
            params.start = start;
            flo_queue_push_block(queues.input, &params);
            next_wait = false;
        }

        if (!flo_queue_empty(queues.output))
        {
            result_t res;
            result_t *result = flo_queue_pop_block(queues.output, &res);
            next_wait = true;

            glBindTexture(GL_TEXTURE_2D, textures[0]);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, result->pixmap_l->width, result->pixmap_l->height, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, result->pixmap_l->buf);

            free(result->pixmap_l);

            glBindTexture(GL_TEXTURE_2D, textures[1]);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, result->pixmap_r->width, result->pixmap_r->height, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, result->pixmap_r->buf);
            free(result->pixmap_r);
        }
        /* GUI */
        // Start a new UI frame
        if (nk_begin(ctx, "Image Display", nk_rect(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT * 2 + 40),
                     NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE | NK_WINDOW_MINIMIZABLE | NK_WINDOW_TITLE))
        {
            nk_layout_row_dynamic(ctx, 256, 1);
            // Use nk_image to display the texture
            nk_image(ctx, nk_image_id(textures[0])); // This will render the full texture
            nk_image(ctx, nk_image_id(textures[1])); // This will render the full texture
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
    nk_sdl_shutdown();
    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
