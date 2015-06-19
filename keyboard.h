#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <SDL.h>

void KeyboardInit();

// return true for magic quit event (meta + esc)
bool ProcessKeyEvent(SDL_KeyboardEvent* key);

#endif
