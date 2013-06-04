#ifndef RENDER_H
#define RENDER_H

#include "abaci.h"
#include "gb_text.h"

void RenderInit();

void RenderBegin();
void RenderEnd();

void RenderTextBegin(const Matrixf& projMatrix, const Matrixf& viewMatrix, const Matrixf& modelMatrix);
void RenderText(GB_GlyphQuad* quads, uint32_t num_quads);
void RenderTextEnd();

void RenderFloor(const Matrixf& projMatrix, const Matrixf& viewMatrix, float height);

void RenderFullScreenQuad(uint32_t texture, int width, int height);

#ifdef DEBUG
#define GL_ERROR_CHECK(x) GLErrorCheck(x)
void GLErrorCheck(const char* message);
#else
#define GL_ERROR_CHECK(x)
#endif

#endif  // #define RENDER_H
