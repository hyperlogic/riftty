// config.c (part of mintty)
// Copyright 2008-12 Andy Koppe
// Based on code from PuTTY-0.60 by Simon Tatham and team.
// Licensed under the terms of the GNU General Public License v3 or later.

#include <stddef.h>
#include "term.h"
//#include "ctrls.h"
//#include "print.h"
#include "charset.h"
#include "win.h"

#include <termios.h>
//#include <sys/cygwin.h>

#define MIN(a, b)  (((a) < (b)) ? (a) : (b))
#define MAX(a, b)  (((a) > (b)) ? (a) : (b))

config cfg, new_cfg;
static config file_cfg;

const config default_cfg = {
  // Looks
  .fg_colour = 0xBFBFBF,
  .bg_colour = 0x000000,
  .cursor_colour = 0xBFBFBF,
  .transparency = 0,
  .opaque_when_focused = false,
  .cursor_type = CUR_LINE,
  .cursor_blinks = true,
  // Text
  .font = {.name = NULL, .isbold = false, .size = 9},
  .font_smoothing = FS_DEFAULT,
  .bold_as_font = -1,  // -1 means "the opposite of bold_as_colour"
  .bold_as_colour = true,
  .allow_blinking = false,
  .locale = NULL,
  .charset = NULL,
  // Keys
  .backspace_sends_bs = CERASE == '\b',
  .ctrl_alt_is_altgr = false,
  .clip_shortcuts = true,
  .window_shortcuts = true,
  .switch_shortcuts = true,
  .zoom_shortcuts = true,
  .alt_fn_shortcuts = true,
  .ctrl_shift_shortcuts = false,
  .swap_alt_and_meta_keys = false,
  // Mouse
  .copy_on_select = true,
  .copy_as_rtf = true,
  .clicks_place_cursor = false,
  .right_click_action = RC_MENU,
  .clicks_target_app = true,
  .click_target_mod = MDK_SHIFT,
  // Window
  .win_pos = {0.2f, 1.5f, -1.2f},
  .win_rot = {0.0f, 0.0f, 0.0f},
  .win_anchor = {0.5f, 0.5f},
  .win_width = 2.0f,
  .win_fullscreen = true,
  .cols = 80,
  .rows = 24,
  .scrollbar = 1,
  .scrollback_lines = 10000,
  .scroll_mod = MDK_SHIFT,
  .pgupdn_scroll = false,
  // Terminal
  .term = NULL,
  .answerback = NULL,
  .bell_sound = false,
  .bell_flash = false,
  .bell_taskbar = true,
  .printer = NULL,
  .confirm_exit = true,
  // Command line
  .klass = NULL,
  .hold = HOLD_START,
  .icon = NULL,
  .log = NULL,
  .utmp = false,
  .title = NULL,
  // "Hidden"
  .col_spacing = 0,
  .row_spacing = 0,
  .word_chars = NULL,
  .use_system_colours = false,
  .ime_cursor_colour = DEFAULT_COLOUR,
  .ansi_colours = {
    [BLACK_I]        = 0x000000,
    [RED_I]          = 0x0000BF,
    [GREEN_I]        = 0x00BF00,
    [YELLOW_I]       = 0x00BFBF,
    [BLUE_I]         = 0xBF0000,
    [MAGENTA_I]      = 0xBF00BF,
    [CYAN_I]         = 0xBFBF00,
    [WHITE_I]        = 0xBFBFBF,
    [BOLD_BLACK_I]   = 0x404040,
    [BOLD_RED_I]     = 0x4040FF,
    [BOLD_GREEN_I]   = 0x40FF40,
    [BOLD_YELLOW_I]  = 0x40FFFF,
    [BOLD_BLUE_I]    = 0xFF6060,
    [BOLD_MAGENTA_I] = 0xFF40FF,
    [BOLD_CYAN_I]    = 0xFFFF40,
    [BOLD_WHITE_I]   = 0xFFFFFF,
  }
};

typedef enum {
  OPT_BOOL, OPT_MOD, OPT_TRANS, OPT_CURSOR, OPT_FONTSMOOTH,
  OPT_RIGHTCLICK, OPT_SCROLLBAR, OPT_WINDOW, OPT_HOLD,
  OPT_INT, OPT_COLOUR, OPT_STRING, OPT_FLOAT,
  OPT_LEGACY = 16
} opt_type;

#define offcfg(option) offsetof(config, option)


#if 0 //CYGWIN_VERSION_API_MINOR >= 222
static mintty_wstring rc_filename = 0;
#else
static mintty_string rc_filename = 0;
#endif

static const struct {
  mintty_string name;
  uchar type;
  uint offset;
}

