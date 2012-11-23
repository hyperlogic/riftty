#include <stdio.h>
#include <string>
#include "SDL/SDL.h"
#include "keyboard.h"
#include "opengl.h"
#include "appconfig.h"

#include "glyphblaster/src/gb_context.h"
#include "glyphblaster/src/gb_font.h"
#include "glyphblaster/src/gb_text.h"

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
}

GB_Context* s_gb = 0;
GB_Font* s_font = 0;
GB_Text* s_text = 0;
std::string s_string;

// time tracking
unsigned int s_ticks = 0;

static uint32_t MakeColor(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha)
{
    return alpha << 24 | blue << 16 | green << 8 | red;
}

void Process()
{
    child_poll();
    /*
    const size_t BUFFER_SIZE = 1024;
    char buffer[BUFFER_SIZE];
    int size;
    while((size = Pty_Read(s_pty, buffer, BUFFER_SIZE)) > 0) {
        s_string += buffer;

        if (s_text) {
            GB_TextRelease(s_gb, s_text);
            s_text = NULL;
        }

        // create a new text
        uint32_t origin[2] = {0, 0};
        uint32_t size[2] = {s_config->width, s_config->height};
        uint32_t textColor = MakeColor(255, 255, 255, 255);
        GB_ERROR err = GB_TextMake(s_gb, (uint8_t*)s_string.c_str(), s_font, textColor,
                                   origin, size, GB_HORIZONTAL_ALIGN_LEFT,
                                   GB_VERTICAL_ALIGN_CENTER, &s_text);
        if (err != GB_ERROR_NONE) {
            fprintf(stderr, "GB_MakeText Error %s\n", GB_ErrorToString(err));
            exit(1);
        }
    }
    */
}

void Render()
{
    Vector4f clearColor(0, 0, 0, 1);
    glClearColor(clearColor.x, clearColor.y, clearColor.z, clearColor.w);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (s_text) {
        GB_ERROR err = GB_TextDraw(s_gb, s_text);
        if (err != GB_ERROR_NONE) {
            fprintf(stderr, "GB_DrawText Error %s\n", GB_ErrorToString(err));
            exit(1);
        }
    }
    SDL_GL_SwapBuffers();
}

void OnKeyPress(int ascii, bool down)
{
    if (down) {
        /*
        const char c = (char)ascii;
        if (!Pty_Send(s_pty, &c, 1))
        {
            fprintf(stderr, "Pty_Send() failed");
            exit(1);
        }
        */
    }
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
        int width = videoInfo->current_w;
        int height = videoInfo->current_h;
        int bpp = videoInfo->vfmt->BitsPerPixel;
        screen = SDL_SetVideoMode(width, height, bpp,
                                  SDL_HWSURFACE | SDL_OPENGL | SDL_FULLSCREEN);
    }
    else
    {
        screen = SDL_SetVideoMode(config.width, config.height, 32, SDL_HWSURFACE | SDL_OPENGL);
    }

    SDL_WM_SetCaption(config.title.c_str(), config.title.c_str());

    if (!screen)
        fprintf(stderr, "Couldn't create SDL screen!\n");

    // clear to black
    Vector4f clearColor(0, 0, 0, 1);
    glClearColor(clearColor.x, clearColor.y, clearColor.z, clearColor.w);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    SDL_GL_SwapBuffers();

    // create the context
    GB_ERROR err;
    err = GB_ContextMake(128, 3, GB_TEXTURE_FORMAT_ALPHA, &s_gb);
    if (err != GB_ERROR_NONE) {
        fprintf(stderr, "GB_Init Error %d\n", err);
        exit(1);
    }

    // create a font
    err = GB_FontMake(s_gb, "font/SourceCodePro-Bold.ttf", 14, GB_RENDER_NORMAL,
                      GB_HINT_DEFAULT, &s_font);
    if (err != GB_ERROR_NONE) {
        fprintf(stderr, "GB_MakeFont Error %s\n", GB_ErrorToString(err));
        exit(1);
    }

    GB_ContextSetTextRenderFunc(s_gb, TextRenderFunc);

    SetRepeatKeyCallback(OnKeyPress);

    init_config();
    cs_init();  // TODO: code pages do not want
    // TODO: load config from /etc/riffty or ~/.rifttyrc
    finish_config();
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

            KeyboardProcess(dt);
            Process();
            Render();
        }
    }

    child_kill(true);

    if (s_text) {
        err = GB_TextRelease(s_gb, s_text);
        if (err != GB_ERROR_NONE) {
            fprintf(stderr, "GB_ReleaseText Error %s\n", GB_ErrorToString(err));
            exit(1);
        }
    }
    err = GB_FontRelease(s_gb, s_font);
    if (err != GB_ERROR_NONE) {
        fprintf(stderr, "GB_ReleaseFont Error %s\n", GB_ErrorToString(err));
        exit(1);
    }
    err = GB_ContextRelease(s_gb);
    if (err != GB_ERROR_NONE) {
        fprintf(stderr, "GB_Shutdown Error %s\n", GB_ErrorToString(err));
        exit(1);
    }

    return 0;
}
