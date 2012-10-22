#include "render.h"
#include "opengl.h"
#include "appconfig.h"

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
