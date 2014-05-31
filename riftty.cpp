
#if (defined _WIN32) || (defined _WIN64)
#include "SDL.h"
#include "SDL_opengl.h"
#else
#include "SDL2/SDL.h"
#endif

#include <stdio.h>
#include <string>
#include "keyboard.h"
#include "opengl.h"
#include "appconfig.h"
#include "render.h"
#include "joystick.h"

#include "text.h"
#include "context.h"

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

#include "../Src/OVR_CAPI.h"
#include "../Src/OVR_CAPI_GL.h"

const float kFeetToMeters = 0.3048;

Vector4f s_clearColor(0, 0, 0.2, 1);

// time tracking
unsigned int s_ticks = 0;
static Matrixf s_RotY90 = Matrixf::AxisAngle(Vector3f(0,1,0), PI/2.0f);

ovrHmd s_hmd;
ovrHmdDesc s_hmdDesc;
ovrEyeRenderDesc s_eyeRenderDesc[2];
ovrSizei s_renderTargetSize;
ovrTexture s_eyeTexture[2];

uint32_t s_fbo;
uint32_t s_fboDepth;
uint32_t s_fboTex;

Vector3f s_cameraPos(0, 6 * kFeetToMeters, 0);

void DumpHMDInfo(const ovrHmdDesc& desc)
{
    printf("HMDInfo\n");
    printf("    ProductName = \"%s\"\n", desc.ProductName);
    printf("    Manufacturer = \"%s\"\n", desc.Manufacturer);
    printf("    HmdCaps = 0x%x\n", desc.HmdCaps);
    printf("        %s Present\n", desc.HmdCaps & ovrHmdCap_Present ? "IS" : "IS NOT");
    printf("        %s Available\n", desc.HmdCaps & ovrHmdCap_Available ? "IS" : "IS NOT");
    printf("    SensorCaps = 0x%x\n", desc.SensorCaps);
    printf("        %s Orientation\n", desc.SensorCaps & ovrSensorCap_Orientation ? "SUPPORTS" : "DOES NOT SUPPORT");
    printf("        %s YawCorrection\n", desc.SensorCaps & ovrSensorCap_YawCorrection ? "SUPPORTS" : "DOES NOT SUPPORT");
    printf("        %s Position\n", desc.SensorCaps & ovrSensorCap_Position ? "SUPPORTS" : "DOES NOT SUPPORT");
    printf("    DistortionCaps = 0x%x\n", desc.DistortionCaps);
    printf("        %s Chromatic\n", desc.DistortionCaps & ovrDistortionCap_Chromatic ? "SUPPORTS" : "DOES NOT SUPPORT");
    printf("        %s TimeWarp\n", desc.DistortionCaps & ovrDistortionCap_TimeWarp ? "SUPPORTS" : "DOES NOT SUPPORT");
    printf("        %s Vignette\n", desc.DistortionCaps & ovrDistortionCap_Vignette ? "SUPPORTS" : "DOES NOT SUPPORT");
    printf("    Resolution = %d x %d\n", desc.Resolution.w, desc.Resolution.h);
    printf("    WindowsPos = (%d, %d)\n", desc.WindowsPos.x, desc.WindowsPos.y);
    printf("    DefaultEyeFov = [(%f, %f, %f, %f), (%f, %f, %f, %f])]\n",
           desc.DefaultEyeFov[0].UpTan, desc.DefaultEyeFov[0].DownTan, desc.DefaultEyeFov[0].LeftTan, desc.DefaultEyeFov[0].RightTan,
           desc.DefaultEyeFov[1].UpTan, desc.DefaultEyeFov[1].DownTan, desc.DefaultEyeFov[1].LeftTan, desc.DefaultEyeFov[1].RightTan);
    printf("    MaxEyeFov = [(%f, %f, %f, %f), (%f, %f, %f, %f])]\n",
           desc.MaxEyeFov[0].UpTan, desc.MaxEyeFov[0].DownTan, desc.MaxEyeFov[0].LeftTan, desc.MaxEyeFov[0].RightTan,
           desc.MaxEyeFov[1].UpTan, desc.MaxEyeFov[1].DownTan, desc.MaxEyeFov[1].LeftTan, desc.MaxEyeFov[1].RightTan);
    printf("    EyeRenderOrder = %s, %s\n", desc.EyeRenderOrder[0] == ovrEye_Left ? "Left" : "Right", desc.EyeRenderOrder[1] == ovrEye_Left ? "Left" : "Right");
}

