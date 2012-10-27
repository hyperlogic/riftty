#include "win.h"


enum {LDRAW_CHAR_NUM = 31, LDRAW_CHAR_TRIES = 4};

// VT100 linedraw character mappings for current font.
wchar win_linedraw_chars[LDRAW_CHAR_NUM];

bool font_ambig_wide;

static const wchar linedraw_chars[LDRAW_CHAR_NUM][LDRAW_CHAR_TRIES] = {
  {0x25C6, 0x2666, '*'},           // 0x60 '`' Diamond 
  {0x2592, '#'},                   // 0x61 'a' Checkerboard (error)
  {0x2409, 0x2192, 0x01AD, 't'},   // 0x62 'b' Horizontal tab
  {0x240C, 0x21A1, 0x0192, 'f'},   // 0x63 'c' Form feed
  {0x240D, 0x21B5, 0x027C, 'r'},   // 0x64 'd' Carriage return
  {0x240A, 0x21B4, 0x019E, 'n'},   // 0x65 'e' Linefeed
  {0x00B0, 'o'},                   // 0x66 'f' Degree symbol
  {0x00B1, '~'},                   // 0x67 'g' Plus/minus
  {0x2424, 0x21B4, 0x019E, 'n'},   // 0x68 'h' Newline
  {0x240B, 0x2193, 0x028B, 'v'},   // 0x69 'i' Vertical tab
  {0x2518, '+'},                   // 0x6A 'j' Lower-right corner
  {0x2510, '+'},                   // 0x6B 'k' Upper-right corner
  {0x250C, '+'},                   // 0x6C 'l' Upper-left corner
  {0x2514, '+'},                   // 0x6D 'm' Lower-left corner
  {0x253C, '+'},                   // 0x6E 'n' Crossing lines
  {0x23BA, 0x00AF, '-'},           // 0x6F 'o' High horizontal line
  {0x23BB, 0x00AF, '-'},           // 0x70 'p' Medium-high horizontal line
  {0x2500, 0x2015, 0x2014, '-'},   // 0x71 'q' Middle horizontal line
  {0x23BC, '_'},                   // 0x72 'r' Medium-low horizontal line
  {0x23BF, '_'},                   // 0x73 's' Low horizontal line
  {0x251C, '+'},                   // 0x74 't' Left "T"
  {0x2524, '+'},                   // 0x75 'u' Right "T"
  {0x2534, '+'},                   // 0x76 'v' Bottom "T"
  {0x252C, '+'},                   // 0x77 'w' Top "T"
  {0x2502, '|'},                   // 0x78 'x' Vertical bar
  {0x2264, '#'},                   // 0x79 'y' Less than or equal to
  {0x2265, '#'},                   // 0x7A 'z' Greater than or equal to
  {0x03C0, '#'},                   // 0x7B '{' Pi
  {0x2260, '#'},                   // 0x7C '|' Not equal to
  {0x00A3, 'L'},                   // 0x7D '}' UK pound sign
  {0x00B7, '.'},                   // 0x7E '~' Centered dot
};

void win_reconfig(void)
{
    
}

/* void win_update(void); */
void win_update()
{
    ;
}

/* void win_schedule_update(void); */
void win_schedule_update(void)
{
    // do nothing, rendering will occur anyhow
}

/* void win_text(int x, int y, wchar *text, int len, uint attr, int lattr); */
void win_text(int x, int y, wchar *text, int len, uint attr, int lattr)
{

}

/* void win_update_mouse(void); */
void win_update_mouse(void)
{
    // change cursor type from arrow to bar
    // based on term.mouse_mode etc.
}

/* void win_capture_mouse(void); */
/* void win_bell(void); */
void win_bell(void)
{

}

/* void win_set_title(char *); */
void win_set_title(char *title)
{
    // change window title
}

/* void win_save_title(void); */
void win_save_title(void)
{
    // save title?
}

/* void win_restore_title(void); */
void win_restore_title(void)
{
    // restore saved title?
}


/* colour win_get_colour(colour_i); */
colour win_get_colour(colour_i i)
{
    return 0;
}

/* void win_set_colour(colour_i, colour); */
void win_set_colour(colour_i i, colour c)
{
    ;
}

/* void win_reset_colours(void); */
void win_reset_colours(void)
{
    ;
}

colour win_get_sys_colour(bool fg)
{
    return 0;
}

/* void win_invalidate_all(void); */
void win_invalidate_all(void)
{
    ;
}

/* void win_set_pos(int x, int y); */
void win_set_pos(int x, int y)
{
    // move window.
}

/* void win_set_chars(int rows, int cols); */
void win_set_chars(int rows, int cols)
{
    // resize window?
}

/* void win_set_pixels(int height, int width); */
void win_set_pixels(int height, int width)
{
    // resize window in pixels rather in characters
}

/* void win_maximise(int max); */
void win_maximise(int max)
{
    // 2 is fullscreen? 0 is not
}

/* void win_set_zorder(bool top); */
void win_set_zorder(bool top)
{
    // move to top or back.
}

/* void win_set_iconic(bool); */
void win_set_iconic(bool iconic)
{
    // minimize or restore window
}

/* void win_update_scrollbar(void); */
void win_update_scrollbar(void)
{
    // display or hide scroll bar
    // int scrollbar = term.show_scrollbar ? cfg.scrollbar : 0;
}

/* bool win_is_iconic(void); */
bool win_is_iconic(void)
{
    return 0;
}

/* void win_get_pos(int *xp, int *yp); */
void win_get_pos(int *xp, int *yp)
{
    *xp = 0;
    *yp = 0;
}

/* void win_get_pixels(int *height_p, int *width_p); */
void win_get_pixels(int *height_p, int *width_p)
{
    *height_p = 640;
    *width_p = 480;
}

/* void win_get_screen_chars(int *rows_p, int *cols_p); */
void win_get_screen_chars(int *rows_p, int *cols_p)
{
    *rows_p = 24;
    *cols_p = 80;
}

/* void win_popup_menu(void); */

/* void win_zoom_font(int); */
void win_zoom_font(int size)
{
    // what?
}

/* void win_set_font_size(int); */
void win_set_font_size(int size)
{
    ;
}

/* uint win_get_font_size(void); */
uint win_get_font_size(void)
{
    return 10;
}


/* void win_check_glyphs(wchar *wcs, uint num); */
void win_check_glyphs(wchar *wcs, uint num)
{
    // huh?
}


/* void win_open(wstring path); */
void win_open(mintty_wstring path)
{
    // no clue.
}

/* void win_copy(const wchar *data, uint *attrs, int len); */
void win_copy(const wchar *data, uint *attrs, int len)
{
    // copy into system clipboard.
}

/* void win_paste(void); */

/* void win_set_timer(void_fn cb, uint ticks); */
void win_set_timer(void_fn cb, uint ticks)
{
    ;
}

/* void win_show_about(void); */
void win_show_error(wchar * error)
{
    fprintf(stderr, "win_show_error %p\n", error);
}

/* bool win_is_glass_available(void); */

/* int get_tick_count(void); */
int get_tick_count(void)
{
    return 1;
}

/* int cursor_blink_ticks(void); */
int cursor_blink_ticks(void)
{
    return 1;
}


/* int win_char_width(xchar); */
int win_char_width(xchar c)
{
    return 0;
}

/* wchar win_combine_chars(wchar bc, wchar cc); */
/* Try to combine a base and combining character into a precomposed one.
 * Returns 0 if unsuccessful.
 */
wchar win_combine_chars(wchar bc, wchar cc)
{
    return 0;
}