options[] = {
  // Looks
  {"ForegroundColour", OPT_COLOUR, offcfg(fg_colour)},
  {"BackgroundColour", OPT_COLOUR, offcfg(bg_colour)},
  {"CursorColour", OPT_COLOUR, offcfg(cursor_colour)},
  {"Transparency", OPT_TRANS, offcfg(transparency)},
  {"OpaqueWhenFocused", OPT_BOOL, offcfg(opaque_when_focused)},
  {"CursorType", OPT_CURSOR, offcfg(cursor_type)},
  {"CursorBlinks", OPT_BOOL, offcfg(cursor_blinks)},

  // Text
  {"Font", OPT_STRING, offcfg(font.name)},
  {"FontIsBold", OPT_BOOL, offcfg(font.isbold)},
  {"FontSize", OPT_INT, offcfg(font.size)},
  {"FontSmoothing", OPT_FONTSMOOTH, offcfg(font_smoothing)},
  {"BoldAsFont", OPT_BOOL, offcfg(bold_as_font)},
  {"BoldAsColour", OPT_BOOL, offcfg(bold_as_colour)},
  {"AllowBlinking", OPT_BOOL, offcfg(allow_blinking)},
  {"Locale", OPT_STRING, offcfg(locale)},
  {"Charset", OPT_STRING, offcfg(charset)},

  // Keys
  {"BackspaceSendsBS", OPT_BOOL, offcfg(backspace_sends_bs)},
  {"CtrlAltIsAltGr", OPT_BOOL, offcfg(ctrl_alt_is_altgr)},
  {"ClipShortcuts", OPT_BOOL, offcfg(clip_shortcuts)},
  {"WindowShortcuts", OPT_BOOL, offcfg(window_shortcuts)},
  {"SwitchShortcuts", OPT_BOOL, offcfg(switch_shortcuts)},
  {"ZoomShortcuts", OPT_BOOL, offcfg(zoom_shortcuts)},
  {"AltFnShortcuts", OPT_BOOL, offcfg(alt_fn_shortcuts)},
  {"CtrlShiftShortcuts", OPT_BOOL, offcfg(ctrl_shift_shortcuts)},
  {"SwapAltAndMetaKeys", OPT_BOOL, offcfg(swap_alt_and_meta_keys)},

  // Mouse
  {"CopyOnSelect", OPT_BOOL, offcfg(copy_on_select)},
  {"CopyAsRTF", OPT_BOOL, offcfg(copy_as_rtf)},
  {"ClicksPlaceCursor", OPT_BOOL, offcfg(clicks_place_cursor)},
  {"RightClickAction", OPT_RIGHTCLICK, offcfg(right_click_action)},
  {"ClicksTargetApp", OPT_BOOL, offcfg(clicks_target_app)},
  {"ClickTargetMod", OPT_MOD, offcfg(click_target_mod)},

  // Window
  {"WinPosX", OPT_FLOAT, offcfg(win_pos.x)},
  {"WinPosY", OPT_FLOAT, offcfg(win_pos.y)},
  {"WinPosZ", OPT_FLOAT, offcfg(win_pos.z)},
  {"WinRotX", OPT_FLOAT, offcfg(win_rot.x)},
  {"WinRotY", OPT_FLOAT, offcfg(win_rot.y)},
  {"WinRotZ", OPT_FLOAT, offcfg(win_rot.z)},
  {"WinAnchorX", OPT_FLOAT, offcfg(win_anchor.x)},
  {"WinAnchorY", OPT_FLOAT, offcfg(win_anchor.y)},
  {"WinWidth", OPT_FLOAT, offcfg(win_width)},
  {"WinFullscreen", OPT_BOOL, offcfg(win_fullscreen)},

  {"Columns", OPT_INT, offcfg(cols)},
  {"Rows", OPT_INT, offcfg(rows)},
  {"ScrollbackLines", OPT_INT, offcfg(scrollback_lines)},
  {"Scrollbar", OPT_SCROLLBAR, offcfg(scrollbar)},
  {"ScrollMod", OPT_MOD, offcfg(scroll_mod)},
  {"PgUpDnScroll", OPT_BOOL, offcfg(pgupdn_scroll)},

  // Terminal
  {"Term", OPT_STRING, offcfg(term)},
  {"Answerback", OPT_STRING, offcfg(answerback)},
  {"BellSound", OPT_BOOL, offcfg(bell_sound)},
  {"BellFlash", OPT_BOOL, offcfg(bell_flash)},
  {"BellTaskbar", OPT_BOOL, offcfg(bell_taskbar)},
  {"Printer", OPT_STRING, offcfg(printer)},
  {"ConfirmExit", OPT_BOOL, offcfg(confirm_exit)},

  // Command line
  {"Class", OPT_STRING, offcfg(klass)},
  {"Hold", OPT_HOLD, offcfg(hold)},
  {"Icon", OPT_STRING, offcfg(icon)},
  {"Log", OPT_STRING, offcfg(log)},
  {"Title", OPT_STRING, offcfg(title)},
  {"Utmp", OPT_BOOL, offcfg(utmp)},
  {"Window", OPT_WINDOW, offcfg(window)},
  {"X", OPT_INT, offcfg(x)},
  {"Y", OPT_INT, offcfg(y)},

  // "Hidden"
  {"ColSpacing", OPT_INT, offcfg(col_spacing)},
  {"RowSpacing", OPT_INT, offcfg(row_spacing)},
  {"WordChars", OPT_STRING, offcfg(word_chars)},
  {"IMECursorColour", OPT_COLOUR, offcfg(ime_cursor_colour)},

  // ANSI colours
  {"Black", OPT_COLOUR, offcfg(ansi_colours[BLACK_I])},
  {"Red", OPT_COLOUR, offcfg(ansi_colours[RED_I])},
  {"Green", OPT_COLOUR, offcfg(ansi_colours[GREEN_I])},
  {"Yellow", OPT_COLOUR, offcfg(ansi_colours[YELLOW_I])},
  {"Blue", OPT_COLOUR, offcfg(ansi_colours[BLUE_I])},
  {"Magenta", OPT_COLOUR, offcfg(ansi_colours[MAGENTA_I])},
  {"Cyan", OPT_COLOUR, offcfg(ansi_colours[CYAN_I])},
  {"White", OPT_COLOUR, offcfg(ansi_colours[WHITE_I])},
  {"BoldBlack", OPT_COLOUR, offcfg(ansi_colours[BOLD_BLACK_I])},
  {"BoldRed", OPT_COLOUR, offcfg(ansi_colours[BOLD_RED_I])},
  {"BoldGreen", OPT_COLOUR, offcfg(ansi_colours[BOLD_GREEN_I])},
  {"BoldYellow", OPT_COLOUR, offcfg(ansi_colours[BOLD_YELLOW_I])},
  {"BoldBlue", OPT_COLOUR, offcfg(ansi_colours[BOLD_BLUE_I])},
  {"BoldMagenta", OPT_COLOUR, offcfg(ansi_colours[BOLD_MAGENTA_I])},
  {"BoldCyan", OPT_COLOUR, offcfg(ansi_colours[BOLD_CYAN_I])},
  {"BoldWhite", OPT_COLOUR, offcfg(ansi_colours[BOLD_WHITE_I])},

  // Legacy
  {"UseSystemColours", OPT_BOOL | OPT_LEGACY, offcfg(use_system_colours)},
  {"BoldAsBright", OPT_BOOL | OPT_LEGACY, offcfg(bold_as_colour)},
  {"FontQuality", OPT_FONTSMOOTH | OPT_LEGACY, offcfg(font_smoothing)},
};

