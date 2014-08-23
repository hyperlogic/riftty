
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
#include <signal.h>

#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>

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

Vector4f s_clearColor(0.0, 0, 0.3, 1);

// time tracking
unsigned int s_ticks = 0;
unsigned int s_frames = 0;
static Matrixf s_RotY90 = Matrixf::AxisAngle(Vector3f(0,1,0), PI/2.0f);

ovrHmd s_hmd;
ovrEyeRenderDesc s_eyeRenderDesc[2];
ovrSizei s_renderTargetSize;
ovrTexture s_eyeTexture[2];

uint32_t s_fbo;
uint32_t s_fboDepth;
uint32_t s_fboTex;

// TODO: get height from ovr SDK
Vector3f s_cameraPos(0, 6 * kFeetToMeters, 0);

void TermConfigInit()
{
    init_config();

    // load config from ~/.riftty
    struct passwd *pw = getpwuid(getuid());
    const char *homedir = pw->pw_dir;
    char config_filename[512];
    strncpy(config_filename, homedir, 512);
    strncat(config_filename, "/.riftty", 512);
    load_config(config_filename);

    cs_init();

    finish_config();
    win_reconfig();
}

void DumpHMDInfo(ovrHmd hmd)
{
    printf("HMDInfo\n");
    printf("    ProductName = \"%s\"\n", hmd->ProductName);
    printf("    Manufacturer = \"%s\"\n", hmd->Manufacturer);
    printf("    VendorId = %d\n", hmd->VendorId);
    printf("    ProductId = %d\n", hmd->VendorId);
    printf("    SerialNumber = ");
    for (int i = 0; i < 24; i++)
    {
        printf("%c", hmd->SerialNumber[i]);
    }
    printf("\n");
    printf("    FirmwareVersion = %d.%d\n", hmd->FirmwareMajor, hmd->FirmwareMinor);
    printf("    CamraFrustum\n");
    printf("        HFov = %0.2f\xC2\xB0\n", RadToDeg(hmd->CameraFrustumHFovInRadians));
    printf("        VFov = %0.2f\xC2\xB0\n", RadToDeg(hmd->CameraFrustumVFovInRadians));
    printf("        NearZ = %0.2fm\n", hmd->CameraFrustumNearZInMeters);
    printf("        FarZ = %0.2fm\n", hmd->CameraFrustumFarZInMeters);

    printf("    HmdCaps = 0x%x\n", hmd->HmdCaps);
    printf("        Present = %s\n", hmd->HmdCaps & ovrHmdCap_Present ? "TRUE" : "FALSE");
    printf("        Available = %s\n", hmd->HmdCaps & ovrHmdCap_Available ? "TRUE" : "FALSE");
    printf("        Captured = %s\n", hmd->HmdCaps & ovrHmdCap_Captured ? "TRUE" : "FALSE");
    printf("        ExtendDesktop = %s\n", hmd->HmdCaps & ovrHmdCap_ExtendDesktop ? "TRUE" : "FALSE");
    printf("        NoMirrorToWindow = %s\n", hmd->HmdCaps & ovrHmdCap_NoMirrorToWindow ? "TRUE" : "FALSE");
    printf("        DisplayOff = %s\n", hmd->HmdCaps & ovrHmdCap_DisplayOff ? "TRUE" : "FALSE");
    printf("        LowPersistence = %s\n", hmd->HmdCaps & ovrHmdCap_LowPersistence ? "TRUE" : "FALSE");
    printf("        DynamicPrediction = %s\n", hmd->HmdCaps & ovrHmdCap_DynamicPrediction ? "TRUE" : "FALSE");
    printf("        DynamicNoVSync = %s\n", hmd->HmdCaps & ovrHmdCap_NoVSync ? "TRUE" : "FALSE");

    printf("    TrackingCaps = 0x%x\n", hmd->TrackingCaps);
    printf("        Orientation = %s\n", hmd->TrackingCaps & ovrTrackingCap_Orientation ? "TRUE" : "FALSE");
    printf("        MagbYawCorrection = %s\n", hmd->TrackingCaps & ovrTrackingCap_MagYawCorrection ? "TRUE" : "FALSE");
    printf("        Position = %s\n", hmd->TrackingCaps & ovrTrackingCap_Position ? "TRUE" : "FALSE");

    printf("    DistortionCaps = 0x%x\n", hmd->DistortionCaps);
    printf("        Chromatic = %s\n", hmd->DistortionCaps & ovrDistortionCap_Chromatic ? "TRUE" : "FALSE");
    printf("        TimeWarp = %s\n", hmd->DistortionCaps & ovrDistortionCap_TimeWarp ? "TRUE" : "FALSE");
    printf("        Vignette = %s\n", hmd->DistortionCaps & ovrDistortionCap_Vignette ? "TRUE" : "FALSE");
    printf("        NoRestore = %s\n", hmd->DistortionCaps & ovrDistortionCap_NoRestore ? "TRUE" : "FALSE");
    printf("        FlipInput = %s\n", hmd->DistortionCaps & ovrDistortionCap_FlipInput ? "TRUE" : "FALSE");
    printf("        SRGB = %s\n", hmd->DistortionCaps & ovrDistortionCap_SRGB ? "TRUE" : "FALSE");
    printf("        Overdrive = %s\n", hmd->DistortionCaps & ovrDistortionCap_Overdrive ? "TRUE" : "FALSE");

    printf("    DefaultEyeFov = [(%f, %f, %f, %f), (%f, %f, %f, %f])]\n",
           hmd->DefaultEyeFov[0].UpTan, hmd->DefaultEyeFov[0].DownTan, hmd->DefaultEyeFov[0].LeftTan, hmd->DefaultEyeFov[0].RightTan,
           hmd->DefaultEyeFov[1].UpTan, hmd->DefaultEyeFov[1].DownTan, hmd->DefaultEyeFov[1].LeftTan, hmd->DefaultEyeFov[1].RightTan);
    printf("    MaxEyeFov = [(%f, %f, %f, %f), (%f, %f, %f, %f])]\n",
           hmd->MaxEyeFov[0].UpTan, hmd->MaxEyeFov[0].DownTan, hmd->MaxEyeFov[0].LeftTan, hmd->MaxEyeFov[0].RightTan,
           hmd->MaxEyeFov[1].UpTan, hmd->MaxEyeFov[1].DownTan, hmd->MaxEyeFov[1].LeftTan, hmd->MaxEyeFov[1].RightTan);
    printf("    EyeRenderOrder = %s, %s\n", hmd->EyeRenderOrder[0] == ovrEye_Left ? "Left" : "Right", hmd->EyeRenderOrder[1] == ovrEye_Left ? "Left" : "Right");

    printf("    Resolution = %d x %d\n", hmd->Resolution.w, hmd->Resolution.h);
    printf("    WindowsPos = (%d, %d)\n", hmd->WindowsPos.x, hmd->WindowsPos.y);

    printf("    DisplayDeviceName = %s\n", hmd->DisplayDeviceName);
    printf("    DisplayId = %d\n", hmd->DisplayId);
}

