#ifndef CONFIG_H
#define CONFIG_H

#include "std.h"

// Enums for various options.

typedef enum { MDK_SHIFT = 1, MDK_ALT = 2, MDK_CTRL = 4 } mod_keys;
enum { HOLD_NEVER, HOLD_START, HOLD_ERROR, HOLD_ALWAYS };
enum { CUR_BLOCK, CUR_UNDERSCORE, CUR_LINE };
enum { FS_DEFAULT, FS_PARTIAL, FS_NONE, FS_FULL };
enum { RC_MENU, RC_PASTE, RC_EXTEND };
enum { TR_OFF = 0, TR_LOW = 16, TR_MEDIUM = 32, TR_HIGH = 48, TR_GLASS = -1 };


// Colour values.

typedef uint colour;

enum { DEFAULT_COLOUR = UINT_MAX };

static inline colour
make_colour(uchar r, uchar g, uchar b) { return r | g << 8 | b << 16; }

bool parse_colour(mintty_string, colour *);

static inline uchar red(colour c) { return c; }
static inline uchar green(colour c) { return c >> 8; }
static inline uchar blue(colour c) { return c >> 16; }


// Font properties.

typedef struct {
  mintty_string name;
  int size;
  bool isbold;
} font_spec;


// Configuration data.

typedef struct {
  // Looks
  colour fg_colour, bg_colour, cursor_colour;
  char transparency;
  bool opaque_when_focused;
  char cursor_type;
  bool cursor_blinks;
  // Text
  font_spec font;
  char font_smoothing;
  char bold_as_font;    // 0 = false, 1 = true, -1 = undefined
  bool bold_as_colour;
  bool allow_blinking;
  mintty_string locale;
  mintty_string charset;
  // Keys
  bool backspace_sends_bs;
  bool ctrl_alt_is_altgr;
  bool clip_shortcuts;
  bool window_shortcuts;
  bool switch_shortcuts;
  bool zoom_shortcuts;
  bool alt_fn_shortcuts;
  bool ctrl_shift_shortcuts;
  bool swap_alt_and_meta_keys;
  // Mouse
  bool copy_on_select;
  bool copy_as_rtf;
  bool clicks_place_cursor;
  char right_click_action;
  bool clicks_target_app;
  char click_target_mod;
  // Window
  int cols, rows;
  int scrollback_lines;
  char scrollbar;
  char scroll_mod;
  bool pgupdn_scroll;
  // Terminal
  mintty_string term;
  mintty_string answerback;
  bool bell_sound;
  bool bell_flash;
  bool bell_taskbar;
  mintty_string printer;
  bool confirm_exit;
  // Command line
  mintty_string klass;
  char hold;
  mintty_string icon;
  mintty_string log;
  mintty_string title;
  bool utmp;
  char window;
  int x, y;
  // "Hidden"
  int col_spacing, row_spacing;
  mintty_string word_chars;
  colour ime_cursor_colour;
  colour ansi_colours[16];
  // Legacy
  bool use_system_colours;
} config;

extern config cfg, new_cfg;

void init_config(void);
void load_config(mintty_string filename);
void set_arg_option(mintty_string name, mintty_string val);
void parse_arg_option(mintty_string);
void remember_arg(mintty_string);
void finish_config(void);
void copy_config(config *dst, const config *src);

#endif
