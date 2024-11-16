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

#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_IMPLEMENTATION
#define NK_SDL_GL3_IMPLEMENTATION
#include "nuklear.h"
#include "nuklear_sdl_gl3.h"

#include "create_image.h"
#include "flo_file.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define WINDOW_WIDTH 2000
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

void RGB565_to_RGB888(uint16_t rgb565, uint8_t *r, uint8_t *g, uint8_t *b)
{
    // Extrahiere die Rot-, Grün- und Blau-Komponenten aus dem RGB565-Wert
    *r = ((rgb565 >> 11) & 0x1F) << 3; // Rot: 5 Bits, auf 8 Bits erweitern
    *g = ((rgb565 >> 5) & 0x3F) << 2;  // Grün: 6 Bits, auf 8 Bits erweitern
    *b = (rgb565 & 0x1F) << 3;         // Blau: 5 Bits, auf 8 Bits erweitern
}

// Function to generate a simple image (random colors)
GLuint generate_image(int width, int height)
{
    // During init, enable debug output
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(MessageCallback, 0);
    const char filename[] = "/run/media/olaf/BAT/bat034/capture00000.raw";
    FILE *fp = fopen(filename, "r");
    if (!fp)
    {
        return 0;
    }
    size_t size;
    uint16_t *raw_file = (uint16_t *)flo_mapfile(fp, &size);
    fclose(fp);
    if (!raw_file)
    {
        return 0;
    }

    size /= 2;
    if (size % 2 == 1)
    {
        raw_file = NULL;
        return 0;
    }

#ifdef STEREO
    uint16_t *audio_l = malloc(size);
    uint16_t *audio_r = malloc(size);
    int stereo_size = size / 2;
    for (int j = 0; j < stereo_size / 2 - 1; j++)
    {
        audio_l[j] = raw_file[j * 2];
        audio_r[j] = raw_file[j * 2 + 1];
    }

    long start = 0;
    long end = 0;
    int fft_size = 512;
    float overlap_percent = 0.1;

    if (start == 0 && end == 0)
    {
        end = stereo_size / 10;
    }
    flo_pixmap_t *pixmap_l = create_image_meow(stereo_size, audio_l, start, width, fft_size, overlap_percent);
    if (!pixmap_l)
    {
        return 0;
    }
    /*  QImage qimage_l((uchar *)pixmap_l->buf, pixmap_l->width, pixmap_l->height, QImage::Format_RGB16, free, pixmap_l);
      renderArea->addImage(qimage_l);

      flo_pixmap_t *pixmap_r = create_image_meow(stereo_size, audio_r, start, end, fft_size, overlap_percent);
      if (!pixmap_r)
      {
          return;
      } */
    // QImage qimage_r((uchar *)pixmap_r->buf, pixmap_r->width, pixmap_r->height, QImage::Format_RGB16, free, pixmap_r);
    // renderArea->addImage(qimage_r);
/*
  flo_pixmap_t *pixmap_c = create_image_cross(stereo_size, audio_l, audio_r, 512, overlap_percent);
  free(audio_l);
  free(audio_r);
  if (!pixmap_c)
  {
    continue;
  }
  QImage qimage_c((uchar *)pixmap_c->buf, pixmap_c->width, pixmap_c->height, QImage::Format_RGB16, free, pixmap_c);
  qwindow.addImage(qimage_c);
*/
#else
    flo_pixmap_t *pixmap = create_image_meow(stereo_size, raw_file, 512, overlap_percent);
    if (!pixmap)
    {
        continue;
    }
    QImage qimage((uchar *)pixmap->buf, pixmap->width, pixmap->height, QImage::Format_RGB16, free, pixmap);
    qwindow.addImage(qimage);
#endif
    free(audio_l);
    free(audio_r);

    GLuint tex_l;
   /* size_t sz = (long)pixmap_l->width * (long)pixmap_l->height;
    uint8_t *rgbbuffer = malloc(sz * 3);
    for (long i = 0; i < sz; i++)
    {
        RGB565_to_RGB888(pixmap_l->buf[i], &rgbbuffer[i * 3], &rgbbuffer[i * 3 + 1], &rgbbuffer[i * 3 + 2]);
    }
*/
    //stbi_write_png("data.png", pixmap_l->width, pixmap_l->height, 3, rgbbuffer, pixmap_l->width);
    glGenTextures(1, &tex_l);
    glBindTexture(GL_TEXTURE_2D, tex_l);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, pixmap_l->width, pixmap_l->height, 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, pixmap_l->buf);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    return tex_l;
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

    /* GUI */
    struct nk_context *ctx;
    struct nk_colorf bg;

    NK_UNUSED(argc);
    NK_UNUSED(argv);

    /* SDL setup */
    SDL_SetHint(SDL_HINT_VIDEO_HIGHDPI_DISABLED, "0");
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

    GLuint image_texture = generate_image(WINDOW_WIDTH, 256); // Generate an image
    while (running)
    {
        /* Input */
        SDL_Event evt;
        nk_input_begin(ctx);
        while (SDL_PollEvent(&evt))
        {
            if (evt.type == SDL_QUIT)
                goto cleanup;
            nk_sdl_handle_event(&evt);
        }
        nk_sdl_handle_grab(); /* optional grabbing behavior */
        nk_input_end(ctx);

        /* GUI */

        // Start a new UI frame
        if (nk_begin(ctx, "Image Display", nk_rect(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT+40),
                     NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE | NK_WINDOW_MINIMIZABLE | NK_WINDOW_TITLE))
        {
            nk_layout_row_dynamic(ctx, 256, 1);
            // Use nk_image to display the texture
            nk_image(ctx, nk_image_id(image_texture)); // This will render the full texture
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
