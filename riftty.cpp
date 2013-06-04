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

#include "OVR.h"

Vector4f s_clearColor(0, 0, 0.2, 1);

// time tracking
unsigned int s_ticks = 0;
static Matrixf s_RotY90 = Matrixf::AxisAngle(Vector3f(0,1,0), PI/2.0f);

OVR::Ptr<OVR::DeviceManager> s_pManager;
OVR::Ptr<OVR::HMDDevice> s_pHMD;
OVR::Ptr<OVR::SensorDevice> s_pSensor;
OVR::SensorFusion s_SFusion;
OVR::Util::Render::StereoConfig s_SConfig;
float s_distortionScale;

void DumpHMDInfo(const OVR::HMDInfo& hmd)
{
    printf("HResolution = %u\n", hmd.HResolution);
    printf("VResolution = %u\n", hmd.VResolution);
    printf("HScreenSize = %.3f\n", hmd.HScreenSize);
    printf("VScreenSize = %.3f\n", hmd.VScreenSize);
    printf("VScreenCenter = %.5f\n", hmd.VScreenCenter);
    printf("EyeToScreenDistance = %.5f\n", hmd.EyeToScreenDistance);
    printf("LensSeparationDistance = %.5f\n", hmd.LensSeparationDistance);
    printf("InterpupillaryDistance = %.5f\n", hmd.InterpupillaryDistance);
    printf("DistortionK = (%.5f, %.5f, %.5f, %.5f)\n", hmd.DistortionK[0], hmd.DistortionK[1], hmd.DistortionK[2], hmd.DistortionK[3]);
    printf("ChromaAbCorrection = (%.5f, %.5f, %.5f, %.5f)\n", hmd.ChromaAbCorrection[0], hmd.ChromaAbCorrection[1], hmd.ChromaAbCorrection[2], hmd.ChromaAbCorrection[3]);
}

void CreateRenderTarget();

void RiftSetup()
{
    OVR::System::Init(OVR::Log::ConfigureDefaultLog(OVR::LogMask_All));

    s_pManager = *OVR::DeviceManager::Create();
    s_pHMD     = *s_pManager->EnumerateDevices<OVR::HMDDevice>().CreateDevice();

    if (s_pHMD) {
        OVR::HMDInfo hmd;
        if (s_pHMD->GetDeviceInfo(&hmd))
        {
            DumpHMDInfo(hmd);
        }

        s_pSensor = *s_pHMD->GetSensor();
        if (s_pSensor) {
            s_SFusion.AttachToSensor(s_pSensor);
            s_SFusion.SetPredictionEnabled(true);
        }

        s_SConfig.SetFullViewport(OVR::Util::Render::Viewport(0,0, hmd.HResolution, hmd.VResolution));
        s_SConfig.SetStereoMode(OVR::Util::Render::Stereo_LeftRight_Multipass);

        // Configure proper Distortion Fit.
        // For 7" screen, fit to touch left side of the view, leaving a bit of invisible
        // screen on the top (saves on rendering cost).
        // For smaller screens (5.5"), fit to the top.
        if (hmd.HScreenSize > 0.0f) {
            if (hmd.HScreenSize > 0.140f) { // 7"
                s_SConfig.SetDistortionFitPointVP(-1.0f, 0.0f);
            } else {
                s_SConfig.SetDistortionFitPointVP(0.0f, 1.0f);
            }
        }

        printf("distortion scale = %.5f\n", s_SConfig.GetDistortionScale());
        s_distortionScale = s_SConfig.GetDistortionScale();

    } else {
        s_distortionScale = 1.71461f;

        fprintf(stderr, "NO RIFT FOUND\n");
    }

    CreateRenderTarget();
}

void RiftShutdown()
{
    OVR::System::Destroy();
}

void Process(float dt)
{
    child_poll();
}