void CreateRenderTarget(int width, int height);

void RiftSetup()
{
    ovr_Initialize();

    s_hmd = ovrHmd_Create(0);
    if (!s_hmd)
    {
        s_hmd = ovrHmd_CreateDebug(ovrHmd_DK1);
    }

    ovrHmd_GetDesc(s_hmd, &s_hmdDesc);
    DumpHMDInfo(s_hmdDesc);

    uint32_t supportedSensorCaps = ovrSensorCap_Orientation;
    uint32_t requiredSensorCaps = ovrSensorCap_Orientation;
    ovrBool success = ovrHmd_StartSensor(s_hmd, supportedSensorCaps, requiredSensorCaps);
    if (!success) {
        fprintf(stderr, "ERROR: HMD does not have required capabilities!\n");
        exit(2);
    }

    // Figure out dimensions of render target
    ovrSizei recommenedTex0Size = ovrHmd_GetFovTextureSize(s_hmd, ovrEye_Left, s_hmdDesc.DefaultEyeFov[0], 1.0f);
    ovrSizei recommenedTex1Size = ovrHmd_GetFovTextureSize(s_hmd, ovrEye_Right, s_hmdDesc.DefaultEyeFov[1], 1.0f);
    s_renderTargetSize.w = recommenedTex0Size.w + recommenedTex1Size.w;
    s_renderTargetSize.h = std::max(recommenedTex0Size.h, recommenedTex1Size.h);

    CreateRenderTarget(s_renderTargetSize.w, s_renderTargetSize.h);

    s_eyeTexture[0].Header.API = ovrRenderAPI_OpenGL;
    s_eyeTexture[0].Header.TextureSize = s_renderTargetSize;
    s_eyeTexture[0].Header.RenderViewport.Pos = {0, 0};
    s_eyeTexture[0].Header.RenderViewport.Size = {s_renderTargetSize.w / 2, s_renderTargetSize.h};
    ((ovrGLTexture*)(&s_eyeTexture[0]))->OGL.TexId = s_fboTex;

    s_eyeTexture[1].Header.API = ovrRenderAPI_OpenGL;
    s_eyeTexture[1].Header.TextureSize = s_renderTargetSize;
    s_eyeTexture[1].Header.RenderViewport.Pos = {s_renderTargetSize.w / 2, 0};
    s_eyeTexture[1].Header.RenderViewport.Size = {s_renderTargetSize.w / 2, s_renderTargetSize.h};
    ((ovrGLTexture*)(&s_eyeTexture[1]))->OGL.TexId = s_fboTex;

    // Configure ovr SDK Rendering
    ovrGLConfig cfg;
    memset(&cfg, 0, sizeof(ovrGLConfig));
    cfg.OGL.Header.API = ovrRenderAPI_OpenGL;
    cfg.OGL.Header.RTSize = {s_config->width, s_config->height};
    cfg.OGL.Header.Multisample = 0;
    // TODO: on windows need to set HWND, on Linux need to set other parameters
    if (!ovrHmd_ConfigureRendering(s_hmd, &cfg.Config, s_hmdDesc.DistortionCaps, s_hmdDesc.DefaultEyeFov, s_eyeRenderDesc))
    {
        fprintf(stderr, "ERROR: HMD configure rendering failed!\n");
        exit(3);
    }
}

void RiftShutdown()
{
    ovrHmd_Destroy(s_hmd);
    ovr_Shutdown();
}

void Process(float dt)
{
    float kSpeed = 1.5f;
    Joystick* joy = JOYSTICK_GetJoystick();
    Vector3f vel(joy->axes[Joystick::LeftStickX],
                 joy->axes[Joystick::RightStickY],
                 -joy->axes[Joystick::LeftStickY]);
    s_cameraPos = s_cameraPos + vel * dt * kSpeed;

    child_poll();
}

