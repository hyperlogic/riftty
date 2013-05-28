#include "render.h"
#include "shader.h"
#include "opengl.h"
#include "appconfig.h"
#include "win.h"

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

class FullbrightShader : public Shader
{
public:
    FullbrightShader() : Shader(), mat_loc(-1), color_loc(-1), pos_loc(-1) {}

    Matrixf mat;
    mutable int mat_loc;
    Vector4f color;
    mutable int color_loc;
    mutable int pos_loc;

    void apply(const float* attribPtr, int stride) const
    {
        if (mat_loc < 0)
            mat_loc = getUniformLoc("mat");
        if (color_loc < 0)
            color_loc = getUniformLoc("color");
        if (pos_loc < 0)
            pos_loc = getAttribLoc("pos");

        assert(mat_loc >= 0);
        assert(color_loc >= 0);

        Shader::apply(0);
        glUniformMatrix4fv(mat_loc, 1, false, (float*)&mat);
        glUniform4fv(color_loc, 1, (float*)&color);

        glVertexAttribPointer(pos_loc, 3, GL_FLOAT, false, stride, attribPtr);
    }
};

class FullbrightTexturedShader : public FullbrightShader
{
public:
    FullbrightTexturedShader() : FullbrightShader(), tex_loc(-1), uv_loc(-1) {}

    int tex;
    mutable int tex_loc;
    mutable int uv_loc;

    void apply(const float* attribPtr, int stride) const
    {
        if (tex_loc < 0)
            tex_loc = getUniformLoc("tex");
        if (uv_loc < 0)
            uv_loc = getAttribLoc("uv");

        assert(tex_loc >= 0);

        FullbrightShader::apply(attribPtr, stride);

        glUniform1i(tex_loc, 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex);

        glVertexAttribPointer(uv_loc, 2, GL_FLOAT, false, stride, attribPtr + 3);
    }
};


FullbrightShader* s_fullbrightShader = 0;
FullbrightTexturedShader* s_fullbrightTexturedShader = 0;

void RenderInit()
{
    s_fullbrightShader = new FullbrightShader();
    s_fullbrightShader->compileAndLinkFromFiles("shader/fullbright.vsh",
                                                "shader/fullbright.fsh");

    s_fullbrightTexturedShader = new FullbrightTexturedShader();
    s_fullbrightTexturedShader->compileAndLinkFromFiles("shader/fullbright_textured.vsh",
                                                       "shader/fullbright_textured_text.fsh");
}

void RenderShutdown()
{
    delete s_fullbrightShader;
    delete s_fullbrightTexturedShader;
}

void DrawUntexturedQuad(Vector2f const& origin, Vector2f const& size, uint32_t color)
{
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

    uint32_t colors[8];
    for (int i = 0; i < 8; i++)
        colors[i] = color;

    glEnableClientState(GL_COLOR_ARRAY);
    glColorPointer(4, GL_UNSIGNED_BYTE, 0, colors);

    static uint16_t indices[] = {0, 2, 1, 2, 3, 1};
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indices);
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

static Vector4f UintColorToVector4(uint32_t color)
{
    return Vector4f((color & 0xff) / 255.0f,
                    ((color >> 8) & 0xff) / 255.0f,
                    ((color >> 16) & 0xff) / 255.0f,
                    ((color >> 24) & 0xff) / 255.0f);
}

void TextRenderFunc(GB_GlyphQuad* quads, uint32_t num_quads)
{
    // note this flips y-axis so y is down.
    Matrixf proj = Matrixf::Ortho(0, s_config->width, s_config->height, 0, -10, 10);

    s_fullbrightShader->mat = proj;
    s_fullbrightTexturedShader->mat = proj;

    /*
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(reinterpret_cast<float*>(&proj));
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    */
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    for (uint32_t i = 0; i < num_quads; ++i) {

        /*
        {
            printf("quad[%d]\n", i);
            printf("    pen = [%d, %d]\n", quads[i].pen[0], quads[i].pen[1]);
            printf("    origin = [%d, %d]\n", quads[i].origin[0], quads[i].origin[1]);
            printf("    size = [%d, %d]\n", quads[i].size[0], quads[i].size[1]);
            printf("    uv_origin = [%.3f, %.3f]\n", quads[i].uv_origin[0], quads[i].uv_origin[1]);
            printf("    uv_size = [%.3f, %.3f]\n", quads[i].uv_size[0], quads[i].uv_size[1]);
            printf("    gl_tex_obj = %u\n", quads[i].gl_tex_obj);
            printf("    user_data = %p\n", quads[i].user_data);
        }
        */

        const WIN_TextUserData* data = (const WIN_TextUserData*)quads[i].user_data;

        s_fullbrightShader->color = UintColorToVector4(data->bg_color);

        uint32_t y_offset = data->line_height / 3; // hack
        Vector2f origin = Vector2f(quads[i].pen[0], quads[i].pen[1] + y_offset);
        Vector2f size = Vector2f(data->max_advance, -(float)data->line_height);

        float pos[12] = {
            origin.x, origin.y, 0,
            origin.x + size.x, origin.y, 0,
            origin.x, origin.y + size.y, 0,
            origin.x + size.x, origin.y + size.y
        };

        s_fullbrightShader->apply(pos, 0);

        static uint16_t indices[] = {0, 2, 1, 2, 3, 1};
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indices);

        if (quads[i].size[0] > 0 && quads[i].size[1] > 0)
        {
            Vector2f origin = Vector2f(quads[i].origin[0], quads[i].origin[1]);
            Vector2f size = Vector2f(quads[i].size[0], quads[i].size[1]);
            Vector2f uv_origin = Vector2f(quads[i].uv_origin[0], quads[i].uv_origin[1]);
            Vector2f uv_size = Vector2f(quads[i].uv_size[0], quads[i].uv_size[1]);
            float attrib[20] = {
                origin.x, origin.y, 0, uv_origin.x, uv_origin.y,
                origin.x + size.x, origin.y, 0, uv_origin.x + uv_size.x, uv_origin.y,
                origin.x, origin.y + size.y, 0, uv_origin.x, uv_origin.y + uv_size.y,
                origin.x + size.x, origin.y + size.y, 0, uv_origin.x + uv_size.x, uv_origin.y + uv_size.y
            };

            s_fullbrightTexturedShader->color = UintColorToVector4(data->fg_color);
            s_fullbrightTexturedShader->tex = quads[i].gl_tex_obj;
            s_fullbrightTexturedShader->apply(attrib, 4 * 5);

            /*
            s_fullbrightShader->color = UintColorToVector4(data->fg_color);
            s_fullbrightShader->apply(attrib, 4 * 5);
            */


            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indices);

            /*
            DrawTexturedQuad(quads[i].gl_tex_obj,
                             Vector2f(quads[i].origin[0], quads[i].origin[1]),
                             Vector2f(quads[i].size[0], quads[i].size[1]),
                             Vector2f(quads[i].uv_origin[0], quads[i].uv_origin[1]),
                             Vector2f(quads[i].uv_size[0], quads[i].uv_size[1]),
                             data->fg_color);
            */
        }
    }
    GL_ERROR_CHECK("TextRenderFunc");
}