const float kFeetToCm = 30.48;
const float kInchesToCm = 2.54;
const float kMetersToCm = 100.0;

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

    float kHResolution = s_config->width;
    float kVResolution = s_config->height;
    float kHScreenSize = 0.15;
    float kVScreenSize = 0.094;
    float kVScreenCenter = 0.04680;
    float kEyeToScreenDistance = 0.04100;
    float kLensSeparationDistance = 0.06350;
    float kInterpupillaryDistance = 0.06400;
    Vector4f kDistortionK(1.00000, 0.22000, 0.24000, 0.00000);
    Vector4f kChromaAbCorrection(0.99600, -0.00400, 1.01400, 0.00000);

    if (s_pHMD) {
        OVR::HMDInfo hmd;
        if (s_pHMD->GetDeviceInfo(&hmd)) {
            kHResolution = hmd.HResolution;
            kVResolution = hmd.VResolution;
            kHScreenSize = hmd.HScreenSize;
            kVScreenSize = hmd.VScreenSize;
            kVScreenCenter = hmd.VScreenCenter;
            kEyeToScreenDistance = hmd.EyeToScreenDistance;
            kLensSeparationDistance = hmd.LensSeparationDistance;
            kInterpupillaryDistance = hmd.InterpupillaryDistance;
            kDistortionK = Vector4f(hmd.DistortionK[0], hmd.DistortionK[1], hmd.DistortionK[2], hmd.DistortionK[3]);
            kChromaAbCorrection = Vector4f(hmd.ChromaAbCorrection[0], hmd.ChromaAbCorrection[1], hmd.ChromaAbCorrection[2], hmd.ChromaAbCorrection[3]);
        }
    }

    // Compute Aspect Ratio. Stereo mode cuts width in half.
    float aspect = float(kHResolution * 0.5f) / float(kVResolution);

    // Compute Vertical FOV based on distance.
    float halfScreenDistance = (kVScreenSize / 2);
    float yfov = 2.0f * atan(halfScreenDistance/kEyeToScreenDistance);

    // Post-projection viewport coordinates range from (-1.0, 1.0), with the
    // center of the left viewport falling at (1/4) of horizontal screen size.
    // We need to shift this projection center to match with the lens center.
    // We compute this shift in physical units (meters) to correct
    // for different screen sizes and then rescale to viewport coordinates.
    float eyeProjectionShift     = kHScreenSize * 0.25f - kLensSeparationDistance*0.5f;
    float projectionCenterOffset = 4.0f * eyeProjectionShift / kHScreenSize;

    // Projection matrix for the "center eye", which the left/right matrices are based on.
    Matrixf projCenter = Matrixf::Frustum(yfov, aspect, 10.0f, 10000.0f);
    Matrixf projLeft   = Matrixf::Trans(Vector3f(projectionCenterOffset, 0, 0)) * projCenter;
    Matrixf projRight  = Matrixf::Trans(Vector3f(-projectionCenterOffset, 0, 0)) * projCenter;

    OVR::Quatf q = s_SFusion.GetOrientation();
    Matrixf cameraMatrix = Matrixf::QuatTrans(Quatf(q.x, q.y, q.z, q.w),
                                              Vector3f(0, 6 * kFeetToCm, 0));
    Matrixf viewCenter = cameraMatrix.OrthoInverse();

    // View transformation translation in world units.
    float    halfIPD   = kInterpupillaryDistance * 0.5f * kMetersToCm;
    Matrixf viewLeft   = Matrixf::Trans(Vector3f(halfIPD, 0, 0)) * viewCenter;
    Matrixf viewRight  = Matrixf::Trans(Vector3f(-halfIPD, 0, 0)) * viewCenter;

    for (int i = 0; i < 2; i++) {

        bool left = i == 0;

        if (left) {
            glViewport(0, 0, kHResolution / 2, kVResolution);
        } else {
            glViewport(kHResolution / 2, 0, kHResolution / 2, kVResolution);
        }

        if (left) {
            glViewport(0, 0, kHResolution / 2, kVResolution);
        } else {
            glViewport(kHResolution / 2, 0, kHResolution / 2, kVResolution);
        }

        Matrixf projMatrix = left ? projLeft : projRight;
        Matrixf viewMatrix = left ? viewLeft : viewRight;

        Matrixf modelMatrix = Matrixf::ScaleQuatTrans(Vector3f(1, -1, 1),
                                                      Quatf::AxisAngle(Vector3f(0, 1, 0), 0),
                                                      Vector3f(-10 * kFeetToCm,
                                                               25 * kFeetToCm,
                                                               -10 * kFeetToCm));
        RenderBegin();

        RenderFloor(projMatrix, viewMatrix, 0.0f);

        RenderTextBegin(projMatrix, viewMatrix, modelMatrix);
        for (size_t i = 0; i < s_context.textCount; i++) {
            const GB_Text* text = s_context.text[i];
            RenderText(text->glyph_quads, text->num_glyph_quads);
        }
        RenderTextEnd();

        RenderEnd();
    }

    SDL_GL_SwapBuffers();
}

void CreateRenderTarget()
{
    float kHResolution = s_config->width;
    float kVResolution = s_config->height;

    if (s_pHMD) {
        OVR::HMDInfo hmd;
        if (s_pHMD->GetDeviceInfo(&hmd)) {
            kHResolution = hmd.HResolution;
            kVResolution = hmd.VResolution;
        }
    }

    int rtWidth = (int)ceil(s_distortionScale * kHResolution);
    int rtHeight = (int)ceil(s_distortionScale * kVResolution);

    printf("Render Target size %d x %d\n", rtWidth, rtHeight);

    GLuint s_fbo;
    GLuint s_fboDepth;
    GLuint s_fboTex;

    // create a fbo
    glGenFramebuffers(1, &s_fbo);

    // create a depth buffer
    glGenRenderbuffers(1, &s_fboDepth);

    // bind fbo
    glBindFramebuffer(GL_FRAMEBUFFER, s_fbo);

    glGenTextures(1, &s_fboTex);
    glBindTexture(GL_TEXTURE_2D, s_fboTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_LINEAR);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, rtWidth, rtHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);

    // attach texture to fbo
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D, s_fboTex, 0);

    // init depth buffer
    glBindRenderbuffer(GL_RENDERBUFFER, s_fboDepth);

    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, rtWidth, rtHeight);

    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, s_fboDepth);

    // check status
    assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
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

    RiftSetup();

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

    RiftShutdown();

    return 0;
}