void Render(float dt)
{
    static unsigned int frameIndex = 0;
    frameIndex++;
    ovrFrameTiming timing = ovrHmd_BeginFrame(s_hmd, 0);

    // ovrSensorState ss = ovrHmd_GetSensorState(s_hmd, timing.ScanoutMidpointSeconds);
    // TODO: Use this for head tracking...
    // TODO: Use player height from SDK

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // render into fbo
    glBindFramebuffer(GL_FRAMEBUFFER, s_fbo);

    // TODO: enable this when we have more complex rendering.
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    static float t = 0.0;
    t += dt;

    // clear render target
    glViewport(0, 0, s_renderTargetSize.w, s_renderTargetSize.h);
    glClearColor(s_clearColor.x, s_clearColor.y, s_clearColor.z, s_clearColor.w);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    for (int i = 0; i < 2; i++)
    {
        ovrEyeType eye = s_hmdDesc.EyeRenderOrder[i];
        ovrPosef pose = ovrHmd_BeginEyeRender(s_hmd, eye);

        glViewport(s_eyeTexture[eye].Header.RenderViewport.Pos.x,
                   s_eyeTexture[eye].Header.RenderViewport.Pos.y,
                   s_eyeTexture[eye].Header.RenderViewport.Size.w,
                   s_eyeTexture[eye].Header.RenderViewport.Size.h);

        Quatf q(pose.Orientation.x, pose.Orientation.y, pose.Orientation.z, pose.Orientation.w);
        Vector3f p(pose.Position.x, pose.Position.y, pose.Position.z);

        Matrixf cameraMatrix = Matrixf::QuatTrans(q, s_cameraPos);
        Matrixf viewCenter = cameraMatrix.OrthoInverse();

        // let ovr compute projection matrix, cause it's hard.
        ovrMatrix4f ovrProj = ovrMatrix4f_Projection(s_eyeRenderDesc[eye].Fov, 0.1f, 10000.0f, true);

        // convert to abaci matrix
        Matrixf projMatrix = Matrixf::Rows(Vector4f(ovrProj.M[0][0], ovrProj.M[0][1], ovrProj.M[0][2], ovrProj.M[0][3]),
                                           Vector4f(ovrProj.M[1][0], ovrProj.M[1][1], ovrProj.M[1][2], ovrProj.M[1][3]),
                                           Vector4f(ovrProj.M[2][0], ovrProj.M[2][1], ovrProj.M[2][2], ovrProj.M[2][3]),
                                           Vector4f(ovrProj.M[3][0], ovrProj.M[3][1], ovrProj.M[3][2], ovrProj.M[3][3]));

        // use EyeRenderDesc.ViewAdjust to do eye offset.
        Matrixf viewMatrix = viewCenter * Matrixf::Trans(Vector3f(s_eyeRenderDesc[eye].ViewAdjust.x,
                                                                  s_eyeRenderDesc[eye].ViewAdjust.y,
                                                                  s_eyeRenderDesc[eye].ViewAdjust.z));

        // compute model matrix for terminal
        const float kTermScale = 0.0043f;
        Matrixf modelMatrix = Matrixf::ScaleQuatTrans(Vector3f(kTermScale, -kTermScale, kTermScale),
                                                      Quatf::AxisAngle(Vector3f(0, 1, 0), 0),
                                                      Vector3f(-10 * kFeetToMeters, 15 * kFeetToMeters, -10 * kFeetToMeters));
        RenderBegin();

        RenderFloor(projMatrix, viewMatrix, 0.0f);

        RenderTextBegin(projMatrix, viewMatrix, modelMatrix);
        for (int j = 0; j < win_get_text_count(); j++)
        {
            gb::Text* text = (gb::Text*)win_get_text(j);
            if (text)
            {
                RenderText(text->GetQuadVec());
            }
        }
        RenderTextEnd();

        RenderEnd();
        ovrHmd_EndEyeRender(s_hmd, eye, pose, &s_eyeTexture[eye]);
    }

    ovrHmd_EndFrame(s_hmd);
}

