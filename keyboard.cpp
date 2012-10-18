#include "keyboard.h"
#include <SDL/SDL.h>

// used for key repeat tracking
static float s_repeatTimer = 0.0f;
static int s_repeatAscii = 0;
static const float kRepeatDelay = 0.5f;
static const float kRepeatRate = 0.035f;
RepeatCallback s_repeatCallback = 0;

void KeyboardInit()
{
	;
}

static int SymToAscii(int sym, int mod)
{
	if (sym < 128)
	{
		char ascii = sym;
	    if (mod & KMOD_SHIFT)
		{
			if (ascii >= 'a' && ascii <= 'z')
				ascii = 'A' + (ascii - 'a');
			else
			{
				switch (ascii)
				{
					case '`': ascii = '~'; break;
					case '1': ascii = '!'; break;
					case '2': ascii = '@'; break;
					case '3': ascii = '#'; break;
					case '4': ascii = '$'; break;
					case '5': ascii = '%'; break;
					case '6': ascii = '^'; break;
					case '7': ascii = '&'; break;
					case '8': ascii = '*'; break;
					case '9': ascii = '('; break;
					case '0': ascii = ')'; break;
					case '-': ascii = '_'; break;
					case '=': ascii = '+'; break;
					case '[': ascii = '{'; break;
					case ']': ascii = '}'; break;
					case '\\': ascii = '|'; break;
					case ';': ascii = ':'; break;
					case '\'': ascii = '\"'; break;
					case ',': ascii = '<'; break;
					case '.': ascii = '>'; break;
					case '/': ascii = '?'; break;
					default:
					break;
				}
			}
		}
		return ascii;
	}
	else
	{
		// handle arrow keys
		switch (sym)
		{
			case SDLK_UP: return 1;
			case SDLK_DOWN: return 2;
			case SDLK_RIGHT: return 3;
			case SDLK_LEFT: return 4;
		}
	}

	return 0;
}

static void KeyEvent(int ascii, bool down)
{
	if (down)
	{
		s_repeatAscii = ascii;
		s_repeatTimer = kRepeatDelay;
	}
	else
	{
		s_repeatAscii = 0;
	}

	if (s_repeatCallback)
        s_repeatCallback(ascii, down);
}

void KeyboardProcess(float dt)
{
	// check for repeats
	if (s_repeatAscii)
	{
		s_repeatTimer -= dt;
		if (s_repeatTimer <= 0.0f)
		{
			s_repeatTimer = kRepeatRate + s_repeatTimer;

            if (s_repeatCallback)
                s_repeatCallback(s_repeatAscii, true);
		}
	}
}

void SetRepeatKeyCallback(RepeatCallback cb)
{
    s_repeatCallback = cb;
}

void ProcessKeyEvent(SDL_KeyboardEvent* key)
{
	bool down = (key->type == SDL_KEYDOWN);
	int sym = key->keysym.sym;
	int mod = key->keysym.mod;
	int ascii = SymToAscii(sym, mod);
	if (ascii)
	{
	    KeyEvent(ascii, down);
	}
	else
	{
		// kill repeats if the modifier state changes.
		switch (sym)
		{
			case SDLK_RSHIFT:
			case SDLK_LSHIFT:
			case SDLK_RCTRL:
			case SDLK_LCTRL:
			case SDLK_RALT:
			case SDLK_LALT:
			case SDLK_RMETA:
			case SDLK_LMETA:
				s_repeatAscii = 0;
				break;
		}
	}
}
