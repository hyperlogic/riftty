#include "render.h"
#include "shader.h"
#include "opengl.h"
#include "win.h"
#include "text.h"

#include "FullbrightShader.h"
#include "FullbrightVertColorShader.h"
#include "FullbrightTexturedShader.h"
#include "FullbrightTexturedTextShader.h"
#include "FullbrightTexturedVertColorShader.h"
#include "FullbrightTexturedVertColorTextShader.h"
#include "PhongTexturedShader.h"

#ifdef DEBUG
// If there is a glError this outputs it along with a message to stderr.
// otherwise there is no output.
void GLErrorCheck(const char* message)
{
    GLenum val = glGetError();
    switch (val)
    {
    case GL_INVALID_ENUM:
        fprintf(stderr, "GL_INVALID_ENUM : %s\n", message);
        break;
    case GL_INVALID_VALUE:
        fprintf(stderr, "GL_INVALID_VALUE : %s\n", message);
        break;
    case GL_INVALID_OPERATION:
        fprintf(stderr, "GL_INVALID_OPERATION : %s\n", message);
        break;
#ifndef GL_ES_VERSION_2_0
    case GL_STACK_OVERFLOW:
        fprintf(stderr, "GL_STACK_OVERFLOW : %s\n", message);
        break;
    case GL_STACK_UNDERFLOW:
        fprintf(stderr, "GL_STACK_UNDERFLOW : %s\n", message);
        break;
#endif
    case GL_OUT_OF_MEMORY:
        fprintf(stderr, "GL_OUT_OF_MEMORY : %s\n", message);
        break;
    case GL_NO_ERROR:
        break;
    }
}
#endif

FullbrightShader* s_fullbrightShader = 0;
FullbrightVertColorShader* s_fullbrightVertColorShader = 0;
FullbrightTexturedShader* s_fullbrightTexturedShader = 0;
FullbrightTexturedTextShader* s_fullbrightTexturedTextShader = 0;
FullbrightTexturedVertColorShader* s_fullbrightTexturedVertColorShader = 0;
FullbrightTexturedVertColorTextShader* s_fullbrightTexturedVertColorTextShader = 0;
PhongTexturedShader* s_phongTexturedShader = 0;
Shader* s_prevShader = 0;
GLuint s_checker = 0;

static GLint CreateCheckerTexture()
{
    GLuint retVal = 0;
    glGenTextures(1, &retVal);
    glBindTexture(GL_TEXTURE_2D, retVal);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    const int W = 512;
    const uint8_t O = 100;
    const uint8_t X = 255;
    uint8_t* checker = new uint8_t[W * W];
    int i, j;
    for (i = 0; i < W; i++) {
        for (j = 0; j < W; j++) {
            if (i < W/2) {
                if (j < W/2)
                    checker[i * W + j] = O;
                else
                    checker[i * W + j] = X;
            } else {
                if (j < W/2)
                    checker[i * W + j] = X;
                else
                    checker[i * W + j] = O;
            }
        }
    }

    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, W, W, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, checker);
    delete [] checker;
    return retVal;
}

void RenderInit()
{
    s_fullbrightShader = new FullbrightShader();
    if (!s_fullbrightShader->compileAndLink())
        exit(EXIT_FAILURE);

    s_fullbrightVertColorShader = new FullbrightVertColorShader();
    if (!s_fullbrightVertColorShader->compileAndLink())
        exit(EXIT_FAILURE);

    s_fullbrightTexturedShader = new FullbrightTexturedShader();
    if (!s_fullbrightTexturedShader->compileAndLink())
        exit(EXIT_FAILURE);

    s_fullbrightTexturedTextShader = new FullbrightTexturedTextShader();
    if (!s_fullbrightTexturedTextShader->compileAndLink())
        exit(EXIT_FAILURE);

    s_fullbrightTexturedVertColorShader = new FullbrightTexturedVertColorShader();
    if (!s_fullbrightTexturedVertColorShader->compileAndLink())
        exit(EXIT_FAILURE);

    s_fullbrightTexturedVertColorTextShader = new FullbrightTexturedVertColorTextShader();
    if (!s_fullbrightTexturedVertColorTextShader->compileAndLink())
        exit(EXIT_FAILURE);

    s_phongTexturedShader = new PhongTexturedShader();
    if (!s_phongTexturedShader->compileAndLink())
        exit(EXIT_FAILURE);

    s_checker = CreateCheckerTexture();
}

void RenderShutdown()
{
    delete s_fullbrightShader;
    delete s_fullbrightVertColorShader;
    delete s_fullbrightTexturedShader;
    delete s_fullbrightTexturedTextShader;
    delete s_fullbrightTexturedVertColorShader;
    delete s_fullbrightTexturedVertColorTextShader;
    delete s_phongTexturedShader;
}

static Vector4f UintColorToVector4(uint32_t color)
{
    return Vector4f((color & 0xff) / 255.0f,
                    ((color >> 8) & 0xff) / 255.0f,
                    ((color >> 16) & 0xff) / 255.0f,
                    ((color >> 24) & 0xff) / 255.0f);
}