void CreateRenderTarget(int width, int height)
{
    printf("Render Target size %d x %d\n", width, height);

    // create a fbo
    glGenFramebuffers(1, &s_fbo);

    GL_ERROR_CHECK("RenderTargetInit1");

    // create a depth buffer
    glGenRenderbuffers(1, &s_fboDepth);

    GL_ERROR_CHECK("RenderTargetInit2");

    // bind fbo
    glBindFramebuffer(GL_FRAMEBUFFER, s_fbo);

    GL_ERROR_CHECK("RenderTargetInit3");

    glGenTextures(1, &s_fboTex);
    glBindTexture(GL_TEXTURE_2D, s_fboTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);

    // attach texture to fbo
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D, s_fboTex, 0);

    // init depth buffer
    glBindRenderbuffer(GL_RENDERBUFFER, s_fboDepth);

    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, width, height);

    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, s_fboDepth);

    // check status
    assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

    GL_ERROR_CHECK("RenderTargetInit");
}

int main(int argc, char* argv[])
{
    bool fullscreen = true;
    s_config = new AppConfig(fullscreen, false, 1280, 800);
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK);
    SDL_Window* displayWindow;
    SDL_Renderer* displayRenderer;

    uint32_t flags = SDL_WINDOW_OPENGL | (s_config->fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
	int err = SDL_CreateWindowAndRenderer(s_config->width, s_config->height, flags, &displayWindow, &displayRenderer);
	if (err == -1 || !displayWindow || !displayRenderer) {
		fprintf(stderr, "SDL_CreateWindowAndRenderer failed!\n");
	}

	SDL_RendererInfo displayRendererInfo;
    SDL_GetRendererInfo(displayRenderer, &displayRendererInfo);
    /* TODO: Check that we have OpenGL */
    if ((displayRendererInfo.flags & SDL_RENDERER_ACCELERATED) == 0 ||
        (displayRendererInfo.flags & SDL_RENDERER_TARGETTEXTURE) == 0) {
        /* TODO: Handle this. We have no render surface and not accelerated. */
        fprintf(stderr, "NO RENDERER wtf!\n");
    }

    atexit(SDL_Quit);

    JOYSTICK_Init();

    AppConfig& config = *s_config;
    config.title = "riftty";

    //SDL_WM_SetCaption(config.title.c_str(), config.title.c_str());
    //SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);

    /*
    // clear
    glClearColor(s_clearColor.x, s_clearColor.y, s_clearColor.z, s_clearColor.w);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    SDL_GL_SwapWindow(displayWindow);
    */

    RiftSetup();

    RenderInit();

    win_init();

    init_config();
    cs_init();  // TODO: code pages do not want

    // TODO: determine this based on window-size & font-size or vice versa.
    cfg.rows = 25;
    cfg.cols = 80;

    // TODO: load config from /etc/riffty or ~/.rifttyrc
    finish_config();
    win_reconfig();

    // TODO: get SHELL from env

    cs_reconfig(); // TODO: do not want

    term_init();
    term_reset();
    term_resize(cfg.rows, cfg.cols);

    // TODO:
    int font_width = 10;
    int font_height = 10;
    unsigned short term_width = font_width * cfg.cols;
    unsigned short term_height = font_height * cfg.rows;

    const char* login_argv[] = {"login", "-pfl", "ajt", NULL};
    unsigned short rows = cfg.rows;
    unsigned short cols = cfg.cols;
    winsize ws = {rows, cols, term_width, term_height};
    child_create(login_argv, &ws);

    int done = 0;
    while (!done)
    {
        JOYSTICK_ClearFlags();

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
                JOYSTICK_UpdateMotion(&event.jaxis);
                break;

            case SDL_JOYBUTTONDOWN:
            case SDL_JOYBUTTONUP:
                JOYSTICK_UpdateButton(&event.jbutton);
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

    RiftShutdown();
    JOYSTICK_Shutdown();

    return 0;
}
