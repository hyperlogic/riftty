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
#include "shader.h"

extern "C" {
#include "config.h"
#include "charset.h"
#include "term.h"
#include "child.h"
#include "win.h"
}

Vector4f s_clearColor(0, 0, 0.2, 1);

// time tracking
unsigned int s_ticks = 0;
static Matrixf s_RotY90 = Matrixf::AxisAngle(Vector3f(0,1,0), PI/2.0f);

void Process(float dt)
{
    child_poll();
}

const float kFeetToCm = 30.48;
const float kInchesToCm = 2.54;

void Render(float dt)
{
    glClearColor(s_clearColor.x, s_clearColor.y, s_clearColor.z, s_clearColor.w);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    static float t = 0.0;
    t += dt;

    Matrixf cameraMatrix = Matrixf::LookAt(Vector3f(50 * sin(t), 50 * cos(t), 500), Vector3f(0, 0, 0), Vector3f(0, 1, 0));
    Matrixf viewMatrix = s_RotY90 * cameraMatrix.OrthoInverse();

    float aspect = (float)s_config->width / s_config->height;
    Matrixf projMatrix = Matrixf::Frustum(DegToRad(50.0f), aspect, 10.0, 10000);
    Matrixf modelMatrix = Matrixf::ScaleQuatTrans(Vector3f(0.25, -0.25, 0.25), Quatf::Identity(), Vector3f(-s_config->width/2.0f, s_config->height/2.0f, 0) * 0.25f);

    RenderBegin();

    RenderFloor(projMatrix, viewMatrix, -100.0f);

    RenderTextBegin(projMatrix, viewMatrix, modelMatrix);
    for (size_t i = 0; i < s_context.textCount; i++) {
        const GB_Text* text = s_context.text[i];
        RenderText(text->glyph_quads, text->num_glyph_quads);
    }
    RenderTextEnd();

    RenderEnd();

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
    s_config = new AppConfig(false, false, 1280, 800);
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

    // clear
    glClearColor(s_clearColor.x, s_clearColor.y, s_clearColor.z, s_clearColor.w);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    SDL_GL_SwapBuffers();

    RenderInit();

    win_init();

    init_config();
    cs_init();  // TODO: code pages do not want

    // TODO: determine this based on window-size & font-size or vice versa.
    cfg.rows = 47;
    cfg.cols = 127;

    // TODO: load config from /etc/riffty or ~/.rifttyrc
    finish_config();
    win_reconfig();

    // TODO: get SHELL from env

    cs_reconfig(); // TODO: do not want

    term_reset();
    term_resize(cfg.rows, cfg.cols);

    // TODO:
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

            //printf("fps = %.0f\n", 1.0f/dt);

            Process(dt);
            Render(dt);
        }
    }

    child_kill(true);
    win_shutdown();

    return 0;
}
