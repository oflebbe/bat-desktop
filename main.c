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
#include <pthread.h>
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
#define FLO_FILE_IMPLEMENTATION
#include "flo_file.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"


#define WINDOW_WIDTH 2000
#define WINDOW_HEIGHT 256

#define TEXTURE_WIDTH 16384

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

typedef struct image_params {
    long size;
    const uint16_t *raw_file;
    int fft_size;
    int offset;
    int scale;
    float overlap_percent;
} image_params_t;

void *thread_routine( void *arg ) {
    image_params_t *params = (image_params_t *) arg;
    return (void *) create_image_meow( params->size, params->raw_file, params->scale, params->offset, params->fft_size, params->overlap_percent);
}

// Function to generate a simple image (random colors)
result_t generate_image(long size, const uint16_t raw_file[size], int fft_size, float overlap_percent)
{
    result_t r;

    image_params_t parms1 = {.size = size, .raw_file = raw_file, .fft_size = fft_size, .scale = 2, .offset = 0, .overlap_percent = overlap_percent};
    pthread_t thrd1;
    pthread_create( &thrd1, NULL, thread_routine, &parms1);

    image_params_t parms2 = {.size = size, .raw_file = raw_file, .fft_size = fft_size, .scale = 2, .offset = 1, .overlap_percent = overlap_percent};
    pthread_t thrd2;
    pthread_create( &thrd2, NULL, thread_routine, &parms2);

    pthread_join(thrd1, (void **) &r.pixmap_l);
    pthread_join(thrd2, (void **) &r.pixmap_r);
    return r;
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

    result_t result = generate_image(size, raw_file, 512, 0.1);

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
    glViewport(0, 0, result.pixmap_l->width, result.pixmap_l->height*2);
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

    int num_tex_line = result.pixmap_l->width / TEXTURE_WIDTH ;
    GLuint *textures = malloc( sizeof(GLuint) * 2 * num_tex_line );
    
   // stbi_write_png("data.png", result.pixmap_l->width, result.pixmap_l->height, 2, result.pixmap_l->buf, 0);
    glGenTextures( num_tex_line * 2, textures);

    int height = result.pixmap_l->height;
    for (int i= 0; i < num_tex_line; i++) {
        glBindTexture(GL_TEXTURE_2D, textures[i]);
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, result.pixmap_l->width);
         
        
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, TEXTURE_WIDTH, result.pixmap_l->height, 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, 
                &result.pixmap_l->buf[(i * TEXTURE_WIDTH)]);

            
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    }

    free(result.pixmap_l);
    result.pixmap_l = NULL;

    for (int i= 0; i < num_tex_line; i++) {
        glBindTexture(GL_TEXTURE_2D, textures[i+num_tex_line]);
       // glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, result.pixmap_r->width);
      
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, TEXTURE_WIDTH, result.pixmap_r->height, 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, 
            &result.pixmap_r->buf[(i * TEXTURE_WIDTH)]);

        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    }
    free(result.pixmap_r);
    result.pixmap_r = NULL;

    bool next_wait = false;
    
    while (running)
    {
        /* Input */
        SDL_Event evt;
        nk_input_begin(ctx);
        if (false)
        {
            SDL_WaitEvent(&evt);
            if (evt.type == SDL_QUIT)
            {
                goto cleanup;
            }
            nk_sdl_handle_event(&evt);
        }
        while (SDL_PollEvent(&evt))
        {
            if (evt.type == SDL_QUIT)
            {
                goto cleanup;
            }
            nk_sdl_handle_event(&evt);
        }
        nk_sdl_handle_grab(); /* optional grabbing behavior */
        nk_input_end(ctx);

        /* GUI */
        // Start a new UI frame
        if (nk_begin(ctx, "Image Display", nk_rect(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT * 2 + 40),
                     NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE | NK_WINDOW_MINIMIZABLE | NK_WINDOW_TITLE))
        {
            // Use nk_image to display the texture
            nk_layout_row_begin(ctx, NK_STATIC, height, num_tex_line);
            for (int i = 0; i < num_tex_line; i++) {
                nk_layout_row_push(ctx, TEXTURE_WIDTH);
                nk_image(ctx, nk_image_id(textures[i]));
            }
            nk_layout_row_end(ctx);

            nk_layout_row_begin(ctx, NK_STATIC, height, num_tex_line);
            for (int i = 0; i < num_tex_line; i++) {
                nk_layout_row_push(ctx, TEXTURE_WIDTH);
                nk_image(ctx, nk_image_id(textures[num_tex_line + i]));
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
    nk_sdl_shutdown();
    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