typedef const struct {
  mintty_string name;
  char val;
} opt_val;

static opt_val
*const opt_vals[] = {
  [OPT_BOOL] = (opt_val[]) {
    {"no", false},
    {"yes", true},
    {"false", false},
    {"true", true},
    {0, 0}
  },
  [OPT_MOD] = (opt_val[]) {
    {"off", 0},
    {"shift", MDK_SHIFT},
    {"alt", MDK_ALT},
    {"ctrl", MDK_CTRL},
    {0, 0}
  },
  [OPT_TRANS] = (opt_val[]) {
    {"off", TR_OFF},
    {"low", TR_LOW},
    {"medium", TR_MEDIUM},
    {"high", TR_HIGH},
    {"glass", TR_GLASS},
    {0, 0}
  },
  [OPT_CURSOR] = (opt_val[]) {
    {"line", CUR_LINE},
    {"block", CUR_BLOCK},
    {"underscore", CUR_UNDERSCORE},
    {0, 0}
  },
  [OPT_FONTSMOOTH] = (opt_val[]) {
    {"default", FS_DEFAULT},
    {"none", FS_NONE},
    {"partial", FS_PARTIAL},
    {"full", FS_FULL},
    {0, 0}
  },
  [OPT_RIGHTCLICK] = (opt_val[]) {
    {"paste", RC_PASTE},
    {"extend", RC_EXTEND},
    {"menu", RC_MENU},
    {0, 0}
  },
  [OPT_SCROLLBAR] = (opt_val[]) {
    {"left", -1},
    {"right", 1},
    {"none", 0},
    {0, 0}
  },
  [OPT_WINDOW] = (opt_val[]){
    {"hide", 0},   // SW_HIDE
    {"normal", 1}, // SW_SHOWNORMAL
    {"min", 2},    // SW_SHOWMINIMIZED
    {"max", 3},    // SW_SHOWMAXIMIZED
    {"full", -1},
    {0, 0}
  },
  [OPT_HOLD] = (opt_val[]) {
    {"never", HOLD_NEVER},
    {"start", HOLD_START},
    {"error", HOLD_ERROR},
    {"always", HOLD_ALWAYS},
    {0, 0}
  }
};

static int
find_option(mintty_string name)
{
  uint i;
  for (i = 0; i < lengthof(options); i++) {
    if (!strcasecmp(name, options[i].name))
      return i;
  }
  fprintf(stderr, "Ignoring unknown option '%s'.\n", name);
  return -1;
}

static uchar file_opts[lengthof(options)], arg_opts[lengthof(options)];
static uint file_opts_num, arg_opts_num;

static bool
seen_file_option(uint i)
{
  return memchr(file_opts, i, file_opts_num);
}

static void
remember_file_option(uint i)
{
  if (!seen_file_option(i))
    file_opts[file_opts_num++] = i;
}

static bool
seen_arg_option(uint i)
{
  return memchr(arg_opts, i, arg_opts_num);
}

