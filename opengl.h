#ifndef OPENGL_H
#define OPENGL_H

#ifdef __APPLE__
	#include "TargetConditionals.h"
#endif

#if defined DARWIN
	#include <OpenGL/gl.h>
	#include <OpenGL/glu.h>
#elif TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
	#include <OpenGLES/ES2/gl.h>
	#include <OpenGLES/ES2/glext.h>
#else
	#include <GL/gl.h>
	#include <GL/glext.h>
	#include <GL/glu.h>
#endif

#endif // #define OPENGL_H