void RenderBegin()
{
    s_prevShader = 0;
}

void RenderEnd()
{

}

void RenderTextBegin(const Matrixf& projMatrix, const Matrixf& viewMatrix, const Matrixf& modelMatrix)
{
    Matrixf fullMatrix = projMatrix * viewMatrix * modelMatrix;
    s_fullbrightShader->setMat(fullMatrix);
    s_fullbrightVertColorShader->setMat(fullMatrix);
    s_fullbrightTexturedShader->setMat(fullMatrix);
    s_fullbrightTexturedTextShader->setMat(fullMatrix);
    s_fullbrightTexturedVertColorShader->setMat(fullMatrix);
    s_fullbrightTexturedVertColorTextShader->setMat(fullMatrix);
}

void RenderText(const std::vector<gb::Quad>& quadVec)
{
    static uint16_t indices[] = {0, 2, 1, 2, 3, 1};

    static std::vector<float> s_attribVec;
    static std::vector<uint16_t> s_indexVec;

    s_attribVec.clear();
    s_indexVec.clear();

    // disable depth writes
    glDepthMask(GL_FALSE);

    Vector2f minCorner(FLT_MAX, FLT_MAX);
    Vector2f maxCorner(-FLT_MAX, -FLT_MAX);

    // draw bg quads
    int i = 0;
    for (auto &quad : quadVec)
    {
        const WIN_TextUserData* data = (const WIN_TextUserData*)quad.userData;

        Vector4f bg_color = UintColorToVector4(data->bg_color);
        // TODO: but this in cfg
        bg_color.w = 0.85f;

        uint32_t y_offset = data->line_height / 3; // hack
        Vector2f origin = Vector2f(quad.pen.x, quad.pen.y + y_offset);
        Vector2f size = Vector2f(data->max_advance, -(float)data->line_height);

        // keep track of min and max corner.
        if (i == 0)
            minCorner = origin;
        else if (i == quadVec.size() - 1)
            maxCorner = origin + size;

        float attrib[28] = {
            origin.x, origin.y, 0,
            bg_color.x, bg_color.y, bg_color.z, bg_color.w,
            origin.x + size.x, origin.y, 0,
            bg_color.x, bg_color.y, bg_color.z, bg_color.w,
            origin.x, origin.y + size.y, 0,
            bg_color.x, bg_color.y, bg_color.z, bg_color.w,
            origin.x + size.x, origin.y + size.y, 0,
            bg_color.x, bg_color.y, bg_color.z, bg_color.w
        };
        for (int j = 0; j < 28; j++)
            s_attribVec.push_back(attrib[j]);
        for (int j = 0; j < 6; j++)
            s_indexVec.push_back(4 * i + indices[j]);
        i++;
    }
    s_fullbrightVertColorShader->apply(s_prevShader, reinterpret_cast<float*>(&s_attribVec[0]));
    s_prevShader = s_fullbrightVertColorShader;
    glDrawElements(GL_TRIANGLES, s_indexVec.size(), GL_UNSIGNED_SHORT, reinterpret_cast<uint16_t*>(&s_indexVec[0]));

    s_attribVec.clear();
    s_indexVec.clear();

    // hack: assuming the texture for the first quad is relevent for all of them!
    s_fullbrightTexturedVertColorTextShader->setTex(quadVec[0].glTexObj);
    s_fullbrightTexturedVertColorTextShader->setLodBias(-1.0f);

    // draw fg glyphs
    const float kDepthOffset = 0.0f;
    i = 0;
    for (auto &quad : quadVec) {
        const WIN_TextUserData* data = (const WIN_TextUserData*)quad.userData;

        if (quad.size.x > 0 && quad.size.y > 0)
        {
            const float inset = 2.0f;
            const float uvInset = inset / 1024.0f; // HACK 1024 is hardcoded size of font texture cache.
            Vector2f origin = Vector2f(quad.origin.x + inset, quad.origin.y + inset);
            Vector2f size = Vector2f(quad.size.x - 2*inset, quad.size.y - 2*inset);
            Vector2f uv_origin = Vector2f(quad.uvOrigin.x + uvInset, quad.uvOrigin.y + uvInset);
            Vector2f uv_size = Vector2f(quad.uvSize.x - 2*uvInset, quad.uvSize.y - 2*uvInset);
            Vector4f fg_color = UintColorToVector4(data->fg_color);
            float attrib[36] = {
                origin.x, origin.y, kDepthOffset, uv_origin.x, uv_origin.y,
                fg_color.x, fg_color.y, fg_color.z, fg_color.w,
                origin.x + size.x, origin.y, kDepthOffset, uv_origin.x + uv_size.x, uv_origin.y,
                fg_color.x, fg_color.y, fg_color.z, fg_color.w,
                origin.x, origin.y + size.y, kDepthOffset, uv_origin.x, uv_origin.y + uv_size.y,
                fg_color.x, fg_color.y, fg_color.z, fg_color.w,
                origin.x + size.x, origin.y + size.y, kDepthOffset, uv_origin.x + uv_size.x, uv_origin.y + uv_size.y,
                fg_color.x, fg_color.y, fg_color.z, fg_color.w
            };

            for (int j = 0; j < 36; j++)
                s_attribVec.push_back(attrib[j]);
            for (int j = 0; j < 6; j++)
                s_indexVec.push_back(4 * i + indices[j]);
            i++;
        }
    }

    s_fullbrightTexturedVertColorTextShader->apply(s_prevShader, reinterpret_cast<float*>(&s_attribVec[0]));
    s_prevShader = s_fullbrightTexturedVertColorTextShader;
    glDrawElements(GL_TRIANGLES, s_indexVec.size(), GL_UNSIGNED_SHORT, reinterpret_cast<uint16_t*>(&s_indexVec[0]));

    // enable depth writes
    glDepthMask(GL_TRUE);

    // disable color buffer writes
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

    if (quadVec.size())
    {
        float pos[12] = {
            minCorner.x, minCorner.y, 0,
            maxCorner.x, minCorner.y, 0,
            minCorner.x, maxCorner.y, 0,
            maxCorner.x, maxCorner.y, 0
        };

        s_fullbrightShader->apply(s_prevShader, pos);
        s_prevShader = s_fullbrightShader;

        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indices);
    }

    // re-enable color buffer writes.
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    GL_ERROR_CHECK("TextRenderFunc");
}