static void
remember_arg_option(uint i)
{
  if (!seen_arg_option(i))
    arg_opts[arg_opts_num++] = i;
}

void
remember_arg(mintty_string option)
{
  remember_arg_option(find_option(option));
}

static void
check_legacy_options(void (*remember_option)(uint))
{
  if (cfg.use_system_colours) {
    // Translate 'UseSystemColours' to colour settings.
    cfg.fg_colour = cfg.cursor_colour = win_get_sys_colour(true);
    cfg.bg_colour = win_get_sys_colour(false);
    cfg.use_system_colours = false;

    // Make sure they're written to the config file.
    // This assumes that the colour options are the first three in options[].
    remember_option(0);
    remember_option(1);
    remember_option(2);
  }
}

bool
parse_colour(mintty_string s, colour *cp)
{
  uint r, g, b;
  if (sscanf(s, "%u,%u,%u%c", &r, &g, &b, &(char){0}) == 3);
  else if (sscanf(s, "#%2x%2x%2x%c", &r, &g, &b, &(char){0}) == 3);
  else if (sscanf(s, "rgb:%2x/%2x/%2x%c", &r, &g, &b, &(char){0}) == 3);
  else if (sscanf(s, "rgb:%4x/%4x/%4x%c", &r, &g, &b, &(char){0}) == 3)
    r >>=8, g >>= 8, b >>= 8;
  else
    return false;

  *cp = make_colour(r, g, b);
  return true;
}

static int
set_option(mintty_string name, mintty_string val_str)
{
  int i = find_option(name);
  if (i < 0)
    return i;

  void *val_p = (void *)&cfg + options[i].offset;
  uint type = options[i].type & ~OPT_LEGACY;

  switch (type) {
    WHEN OPT_STRING:
      strset(val_p, val_str);
      return i;
    WHEN OPT_INT: {
      char *val_end;
      int val = strtol(val_str, &val_end, 0);
      if (val_end != val_str) {
        *(int *)val_p = val;
        return i;
      }
    }
    WHEN OPT_FLOAT: {
      char *val_end;
      double val = strtod(val_str, &val_end);
      if (val_end != val_str) {
        *(float *)val_p = (float)val;
        return i;
      }
    }
    WHEN OPT_COLOUR:
      if (parse_colour(val_str, val_p))
        return i;
    OTHERWISE: {
      int len = strlen(val_str);
      if (!len)
        break;
      opt_val *o;
      for (o = opt_vals[type]; o->name; o++) {
        if (!strncasecmp(val_str, o->name, len)) {
          *(char *)val_p = o->val;
          return i;
        }
      }
      // Value not found: try interpreting it as a number.
      char *val_end;
      int val = strtol(val_str, &val_end, 0);
      if (val_end != val_str) {
        *(char *)val_p = val;
        return i;
      }
    }
  }
  fprintf(stderr, "Ignoring invalid value '%s' for option '%s'.\n",
                  val_str, name);
  return -1;
}

static int
parse_option(mintty_string option)
{
  const char *eq = strchr(option, '=');
  if (!eq) {
    fprintf(stderr, "Ignoring malformed option '%s'.\n", option);
    return -1;
  }

  const char *name_end = eq;
  while (isspace((uchar)name_end[-1]))
    name_end--;

  uint name_len = name_end - option;
  char name[name_len + 1];
  memcpy(name, option, name_len);
  name[name_len] = 0;

  const char *val = eq + 1;
  while (isspace((uchar)*val))
    val++;

  return set_option(name, val);
}

static void
check_arg_option(int i)
{
  if (i >= 0) {
    remember_arg_option(i);
    check_legacy_options(remember_arg_option);
  }
}

void
set_arg_option(mintty_string name, mintty_string val)
{
  check_arg_option(set_option(name, val));
}

void
parse_arg_option(mintty_string option)
{
  check_arg_option(parse_option(option));
}

void
load_config(mintty_string filename)
{
  file_opts_num = arg_opts_num = 0;

  mintty_delete(rc_filename);
#if CYGWIN_VERSION_API_MINOR >= 222
  rc_filename = cygwin_create_path(CCP_POSIX_TO_WIN_W, filename);
#else
  rc_filename = strdup(filename);
#endif

  FILE *file = fopen(filename, "r");
  if (file) {
    char line[256];
    while (fgets(line, sizeof line, file)) {
      line[strcspn(line, "\r\n")] = 0;  /* trim newline */
      int i = parse_option(line);
      if (i >= 0)
        remember_file_option(i);
    }
    fclose(file);
  }

  check_legacy_options(remember_file_option);

  copy_config(&file_cfg, &cfg);
}

void
copy_config(config *dst_p, const config *src_p)
{
  uint i;
  char* temp;
  for (i = 0; i < lengthof(options); i++) {
    opt_type type = options[i].type;
    if (!(type & OPT_LEGACY)) {
      uint offset = options[i].offset;
      void *dst_val_p = (void *)dst_p + offset;
      void *src_val_p = (void *)src_p + offset;
      switch (type) {
        WHEN OPT_STRING:
          strset(dst_val_p, *(mintty_string *)src_val_p);
        WHEN OPT_INT OR OPT_COLOUR:
          *(int *)dst_val_p = *(int *)src_val_p;
        WHEN OPT_FLOAT:
          *(float *)dst_val_p = *(float *)src_val_p;
        OTHERWISE:
          *(char *)dst_val_p = *(char *)src_val_p;
      }
    }
  }
}

