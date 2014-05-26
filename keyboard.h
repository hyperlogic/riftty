#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <SDL2/SDL.h>

void KeyboardInit();
void ProcessKeyEvent(SDL_KeyboardEvent* key);

#endif