void CreateRenderTarget(int width, int height);

void RiftInit()
{
    ovr_Initialize();

    s_hmd = ovrHmd_Create(0);
    if (!s_hmd)
    {
        s_hmd = ovrHmd_CreateDebug(ovrHmd_DK1);
    }

    DumpHMDInfo(s_hmd);
}

void TermInit()
{
    win_init();

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

    char login[128];
    strncpy(login, getlogin(), 128);
    const char* login_argv[] = {"login", "-pfl", login, NULL};
    unsigned short rows = cfg.rows;
    unsigned short cols = cfg.cols;
    winsize ws = {rows, cols, term_width, term_height};
    child_create(login_argv, &ws);
}

void TermShutdown()
{
    child_kill(true);
    win_shutdown();
}

void RiftConfigure()
{
    uint32_t supportedSensorCaps = ovrTrackingCap_Orientation | ovrTrackingCap_MagYawCorrection | ovrTrackingCap_Position;
    uint32_t requiredTrackingCaps = 0;

    ovrBool success = ovrHmd_ConfigureTracking(s_hmd, supportedSensorCaps, 0);
    if (!success) {
        fprintf(stderr, "ERROR: HMD does not have required capabilities!\n");
        exit(2);
    }

    // Figure out dimensions of render target
    ovrSizei recommenedTex0Size = ovrHmd_GetFovTextureSize(s_hmd, ovrEye_Left, s_hmd->DefaultEyeFov[0], 1.0f);
    ovrSizei recommenedTex1Size = ovrHmd_GetFovTextureSize(s_hmd, ovrEye_Right, s_hmd->DefaultEyeFov[1], 1.0f);
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
    cfg.OGL.Header.RTSize = {s_hmd->Resolution.w, s_hmd->Resolution.h};
    cfg.OGL.Header.Multisample = 0;
    // TODO: on windows need to set HWND, on Linux need to set other parameters
    if (!ovrHmd_ConfigureRendering(s_hmd, &cfg.Config, s_hmd->DistortionCaps & ~ovrDistortionCap_FlipInput, s_hmd->DefaultEyeFov, s_eyeRenderDesc))
    {
        fprintf(stderr, "ERROR: HMD configure rendering failed!\n");
        exit(3);
    }

    ovrHmd_DismissHSWDisplay(s_hmd);
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

    ovrTrackingState ts = ovrHmd_GetTrackingState(s_hmd, ovr_GetTimeInSeconds());

    Vector3f posTrackOffset(0.0f, 0.0f, 0.0f);
    if (ts.StatusFlags & (ovrStatus_PositionTracked)) {
        ovrPosef pose = ts.HeadPose.ThePose;
        posTrackOffset.Set(pose.Position.x, pose.Position.y, pose.Position.z);
    }

    ovrPosef headPose[2];

    for (int i = 0; i < 2; i++)
    {
        ovrEyeType eye = s_hmd->EyeRenderOrder[i];
        ovrPosef pose = ovrHmd_GetEyePose(s_hmd, eye);
        headPose[eye] = pose;

        glViewport(s_eyeTexture[eye].Header.RenderViewport.Pos.x,
                   s_eyeTexture[eye].Header.RenderViewport.Pos.y,
                   s_eyeTexture[eye].Header.RenderViewport.Size.w,
                   s_eyeTexture[eye].Header.RenderViewport.Size.h);

        Quatf q(pose.Orientation.x, pose.Orientation.y, pose.Orientation.z, pose.Orientation.w);
        Vector3f p(pose.Position.x, pose.Position.y, pose.Position.z);

        Matrixf cameraMatrix = Matrixf::QuatTrans(q, s_cameraPos + posTrackOffset);
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
        const float kTermScale = 0.0008f;
        const Vector3f termOrigin(-2.6f * kFeetToMeters, 6.75f * kFeetToMeters, -2.5 * kFeetToMeters);
        Matrixf modelMatrix = Matrixf::ScaleQuatTrans(Vector3f(kTermScale, -kTermScale, kTermScale),
                                                      Quatf::AxisAngle(Vector3f(0, 1, 0), 0),
                                                      termOrigin);
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
    }

    ovrHmd_EndFrame(s_hmd, headPose, s_eyeTexture);
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

static bool s_needsRestart = false;
void RestartHandler(int signal)
{
    printf("NEEDS RESTART! pid = %d\n", getpid());
    // NOTE: I could call execvp directly here, but the process will
    // inherit the signal mask which will not let me hook up the SIGUSR1 handler.
    // So we set a flag instead.
    s_needsRestart = true;
}

void Restart()
{
    printf("RESTART! pid = %d\n", getpid());
    char* const argv[] = {"./riftty", NULL};
    int err = execvp(*argv, argv);
    if (err == -1)
        perror("execvp");
}

int main(int argc, char* argv[])
{
    printf("START! pid = %d\n", getpid());

    if (!system("./restart.rb"))
        exit(1);

    if (signal(SIGUSR1, RestartHandler) == SIG_ERR) {
        fprintf(stderr, "An error occurred while setting a signal handler.\n");
        exit(1);
    }

    TermConfigInit();
    RiftInit();

    bool fullscreen = false;
    s_config = new AppConfig(fullscreen, false, s_hmd->Resolution.w, s_hmd->Resolution.h);
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK);
    SDL_Window* displayWindow;
    SDL_Renderer* displayRenderer;

    uint32_t flags = SDL_WINDOW_OPENGL | (s_config->fullscreen ? SDL_WINDOW_FULLSCREEN : 0);
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

    RiftConfigure();
    RenderInit();
    TermInit();

    bool done = false;
    while (!done)
    {
        JOYSTICK_ClearFlags();

        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
            case SDL_QUIT:
                done = true;
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
                if (ProcessKeyEvent(&event.key)) {
                    done = true;
                }
                break;
            }
        }

        if (!done)
        {
            if (s_needsRestart)
                Restart();

            unsigned int now = SDL_GetTicks();  // milliseconds
            float dt = (now - s_ticks) / 1000.0f;	// convert to seconds.
            s_ticks = now;

            //printf("fps = %.0f\n", 1.0f/dt);

            Process(dt);
            Render(dt);

            s_frames++;
        }
    }

    TermShutdown();
    RiftShutdown();
    JOYSTICK_Shutdown();

    return 0;
}
