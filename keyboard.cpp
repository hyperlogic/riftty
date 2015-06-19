#include "keyboard.h"
#include <SDL.h>
#include <termios.h>

extern "C" {
#include "config.h"
#include "child.h"
#include "term.h"
}

void KeyboardInit()
{
	;
}

static char SymToAscii(int sym, int mod)
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

    return 0;
}

struct ModState {
    bool numlock;
    bool shift;
    bool lalt;
    bool ralt;
    bool alt;
    bool lmeta;
    bool rmeta;
    bool meta;
    bool lctrl;
    bool rctrl;
    bool ctrl;
    bool ctrl_lalt_altgr;
    bool altgr;
    int mods;
};

static void ss3(char c)
{
    char buf[3];
    int len = 0;
    buf[len++] = '\e';
    buf[len++] = 'O';
    buf[len++] = c;
    assert(len == 3);
    child_send(buf, len);
}

static void csi(char c)
{
    char buf[3];
    int len = 0;
    buf[len++] = '\e';
    buf[len++] = '[';
    buf[len++] = c;
    assert(len == 3);
    child_send(buf, len);
}

static void mod_csi(ModState mod_state, char c)
{
    char buf[32];
    int len = 0;
    len = sprintf(buf, "\e[1;%c%c", mod_state.mods + '1', c);
    child_send(buf, len);
}

static void mod_ss3(ModState mod_state, char c)
{
    if (mod_state.mods)
        mod_csi(mod_state, c);
    else
        ss3(c);
}

static void tilde_code(ModState mod_state, uchar code)
{
    char buf[32];
    int len = sprintf(buf, mod_state.mods ? "\e[%i;%c~" : "\e[%i~", code, mod_state.mods + '1');
    child_send(buf, len);
}

static void cursor_key(ModState mod_state, char code, char symbol)
{
    if (mod_state.mods)
        mod_csi(mod_state, code);
    else if (term.app_cursor_keys)
        ss3(code);
    else
        csi(code);
}

static void ch(char c)
{
    child_send(&c, 1);
}

static void esc_if(bool b)
{
    if (b)
        ch('\e');
}

void ctrl_ch(ModState mod_state, uchar c)
{
    esc_if(mod_state.alt);
    if (mod_state.shift)
    {
        /*
        // Send C1 control char if the charset supports it.
        // Otherwise prefix the C0 char with ESC.
        if (c < 0x20) {
            wchar wc = c | 0x80;
            int l = cs_wcntombn(buf + len, &wc, cs_cur_max, 1);
            if (l > 0 && buf[len] != '?') {
                len += l;
                return;
            }
        };
        */
        esc_if(!mod_state.alt);
    }
    ch(c);
}

#define CDEL 0x0007f

