#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <SDL/SDL.h>

typedef void (*RepeatCallback)(int ascii, bool down);

void KeyboardInit();
void KeyboardProcess(float dt);
void SetRepeatKeyCallback(RepeatCallback cb);
void ProcessKeyEvent(SDL_KeyboardEvent* key);

#endif