void
init_config(void)
{
  copy_config(&cfg, &default_cfg);
  strset(&cfg.term, "xterm");
  strset(&cfg.title, "riftty");
  strset(&cfg.font.name, "font/DejaVuSansMono-Bold.ttf");
  cfg.font.size = 32;
}

void
finish_config(void)
{
  // Avoid negative sizes.
  cfg.rows = MAX(1, cfg.rows);
  cfg.cols = MAX(1, cfg.cols);
  cfg.scrollback_lines = MAX(0, cfg.scrollback_lines);

  // Ignore charset setting if we haven't got a locale.
  if (!cfg.locale || !*cfg.locale)
    strset(&cfg.charset, "");

  // bold_as_font used to be implied by !bold_as_colour.
  if (cfg.bold_as_font == -1) {
    cfg.bold_as_font = !cfg.bold_as_colour;
    remember_file_option(find_option("BoldAsFont"));
  }

  if (0 < cfg.transparency && cfg.transparency <= 3)
    cfg.transparency *= 16;
}

static void
save_config(void)
{
  mintty_string filename;

#if CYGWIN_VERSION_API_MINOR >= 222
  filename = cygwin_create_path(CCP_WIN_W_TO_POSIX, rc_filename);
#else
  filename = rc_filename;
#endif

  FILE *file = fopen(filename, "w");

  if (!file) {
    char *msg;
    int len = asprintf(&msg, "Could not save options to '%s':\n%s.",
                       filename, strerror(errno));
    if (len > 0) {
      wchar wmsg[len + 1];
      if (cs_mbstowcs(wmsg, msg, lengthof(wmsg)) >= 0)
        win_show_error(wmsg);
      mintty_delete(msg);
    }
  }
  else {
    uint j;
    for (j = 0; j < file_opts_num; j++) {
      uint i = file_opts[j];
      opt_type type = options[i].type;
      if (!(type & OPT_LEGACY)) {
        fprintf(file, "%s=", options[i].name);
        void *cfg_p = seen_arg_option(i) ? &file_cfg : &cfg;
        void *val_p = cfg_p + options[i].offset;
        switch (type) {
          WHEN OPT_STRING:
            fprintf(file, "%s", *(mintty_string *)val_p);
          WHEN OPT_INT:
            fprintf(file, "%i", *(int *)val_p);
          WHEN OPT_FLOAT:
            fprintf(file, "%f", *(float *)val_p);
          WHEN OPT_COLOUR: {
            colour c = *(colour *)val_p;
            fprintf(file, "%u,%u,%u", red(c), green(c), blue(c));
          }
          OTHERWISE: {
            int val = *(char *)val_p;
            opt_val *o = opt_vals[type];
            for (; o->name; o++) {
              if (o->val == val)
                break;
            }
            if (o->name)
              fputs(o->name, file);
            else
              fprintf(file, "%i", val);
          }
        }
        fputc('\n', file);
      }
    }
    fclose(file);
  }

#if CYGWIN_VERSION_API_MINOR >= 222
  mintty_delete(filename);
#endif
}

#ifdef MINTTY_WINDOWS_DIALOG_BOXES
 static control *cols_box, *rows_box, *locale_box, *charset_box;
#endif

static void
apply_config(void)
{
  // Record what's changed
  uint i;
  for (i = 0; i < lengthof(options); i++) {
    opt_type type = options[i].type;
    uint offset = options[i].offset;
    void *val_p = (void *)&cfg + offset;
    void *new_val_p = (void *)&new_cfg + offset;
    bool changed;
    switch (type) {
      WHEN OPT_STRING:
        changed = strcmp(*(mintty_string *)val_p, *(mintty_string *)new_val_p);
      WHEN OPT_INT OR OPT_COLOUR:
        changed = (*(int *)val_p != *(int *)new_val_p);
      WHEN OPT_FLOAT:
        changed = (*(float *)val_p != *(float *)new_val_p);
      OTHERWISE:
        changed = (*(char *)val_p != *(char *)new_val_p);
    }
    if (changed && !seen_arg_option(i))
      remember_file_option(i);
  }

  win_reconfig();
  save_config();
}

#ifdef MINTTY_WINDOWS_DIALOG_BOXES
static void
ok_handler(control *unused(ctrl), int event)
{
  if (event == EVENT_ACTION) {
    apply_config();
    dlg_end();
  }
}

static void
cancel_handler(control *unused(ctrl), int event)
{
  if (event == EVENT_ACTION)
    dlg_end();
}

static void
apply_handler(control *unused(ctrl), int event)
{
  if (event == EVENT_ACTION)
    apply_config();
}

static void
about_handler(control *unused(ctrl), int event)
{
  if (event == EVENT_ACTION)
    win_show_about();
}