bool ProcessKeyEvent(SDL_KeyboardEvent* key)
{
    bool down = (key->type == SDL_KEYDOWN);
    int sym = key->keysym.sym;
    int mod = key->keysym.mod;
    char ascii = SymToAscii(sym, mod);

    // init modState struct
    ModState mod_state;
    mod_state.numlock = (mod & KMOD_NUM) != 0;
    mod_state.shift = (mod & (KMOD_LSHIFT | KMOD_RSHIFT)) != 0;
    if (cfg.swap_alt_and_meta_keys) {
        mod_state.lalt = (mod & KMOD_LGUI) != 0;
        mod_state.ralt = (mod & KMOD_RGUI) != 0;
        mod_state.lmeta = (mod & KMOD_LALT) != 0;
        mod_state.rmeta = (mod & KMOD_RALT) != 0;
    } else {
        mod_state.lalt = (mod & KMOD_LALT) != 0;
        mod_state.ralt = (mod & KMOD_RALT) != 0;
        mod_state.lmeta = (mod & KMOD_LGUI) != 0;
        mod_state.rmeta = (mod & KMOD_RGUI) != 0;
    }
    mod_state.alt = mod_state.lalt || mod_state.ralt;
    mod_state.meta = mod_state.lmeta || mod_state.rmeta;
    mod_state.lctrl = (mod & KMOD_LCTRL) != 0;
    mod_state.rctrl = (mod & KMOD_RCTRL) != 0;
    mod_state.ctrl = mod_state.lctrl || mod_state.rctrl;
    mod_state.ctrl_lalt_altgr = cfg.ctrl_alt_is_altgr & mod_state.ctrl & mod_state.lalt & !mod_state.ralt,
        mod_state.altgr = mod_state.ralt | mod_state.ctrl_lalt_altgr;
    mod_state.mods = mod_state.shift * MDK_SHIFT | mod_state.alt * MDK_ALT | mod_state.ctrl * MDK_CTRL;

    // check for magic quit (meta + esc)
    if (down && sym == SDLK_ESCAPE && mod_state.meta) {
        return true;
    }

    if (down) {
        switch (sym) {
        case SDLK_RETURN:
            if (!mod_state.ctrl) {
                esc_if(mod_state.alt);
                if (term.newline_mode) {
                    ch('\r');
                    ch('\n');
                } else {
                    ch(mod_state.shift ? '\n' : '\r');
                }
            } else {
                ctrl_ch(mod_state, CTRL('^'));
            }
            break;
        case SDLK_BACKSPACE:
            if (!mod_state.ctrl) {
                esc_if(mod_state.alt);
                ch(term.backspace_sends_bs ? '\b' : CDEL);
            } else {
                ctrl_ch(mod_state, term.backspace_sends_bs ? CDEL : CTRL('_'));
            }
            break;
        case SDLK_TAB:
            if (!mod_state.alt && !mod_state.ctrl) {
                mod_state.shift ? csi('Z') : ch('\t');
            }
            break;
        case SDLK_ESCAPE:
            if (term.app_escape_key)
                ss3('[');
            else
                ctrl_ch(mod_state, term.escape_sends_fs ? CTRL('\\') : CTRL('['));
            break;
        case SDLK_PAUSE:
            ctrl_ch(mod_state, mod_state.ctrl ? CTRL('\\') : CTRL(']'));
            break;
        case SDLK_F1:
        case SDLK_F2:
        case SDLK_F3:
        case SDLK_F4:
        case SDLK_F5:
        case SDLK_F6:
        case SDLK_F7:
        case SDLK_F8:
        case SDLK_F9:
        case SDLK_F10:
        case SDLK_F11:
        case SDLK_F12:
        case SDLK_F13:
        case SDLK_F14:
        case SDLK_F15:
            // TODO:
            /*
              if (term.vt220_keys && ctrl && VK_F3 <= key && key <= VK_F10)
              key += 10, mods &= ~MDK_CTRL;
              if (key <= VK_F4)
              mod_ss3(key - VK_F1 + 'P');
              else {
              tilde_code(
              (uchar[]){
              15, 17, 18, 19, 20, 21, 23, 24, 25, 26,
              28, 29, 31, 32, 33, 34, 42, 43, 44, 45
              }[key - VK_F5]
              );
              }
            */
            break;
        case SDLK_INSERT:
            tilde_code(mod_state, 2);
            break;
        case SDLK_DELETE:
            tilde_code(mod_state, 3);
            break;
        case SDLK_HOME:
            if (term.vt220_keys)
                tilde_code(mod_state, 1);
            else
                cursor_key(mod_state, 'H', '7');
            break;
        case SDLK_END:
            if (term.vt220_keys)
                tilde_code(mod_state, 4);
            else
                cursor_key(mod_state, 'F', '1');
            break;
        case SDLK_KP_0:
        case SDLK_KP_1:
        case SDLK_KP_2:
        case SDLK_KP_3:
        case SDLK_KP_4:
        case SDLK_KP_5:
        case SDLK_KP_6:
        case SDLK_KP_7:
        case SDLK_KP_8:
        case SDLK_KP_9:
            // TODO:
            break;
        case SDLK_KP_PERIOD:
            // TODO:
            break;
        case SDLK_KP_DIVIDE:
            // TODO:
            break;
        case SDLK_KP_MULTIPLY:
            // TODO:
            break;
        case SDLK_KP_MINUS:
            // TODO:
            break;
        case SDLK_KP_PLUS:
            // TODO:
            break;
        case SDLK_KP_ENTER:
            mod_ss3(mod_state, 'M');
            break;
        case SDLK_KP_EQUALS:
            // TODO:
            break;
        case SDLK_UP:
            cursor_key(mod_state, 'A', '8');
            break;
        case SDLK_DOWN:
            cursor_key(mod_state, 'B', '2');
            break;
        case SDLK_RIGHT:
            cursor_key(mod_state, 'C', '6');
            break;
        case SDLK_LEFT:
            cursor_key(mod_state, 'D', '4');
            break;
        case SDLK_CLEAR:
            cursor_key(mod_state, 'E', '5');
            break;
        case SDLK_LSHIFT:
        case SDLK_RSHIFT:
            break;
        case SDLK_LCTRL:
        case SDLK_RCTRL:
            break;
        case SDLK_LALT:
        case SDLK_RALT:
            break;
        case SDLK_LGUI:
        case SDLK_RGUI:
            break;
        default: {
            if (mod_state.ctrl) {
                ctrl_ch(mod_state, CTRL(ascii));
            } else {
                esc_if(mod_state.alt);
                ch(ascii);
            }
            break;
        }
        }
    }
    return false; // don't quit
}
