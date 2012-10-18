#include <stdio.h>
#include <string>
#include "SDL/SDL.h"
#include "pty.h"
#include "keyboard.h"

#ifdef __APPLE__
#include "TargetConditionals.h"
#endif

#if defined DARWIN

#include <OpenGL/gl.h>
#include <OpenGL/glu.h>

#elif TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR

#include <OpenGLES/ES1/gl.h>
#include <OpenGLES/ES1/glext.h>

#else

#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/glu.h>

#endif

#include "glyphblaster/src/gb_context.h"
#include "glyphblaster/src/gb_font.h"
#include "glyphblaster/src/gb_text.h"

#include "abaci.h"

#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>

struct Config
{
    Config(bool fullscreenIn, bool msaaIn, int widthIn, int heightIn) :
        fullscreen(fullscreenIn), msaa(msaaIn), msaaSamples(4),
        width(widthIn), height(heightIn) {};

    bool fullscreen;
    bool msaa;
    int msaaSamples;
    int width;
    int height;
    std::string title;
};

Config* s_config = 0;
Pty* s_pty = 0;
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

void DrawTexturedQuad(uint32_t gl_tex, Vector2f const& origin, Vector2f const& size,
                      Vector2f const& uv_origin, Vector2f const& uv_size,
                      uint32_t color)
{
    // assume texture is enabled.
    glBindTexture(GL_TEXTURE_2D, gl_tex);

    float verts[8];
    verts[0] = origin.x;
    verts[1] = origin.y;
    verts[2] = origin.x + size.x;
    verts[3] = origin.y;
    verts[4] = origin.x;
    verts[5] = origin.y + size.y;
    verts[6] = origin.x + size.x;
    verts[7] = origin.y + size.y;

    glVertexPointer(2, GL_FLOAT, 0, verts);
    glEnableClientState(GL_VERTEX_ARRAY);

    float uvs[8];
    uvs[0] = uv_origin.x;
    uvs[1] = uv_origin.y;
    uvs[2] = uv_origin.x + uv_size.x;
    uvs[3] = uv_origin.y;
    uvs[4] = uv_origin.x;
    uvs[5] = uv_origin.y + uv_size.y;
    uvs[6] = uv_origin.x + uv_size.x;
    uvs[7] = uv_origin.y + uv_size.y;

    uint32_t colors[8];
    for (int i = 0; i < 8; i++)
        colors[i] = color;

    glEnableClientState(GL_COLOR_ARRAY);
    glColorPointer(4, GL_UNSIGNED_BYTE, 0, colors);

    glClientActiveTexture(GL_TEXTURE0);
    glTexCoordPointer(2, GL_FLOAT, 0, uvs);

    glActiveTexture(GL_TEXTURE0);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);

    static uint16_t indices[] = {0, 2, 1, 2, 3, 1};
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indices);
}

void TextRenderFunc(GB_GlyphQuad* quads, uint32_t num_quads)
{
    // note this flips y-axis so y is down.
    Matrixf proj = Matrixf::Ortho(0, s_config->width, s_config->height, 0, -10, 10);
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(reinterpret_cast<float*>(&proj));
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_TEXTURE_2D);

    static int count = 1;  // set to 0 to enable dumping
    for (uint32_t i = 0; i < num_quads; ++i) {

        if (count == 0) {
            printf("quad[%d]\n", i);
            printf("    origin = [%d, %d]\n", quads[i].origin[0], quads[i].origin[1]);
            printf("    size = [%d, %d]\n", quads[i].size[0], quads[i].size[1]);
            printf("    uv_origin = [%.3f, %.3f]\n", quads[i].uv_origin[0], quads[i].uv_origin[1]);
            printf("    uv_size = [%.3f, %.3f]\n", quads[i].uv_size[0], quads[i].uv_size[1]);
            printf("    gl_tex_obj = %u\n", quads[i].gl_tex_obj);
            printf("    color = 0x%x\n", quads[i].color);
        }

        DrawTexturedQuad(quads[i].gl_tex_obj,
                         Vector2f(quads[i].origin[0], quads[i].origin[1]),
                         Vector2f(quads[i].size[0], quads[i].size[1]),
                         Vector2f(quads[i].uv_origin[0], quads[i].uv_origin[1]),
                         Vector2f(quads[i].uv_size[0], quads[i].uv_size[1]),
                         quads[i].color);
    }

    count++;
}

void Process()
{
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
        const char c = (char)ascii;
        if (!Pty_Send(s_pty, &c, 1))
        {
            fprintf(stderr, "Pty_Send() failed");
            exit(1);
        }
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
    s_config = new Config(false, false, 1024, 768);
    Config& config = *s_config;
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

    if (!Pty_Make(&s_pty)) {
        fprintf(stderr, "Pty_Make failed");
        exit(1);
    }

    SetRepeatKeyCallback(OnKeyPress);

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