static void
current_size_handler(control *unused(ctrl), int event)
{
  if (event == EVENT_ACTION) {
    new_cfg.cols = term.cols;
    new_cfg.rows = term.rows;
    dlg_refresh(cols_box);
    dlg_refresh(rows_box);
  }
}

static void
printerbox_handler(control *ctrl, int event)
{
  const string NONE = "None (printing disabled)";
  string printer = new_cfg.printer;
  if (event == EVENT_REFRESH) {
    dlg_listbox_clear(ctrl);
    dlg_listbox_add(ctrl, NONE);
    uint num = printer_start_enum();
    for (uint i = 0; i < num; i++)
      dlg_listbox_add(ctrl, printer_get_name(i));
    printer_finish_enum();
    dlg_editbox_set(ctrl, *printer ? printer : NONE);
  }
  else if (event == EVENT_VALCHANGE || event == EVENT_SELCHANGE) {
    dlg_editbox_get(ctrl, &printer);
    if (!strcmp(printer, NONE))
      strset(&printer, "");
    new_cfg.printer = printer;
  }
}
#endif // #ifdef MINTTY_WINDOWS_DIALOG_BOXES

static void
set_charset(mintty_string charset)
{
  strset(&new_cfg.charset, charset);

#ifdef MINTTY_WINDOWS_DIALOG_BOXES
  dlg_editbox_set(charset_box, charset);
#else
  assert(0);
#endif

}

#ifdef MINTTY_WINDOWS_DIALOG_BOXES
static void
locale_handler(control *ctrl, int event)
{
  string locale = new_cfg.locale;
  switch (event) {
    WHEN EVENT_REFRESH:
      dlg_listbox_clear(ctrl);
      string l;
      for (int i = 0; (l = locale_menu[i]); i++)
        dlg_listbox_add(ctrl, l);
      dlg_editbox_set(ctrl, locale);
    WHEN EVENT_UNFOCUS:
      dlg_editbox_set(ctrl, locale);
      if (!*locale)
        set_charset("");
    WHEN EVENT_VALCHANGE:
      dlg_editbox_get(ctrl, &new_cfg.locale);
    WHEN EVENT_SELCHANGE:
      dlg_editbox_get(ctrl, &new_cfg.locale);
      if (*locale == '(')
        strset(&locale, "");
      if (!*locale)
        set_charset("");
#if HAS_LOCALES
      else if (!*new_cfg.charset)
        set_charset("UTF-8");
#endif
      new_cfg.locale = locale;
  }
}
#endif // #ifdef MINTTY_WINDOWS_DIALOG_BOXES

static void
check_locale(void)
{
  if (!*new_cfg.locale) {
    strset(&new_cfg.locale, "C");
#ifdef MINTTY_WINDOWS_DIALOG_BOXES
    dlg_editbox_set(locale_box, "C");
#else
    assert(0);
#endif
  }
}

#ifdef MINTTY_WINDOWS_DIALOG_BOXES
static void
charset_handler(control *ctrl, int event)
{
  string charset = new_cfg.charset;
  switch (event) {
    WHEN EVENT_REFRESH:
      dlg_listbox_clear(ctrl);
      string cs;
      for (int i = 0; (cs = charset_menu[i]); i++)
        dlg_listbox_add(ctrl, cs);
      dlg_editbox_set(ctrl, charset);
    WHEN EVENT_UNFOCUS:
      dlg_editbox_set(ctrl, charset);
      if (*charset)
        check_locale();
    WHEN EVENT_VALCHANGE:
      dlg_editbox_get(ctrl, &new_cfg.charset);
    WHEN EVENT_SELCHANGE:
      dlg_editbox_get(ctrl, &charset);
      if (*charset == '(')
        strset(&charset, "");
      else {
        *strchr(charset, ' ') = 0;
        check_locale();
      }
      new_cfg.charset = charset;
  }
}
#endif // #ifdef MINTTY_WINDOWS_DIALOG_BOXES

#ifdef MINTTY_WINDOWS_DIALOG_BOXES
static void
term_handler(control *ctrl, int event)
{
  switch (event) {
    WHEN EVENT_REFRESH:
      dlg_listbox_clear(ctrl);
      dlg_listbox_add(ctrl, "xterm");
      dlg_listbox_add(ctrl, "xterm-256color");
      dlg_listbox_add(ctrl, "xterm-vt220");
      dlg_listbox_add(ctrl, "vt100");
      dlg_listbox_add(ctrl, "vt220");
      dlg_editbox_set(ctrl, new_cfg.term);
    WHEN EVENT_VALCHANGE or EVENT_SELCHANGE:
      dlg_editbox_get(ctrl, &new_cfg.term);
  }
}
#endif // #ifdef MINTTY_WINDOWS_DIALOG_BOXES

