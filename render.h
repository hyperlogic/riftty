#ifndef RENDER_H
#define RENDER_H

#include <vector>
#include "abaci.h"

namespace gb {
    struct Quad;
}

void RenderInit(bool verbose = false);

void RenderBegin();
void RenderEnd();

void RenderTextBegin(const Matrixf& projMatrix, const Matrixf& viewMatrix, const Matrixf& modelMatrix);
void RenderText(const std::vector<gb::Quad>& quadVec);
void RenderTextEnd();

void RenderFloor(const Matrixf& projMatrix, const Matrixf& viewMatrix, float height);
void RenderScreenAlignedQuad(unsigned int texture, Vector2f viewportSize, Vector2f quadPos, Vector2f quadSize);

#ifdef DEBUG
#define GL_ERROR_CHECK(x) GLErrorCheck(x)
void GLErrorCheck(const char* message);
#else
#define GL_ERROR_CHECK(x)
#endif

#endif  // #define RENDER_H
