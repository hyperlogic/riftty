#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <SDL/SDL.h>

void KeyboardInit();
void ProcessKeyEvent(SDL_KeyboardEvent* key);

#endif
