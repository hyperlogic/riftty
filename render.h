#ifndef RENDER_H
#define RENDER_H

#include "abaci.h"
#include "gb_text.h"

void DrawTexturedQuad(uint32_t gl_tex, Vector2f const& origin, Vector2f const& size,
                      Vector2f const& uv_origin, Vector2f const& uv_size,
                      uint32_t color);

void TextRenderFunc(GB_GlyphQuad* quads, uint32_t num_quads);

#endif  // #define RENDER_H