void RenderTextEnd()
{

}

void RenderFloor(const Matrixf& projMatrix, const Matrixf& viewMatrix, float height)
{
    Matrixf worldMatrix = Matrixf::Trans(Vector3f(0, height, 0));
    Matrixf normalMatrix = worldMatrix;
    normalMatrix.SetTrans(Vector3f(0, 0, 0));

    const float kFeetToMeters = 0.3048;

    s_phongTexturedShader->setFullMat(projMatrix * viewMatrix * worldMatrix);
    s_phongTexturedShader->setWorldMat(worldMatrix);
    s_phongTexturedShader->setWorldNormalMat(normalMatrix);
    s_phongTexturedShader->setColor(Vector4f(1, 1, 1, 1));
    s_phongTexturedShader->setTex(s_checker);

    static bool init = false;
    if (!init) {
        std::vector<Vector3f> lightWorldPos;
        lightWorldPos.push_back(Vector3f(0, 4 * kFeetToMeters, 0));
        lightWorldPos.push_back(Vector3f(10 * kFeetToMeters, -1, -1.0f));
        lightWorldPos.push_back(Vector3f(0, 4 * kFeetToMeters, 0));
        s_phongTexturedShader->setLightWorldPos(lightWorldPos);

        std::vector<Vector3f> lightColor;
        lightColor.push_back(Vector3f(1, 1, 1));
        lightColor.push_back(Vector3f(1, 0, 0));
        lightColor.push_back(Vector3f(0, 0, 1));
        s_phongTexturedShader->setLightColor(lightColor);

        std::vector<float> lightStrength;
        lightStrength.push_back(10);
        lightStrength.push_back(5);
        lightStrength.push_back(5);
        s_phongTexturedShader->setLightStrength(lightStrength);

        s_phongTexturedShader->setNumLights(3);
        init = true;
    }

    float kOffset = 1000.0f * kFeetToMeters;
    float kTexOffset = 100.0f;
    float attrib[32] = {
        0 - kOffset, 0, 0 - kOffset, 0, 0, 0, 1, 0,
        0 + kOffset, 0, 0 - kOffset, 0 + kTexOffset, 0, 0, 1, 0,
        0 - kOffset, 0, 0 + kOffset, 0, 0 + kTexOffset, 0, 1, 0,
        0 + kOffset, 0, 0 + kOffset, 0 + kTexOffset, 0 + kTexOffset, 0, 1, 0
    };
    s_phongTexturedShader->apply(s_prevShader, attrib);

    static uint16_t indices[] = {0, 2, 1, 2, 3, 1};
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indices);
}

void RenderScreenAlignedQuad(GLuint texture, Vector2f viewportSize, Vector2f quadPos, Vector2f quadSize)
{
    Matrixf projMatrix = Matrixf::Ortho(0, viewportSize.x, 0, viewportSize.y, -1, 1);

    s_fullbrightTexturedShader->setMat(projMatrix);
    s_fullbrightTexturedShader->setColor(Vector4f(1, 1, 1, 1));
    s_fullbrightTexturedShader->setTex(texture);

    float attrib[20] = {
        quadPos.x, quadPos.y, 0, 0, 0,
        quadPos.x + quadSize.x, quadPos.y, 0, 1, 0,
        quadPos.x, quadPos.y + quadSize.y, 0, 0, 1,
        quadPos.x + quadSize.x, quadPos.y + quadSize.y, 0, 1, 1
    };

    s_fullbrightTexturedShader->apply(s_prevShader, attrib);
    s_prevShader = s_fullbrightTexturedShader;

    static uint16_t indices[] = {0, 2, 1, 2, 3, 1};
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indices);
}
