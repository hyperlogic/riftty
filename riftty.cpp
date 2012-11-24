#include <stdio.h>
#include <string>
#include "SDL/SDL.h"
#include "keyboard.h"
#include "opengl.h"
#include "appconfig.h"
#include "render.h"
#include "gb_context.h"

#include "abaci.h"

#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "render.h"

extern "C" {
#include "config.h"
#include "charset.h"
#include "term.h"
#include "child.h"
#include "win.h"
}

// time tracking
unsigned int s_ticks = 0;

static uint32_t MakeColor(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha)
{
    return alpha << 24 | blue << 16 | green << 8 | red;
}

void Process(float dt)
{
    child_poll();
}

void Render()
{
    Vector4f clearColor(0, 0, 0, 1);
    glClearColor(clearColor.x, clearColor.y, clearColor.z, clearColor.w);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    for (size_t i = 0; i < s_context.textCount; i++)
    {
        GB_ERROR err = GB_TextDraw(s_context.gb, s_context.text[i]);

        if (err != GB_ERROR_NONE) {
            fprintf(stderr, "GB_DrawText Error %s\n", GB_ErrorToString(err));
            exit(1);
        }
    }
    SDL_GL_SwapBuffers();
}

int main(int argc, char* argv[])
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) < 0)
        fprintf(stderr, "Couldn't init SDL!\n");

    atexit(SDL_Quit);

    // Get the current desktop width & height
    const SDL_VideoInfo* videoInfo = SDL_GetVideoInfo();

    // TODO: get this from config file.
    s_config = new AppConfig(false, false, 1024, 768);
    AppConfig& config = *s_config;
    config.title = "riftty";

    // msaa
    if (config.msaa)
    {
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, config.msaaSamples);
    }

    SDL_Surface* screen;
    if (config.fullscreen)
    {
        int width = config.width;//videoInfo->current_w;
        int height = config.height;//videoInfo->current_h;
        int bpp = 32;//videoInfo->vfmt->BitsPerPixel;

        fprintf(stderr, "fullscreen : width = %d, height = %d, bpp = %d\n", width, height, bpp);
        screen = SDL_SetVideoMode(width, height, bpp,
                                  SDL_HWSURFACE | SDL_OPENGL | SDL_FULLSCREEN);
    }
    else
    {
        screen = SDL_SetVideoMode(config.width, config.height, 32, SDL_HWSURFACE | SDL_OPENGL);
    }

    SDL_WM_SetCaption(config.title.c_str(), config.title.c_str());

    SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);

    if (!screen)
    {
        fprintf(stderr, "Couldn't create SDL screen!\n");
        exit(2);
    }

    // clear to black
    Vector4f clearColor(0, 0, 0, 1);
    glClearColor(clearColor.x, clearColor.y, clearColor.z, clearColor.w);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    SDL_GL_SwapBuffers();

    win_init();

    GB_ContextSetTextRenderFunc(s_context.gb, TextRenderFunc);

    init_config();
    cs_init();  // TODO: code pages do not want

    // TODO: determine this based on window-size & font-size or vice versa.
    cfg.rows = 42;
    cfg.cols = 124;

    // TODO: load config from /etc/riffty or ~/.rifttyrc
    finish_config();
    win_reconfig();

    // TODO: get SHELL from env

    cs_reconfig(); // TODO: do not want

    term_reset();
    term_resize(cfg.rows, cfg.cols);

    int font_width = 10;
    int font_height = 10;
    int term_width = font_width * cfg.cols;
    int term_height = font_height * cfg.rows;

    const char* login_argv[] = {"login", "-pfl", "ajt", NULL};
    child_create(login_argv, &(struct winsize){cfg.rows, cfg.cols, term_width, term_height});

    int done = 0;
    while (!done)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
            case SDL_QUIT:
                done = 1;
                break;

            case SDL_MOUSEMOTION:
                if (event.motion.state & SDL_BUTTON(1)) {
                    // move touch
                }
                break;

            case SDL_MOUSEBUTTONDOWN:
                if (event.button.button == SDL_BUTTON_LEFT) {
                    // start touch
                }
                break;

            case SDL_MOUSEBUTTONUP:
                if (event.button.button == SDL_BUTTON_LEFT) {
                    // end touch
                }
                break;

            case SDL_JOYAXISMOTION:
                // stick move
                break;

            case SDL_JOYBUTTONDOWN:
            case SDL_JOYBUTTONUP:
                // joy pad press
                break;

			case SDL_KEYDOWN:
			case SDL_KEYUP:
				ProcessKeyEvent(&event.key);
				break;
            }
        }

        if (!done)
        {
            unsigned int now = SDL_GetTicks();  // milliseconds
            float dt = (now - s_ticks) / 1000.0f;	// convert to seconds.
            s_ticks = now;

            Process(dt);
            Render();
        }
    }

    child_kill(true);
    win_shutdown();

    return 0;
}