#ifdef MINTTY_WINDOWS_DIALOG_BOXES
void
setup_config_box(controlbox * b)
{
  controlset *s;
  control *c;

 /*
  * The standard panel that appears at the bottom of all panels:
  * Open, Cancel, Apply etc.
  */
  s = ctrl_new_set(b, "", "");
  ctrl_columns(s, 5, 20, 20, 20, 20, 20);
  c = ctrl_pushbutton(s, "About...", about_handler, 0);
  c->column = 0;
  c = ctrl_pushbutton(s, "OK", ok_handler, 0);
  c->button.isdefault = true;
  c->column = 2;
  c = ctrl_pushbutton(s, "Cancel", cancel_handler, 0);
  c->button.iscancel = true;
  c->column = 3;
  c = ctrl_pushbutton(s, "Apply", apply_handler, 0);
  c->column = 4;

 /*
  * The Looks panel.
  */
  s = ctrl_new_set(b, "Looks", "Colours");
  ctrl_columns(s, 3, 33, 33, 33);
  ctrl_pushbutton(
    s, "&Foreground...", dlg_stdcolour_handler, &new_cfg.fg_colour
  )->column = 0;
  ctrl_pushbutton(
    s, "&Background...", dlg_stdcolour_handler, &new_cfg.bg_colour
  )->column = 1;
  ctrl_pushbutton(
    s, "&Cursor...", dlg_stdcolour_handler, &new_cfg.cursor_colour
  )->column = 2;

  s = ctrl_new_set(b, "Looks", "Transparency");
  bool with_glass = win_is_glass_available();
  ctrl_radiobuttons(
    s, null, 4 + with_glass,
    dlg_stdradiobutton_handler, &new_cfg.transparency,
    "&Off", TR_OFF,
    "&Low", TR_LOW,
    with_glass ? "&Med." : "&Medium", TR_MEDIUM,
    "&High", TR_HIGH,
    with_glass ? "Gla&ss" : null, TR_GLASS,
    null
  );
  ctrl_checkbox(
    s, "Opa&que when focused",
    dlg_stdcheckbox_handler, &new_cfg.opaque_when_focused
  );

  s = ctrl_new_set(b, "Looks", "Cursor");
  ctrl_radiobuttons(
    s, null, 4 + with_glass,
    dlg_stdradiobutton_handler, &new_cfg.cursor_type,
    "Li&ne", CUR_LINE,
    "Bloc&k", CUR_BLOCK,
    "&Underscore", CUR_UNDERSCORE,
    null
  );
  ctrl_checkbox(
    s, "Blinkin&g", dlg_stdcheckbox_handler, &new_cfg.cursor_blinks
  );

 /*
  * The Text panel.
  */
  s = ctrl_new_set(b, "Text", "Font");
  ctrl_fontsel(
    s, null, dlg_stdfontsel_handler, &new_cfg.font
  );

  s = ctrl_new_set(b, "Text", null);
  ctrl_columns(s, 2, 50, 50);
  ctrl_radiobuttons(
    s, "Font smoothing", 2,
    dlg_stdradiobutton_handler, &new_cfg.font_smoothing,
    "&Default", FS_DEFAULT,
    "&None", FS_NONE,
    "&Partial", FS_PARTIAL,
    "&Full", FS_FULL,
    null
  )->column = 1;

  ctrl_checkbox(
    s, "Sho&w bold as font",
    dlg_stdcheckbox_handler, &new_cfg.bold_as_font
  )->column = 0;
  ctrl_checkbox(
    s, "Show &bold as colour",
    dlg_stdcheckbox_handler, &new_cfg.bold_as_colour
  )->column = 0;
  ctrl_checkbox(
    s, "&Allow blinking",
    dlg_stdcheckbox_handler, &new_cfg.allow_blinking
  )->column = 0;

  s = ctrl_new_set(b, "Text", null);
  ctrl_columns(s, 2, 29, 71);
  (locale_box = ctrl_combobox(
    s, "&Locale", 100, locale_handler, 0
  ))->column = 0;
  (charset_box = ctrl_combobox(
    s, "&Character set", 100, charset_handler, 0
  ))->column = 1;

 /*
  * The Keys panel.
  */
  s = ctrl_new_set(b, "Keys", null);
  ctrl_checkbox(
    s, "&Backspace sends ^H",
    dlg_stdcheckbox_handler, &new_cfg.backspace_sends_bs
  );
  ctrl_checkbox(
    s, "Ctrl+LeftAlt is Alt&Gr",
    dlg_stdcheckbox_handler, &new_cfg.ctrl_alt_is_altgr
  );

  s = ctrl_new_set(b, "Keys", "Shortcuts");
  ctrl_checkbox(
    s, "Cop&y and Paste (Ctrl/Shift+Ins)",
    dlg_stdcheckbox_handler, &new_cfg.clip_shortcuts
  );
  ctrl_checkbox(
    s, "&Menu and Full Screen (Alt+Space/Enter)",
    dlg_stdcheckbox_handler, &new_cfg.window_shortcuts
  );
  ctrl_checkbox(
    s, "&Switch window (Ctrl+[Shift+]Tab)",
    dlg_stdcheckbox_handler, &new_cfg.switch_shortcuts
  );
  ctrl_checkbox(
    s, "&Zoom (Ctrl+plus/minus/zero)",
    dlg_stdcheckbox_handler, &new_cfg.zoom_shortcuts
  );
  ctrl_checkbox(
    s, "&Alt+Fn shortcuts",
    dlg_stdcheckbox_handler, &new_cfg.alt_fn_shortcuts
  );
  ctrl_checkbox(
    s, "&Ctrl+Shift+letter shortcuts",
    dlg_stdcheckbox_handler, &new_cfg.ctrl_shift_shortcuts
  );

 /*
  * The Mouse panel.
  */
  s = ctrl_new_set(b, "Mouse", null);
  ctrl_columns(s, 2, 50, 50);
  ctrl_checkbox(
    s, "Cop&y on select",
    dlg_stdcheckbox_handler, &new_cfg.copy_on_select
  )->column = 0;
  ctrl_checkbox(
    s, "Copy as &rich text",
    dlg_stdcheckbox_handler, &new_cfg.copy_as_rtf
  )->column = 1;
  ctrl_checkbox(
    s, "Clic&ks place command line cursor",
    dlg_stdcheckbox_handler, &new_cfg.clicks_place_cursor
  );

  s = ctrl_new_set(b, "Mouse", "Right click action");
  ctrl_radiobuttons(
    s, null, 4,
    dlg_stdradiobutton_handler, &new_cfg.right_click_action,
    "&Paste", RC_PASTE,
    "E&xtend", RC_EXTEND,
    "Show &menu", RC_MENU,
    null
  );

  s = ctrl_new_set(b, "Mouse", "Application mouse mode");
  ctrl_radiobuttons(
    s, "Default click target", 4,
    dlg_stdradiobutton_handler, &new_cfg.clicks_target_app,
    "&Window", false,
    "Applicatio&n", true,
    null
  );
  ctrl_radiobuttons(
    s, "Modifier for overriding default", 4,
    dlg_stdradiobutton_handler, &new_cfg.click_target_mod,
    "&Shift", MDK_SHIFT,
    "&Ctrl", MDK_CTRL,
    "&Alt", MDK_ALT,
    "&Off", 0,
    null
  );

 /*
  * The Window panel.
  */
  s = ctrl_new_set(b, "Window", "Default size");
  ctrl_columns(s, 5, 35, 3, 28, 4, 30);
  (cols_box = ctrl_editbox(
    s, "Colu&mns", 44, dlg_stdintbox_handler, &new_cfg.cols
  ))->column = 0;
  (rows_box = ctrl_editbox(
    s, "Ro&ws", 55, dlg_stdintbox_handler, &new_cfg.rows
  ))->column = 2;
  ctrl_pushbutton(
    s, "C&urrent size", current_size_handler, 0
  )->column = 4;

  s = ctrl_new_set(b, "Window", null);
  ctrl_columns(s, 2, 66, 34);
  ctrl_editbox(
    s, "Scroll&back lines", 50,
    dlg_stdintbox_handler, &new_cfg.scrollback_lines
  )->column = 0;
  ctrl_radiobuttons(
    s, "Scrollbar", 4,
    dlg_stdradiobutton_handler, &new_cfg.scrollbar,
    "&Left", -1,
    "&None", 0,
    "&Right", 1,
    null
  );
  ctrl_radiobuttons(
    s, "Modifier for scrolling", 4,
    dlg_stdradiobutton_handler, &new_cfg.scroll_mod,
    "&Shift", MDK_SHIFT,
    "&Ctrl", MDK_CTRL,
    "&Alt", MDK_ALT,
    "&Off", 0,
    null
  );
  ctrl_checkbox(
    s, "&PgUp and PgDn scroll without modifier",
    dlg_stdcheckbox_handler, &new_cfg.pgupdn_scroll
  );

 /*
  * The Terminal panel.
  */
  s = ctrl_new_set(b, "Terminal", null);
  ctrl_columns(s, 2, 50, 50);
  ctrl_combobox(
    s, "&Type", 100, term_handler, 0
  )->column = 0;
  ctrl_editbox(
    s, "&Answerback", 100, dlg_stdstringbox_handler, &new_cfg.answerback
  )->column = 1;

  s = ctrl_new_set(b, "Terminal", "Bell");
  ctrl_columns(s, 3, 25, 25, 50);
  ctrl_checkbox(
    s, "&Sound", dlg_stdcheckbox_handler, &new_cfg.bell_sound
  )->column = 0;
  ctrl_checkbox(
    s, "&Flash", dlg_stdcheckbox_handler, &new_cfg.bell_flash
  )->column = 1;
  ctrl_checkbox(
    s, "&Highlight in taskbar", dlg_stdcheckbox_handler, &new_cfg.bell_taskbar
  )->column = 2;

  s = ctrl_new_set(b, "Terminal", "Printer");
  ctrl_combobox(
    s, null, 100, printerbox_handler, 0
  );

  s = ctrl_new_set(b, "Terminal", null);
  ctrl_checkbox(
    s, "&Prompt about running processes on close",
    dlg_stdcheckbox_handler, &new_cfg.confirm_exit
  );
}
#endif  // #ifdef MINTTY_WINDOWS_DIALOG_BOXES
