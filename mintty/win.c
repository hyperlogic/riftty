#include "win.h"
#include "gb_context.h"
#include "gb_font.h"
#include "gb_text.h"

enum {LDRAW_CHAR_NUM = 31, LDRAW_CHAR_TRIES = 4};

// TODO: REMOVE nobody appears to use linedraw_chars

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

struct WIN_Context s_context;
char s_temp[4096];
uint32_t s_ansi_colors[COLOUR_NUM];

static uint32_t MakeColor(uint8_t red, uint8_t green, uint8_t blue)
{
    uint8_t alpha = 255;
    return alpha << 24 | blue << 16 | green << 8 | red;
}

static void init_colors(void)
{
    // from http://en.wikipedia.org/wiki/ANSI_escape_code for terminal.app
    s_ansi_colors[BLACK_I] = MakeColor(0, 0, 0);
    s_ansi_colors[RED_I] = MakeColor(194, 54, 33);
    s_ansi_colors[GREEN_I] = MakeColor(37, 188, 36);
    s_ansi_colors[YELLOW_I] = MakeColor(173, 173, 39);
    s_ansi_colors[BLUE_I] = MakeColor(73, 46, 225);
    s_ansi_colors[MAGENTA_I] = MakeColor(211, 56, 211);
    s_ansi_colors[CYAN_I] = MakeColor(51, 187, 200);
    s_ansi_colors[WHITE_I] = MakeColor(203, 204, 205);

    s_ansi_colors[BOLD_BLACK_I] = MakeColor(129, 131, 131);
    s_ansi_colors[BOLD_RED_I] = MakeColor(252,57,31);
    s_ansi_colors[BOLD_GREEN_I] = MakeColor(49, 231, 34);
    s_ansi_colors[BOLD_YELLOW_I] = MakeColor(234, 236, 35);
    s_ansi_colors[BOLD_BLUE_I] = MakeColor(88, 51, 255);
    s_ansi_colors[BOLD_MAGENTA_I] = MakeColor(249, 53, 248);
    s_ansi_colors[BOLD_CYAN_I] = MakeColor(20, 240, 240);
    s_ansi_colors[BOLD_WHITE_I] = MakeColor(233, 235, 235);

    int r, g, b;
    for (r = 0; r < 16; r++) {
        for (g = 0; g < 16; g++) {
            for (b = 0; b < 16; b++) {
                s_ansi_colors[36 * r + 6 * g + b + 16] = MakeColor(r * 17, g * 17, b * 17);
            }
        }
    }

    int i;
    for (i = 0; i < 23; i++) {
        uint8_t intensity = (uint8_t)((i + 1) * (255.0f / 24.0f));
        s_ansi_colors[232 + i] = MakeColor(intensity, intensity, intensity);
    }

    s_ansi_colors[FG_COLOUR_I] = s_ansi_colors[WHITE_I];
    s_ansi_colors[BOLD_FG_COLOUR_I] = s_ansi_colors[BOLD_WHITE_I];

    s_ansi_colors[BG_COLOUR_I] = s_ansi_colors[BLACK_I];
    s_ansi_colors[BOLD_BG_COLOUR_I] = s_ansi_colors[BOLD_BLACK_I];

    s_ansi_colors[CURSOR_TEXT_COLOUR_I] = MakeColor(0, 0, 0);
    s_ansi_colors[CURSOR_COLOUR_I] = MakeColor(255, 255, 255);
    s_ansi_colors[IME_CURSOR_COLOUR_I] = MakeColor(255, 255, 255);
}

void win_init(void)
{
    init_colors();

    memset(&s_context, 0, sizeof(struct WIN_Context));

    // create the glyphblaster context
    GB_ERROR err;
    err = GB_ContextMake(128, 3, GB_TEXTURE_FORMAT_ALPHA, &s_context.gb);
    if (err != GB_ERROR_NONE) {
        fprintf(stderr, "GB_Init Error %d\n", err);
        exit(1);
    }

    // create a monospace font
    err = GB_FontMake(s_context.gb, "font/SourceCodePro-Bold.ttf", 14, GB_RENDER_NORMAL, GB_HINT_NONE, &s_context.font);
    if (err != GB_ERROR_NONE) {
        fprintf(stderr, "GB_MakeFont Error %s\n", GB_ErrorToString(err));
        exit(1);
    }

    // start off with with 4 text ptrs
    s_context.text = malloc(sizeof(struct GB_Text*) * 4);
    s_context.textCapacity = 4;
    s_context.textCount = 0;
}

static void win_clear_text()
{
    // delete all texts
    int i;
    for (i = 0; i < s_context.textCount; i++) {
        GB_TextRelease(s_context.gb, s_context.text[i]);
        s_context.text[i] = NULL;
    }
    s_context.textCount = 0;
}

void win_shutdown(void)
{
    win_clear_text();
    free(s_context.text);

    GB_ERROR err = GB_FontRelease(s_context.gb, s_context.font);
    if (err != GB_ERROR_NONE) {
        fprintf(stderr, "GB_ReleaseFont Error %s\n", GB_ErrorToString(err));
        exit(1);
    }

    err = GB_ContextRelease(s_context.gb);
    if (err != GB_ERROR_NONE) {
        fprintf(stderr, "GB_Shutdown Error %s\n", GB_ErrorToString(err));
        exit(1);
    }
}

void win_reconfig(void)
{
    fprintf(stderr, "mintty: win_reconfig\n");
    fprintf(stderr, "    rows = %d, cols = %d\n", cfg.rows, cfg.cols);
}

/* void win_update(void); */
void win_update()
{
    ;
}

/* void win_schedule_update(void); */
void win_schedule_update(void)
{
    //fprintf(stderr, "mintty: win_schedule_update()\n");
    win_clear_text();

    // NOTE: because of the way rendering works with OpenGL we need to rebuild all the text
    // when anything changes, it would be difficult to re-use the text from a previous frame.
    term_invalidate(0, 0, cfg.rows, cfg.cols);

    term_paint();
}

/* void win_text(int x, int y, wchar *text, int len, uint attr, int lattr); */
void win_text(int x, int y, wchar *text, int len, uint attr, int lattr)
{
    //fprintf(stderr, "mintty: win_text() x = %d, y = %d, text = %p, len = %d, attr = %u, lattr = %d\n", x, y, text, len, attr, lattr);

    int fg_color = (attr & ATTR_FGMASK) >> ATTR_FGSHIFT;
    int bg_color = (attr & ATTR_BGMASK) >> ATTR_BGSHIFT;

    // realloc text ptrs, if necessary
    if (s_context.textCount == s_context.textCapacity)
    {
        s_context.textCapacity *= s_context.textCapacity;
        s_context.text = realloc(s_context.text, s_context.textCapacity * sizeof(struct GB_Text*));
    }

    // HACK: because GB_Text does not have a pen position.
    // we append the string with the appropriate amount of newlines and spaces.
    // use s_temp to build string

    // fill up s_indent string prefix string.
    int ii = 0;
    int i;
    for (i = 0; i < y; i++)
        s_temp[ii++] = '\n';
    for (i = 0; i < x; i++)
        s_temp[ii++] = ' ';

    assert(ii < 2048);  // s_temp overflow!
    assert(ii + len < 2048);  // s_temp overflow

    // HACK: convert each wchar to a char. fuck unicode.
    for (i = 0; i < len; i++)
        s_temp[ii + i] = (char)text[i];
    s_temp[ii + len] = 0;

    //fprintf(stderr, "    -> %s\n", s_temp + ii);

    // allocate a new text object!
    uint32_t origin[2] = {0, 0};
    uint32_t size[2] = {1000, 1000};
    struct GB_Text* gb_text = NULL;
    GB_ERROR err = GB_TextMake(s_context.gb, (uint8_t*)s_temp, s_context.font, s_ansi_colors[fg_color], origin, size,
                               GB_HORIZONTAL_ALIGN_LEFT, GB_VERTICAL_ALIGN_TOP, &gb_text);
    if (err != GB_ERROR_NONE) {
        fprintf(stderr, "GB_TextMake Error %s\n", GB_ErrorToString(err));
        exit(1);
    }

    s_context.text[s_context.textCount++] = gb_text;
}

// original win_text code from mintty
#if 0
/*
 * Draw a line of text in the window, at given character
 * coordinates, in given attributes.
 *
 * We are allowed to fiddle with the contents of `text'.
 */
void
win_text(int x, int y, wchar *text, int len, uint attr, int lattr)
{
    lattr &= LATTR_MODE;
    int char_width = font_width * (1 + (lattr != LATTR_NORM));

    /* Convert to window coordinates */
    x = x * char_width + PADDING;
    y = y * font_height + PADDING;

    if (attr & ATTR_WIDE)
        char_width *= 2;

    /* Only want the left half of double width lines */
    if (lattr != LATTR_NORM && x * 2 >= term.cols)
        return;

    uint nfont;
    switch (lattr) {
        when LATTR_NORM: nfont = 0;
        when LATTR_WIDE: nfont = FONT_WIDE;
        otherwise:       nfont = FONT_WIDE + FONT_HIGH;
    }
    if (attr & ATTR_NARROW)
        nfont |= FONT_NARROW;

    if (bold_mode == BOLD_FONT && (attr & ATTR_BOLD))
        nfont |= FONT_BOLD;
    if (und_mode == UND_FONT && (attr & ATTR_UNDER))
        nfont |= FONT_UNDERLINE;
    another_font(nfont);

    bool force_manual_underline = false;
    if (!fonts[nfont]) {
        if (nfont & FONT_UNDERLINE)
            force_manual_underline = true;
        // Don't force manual bold, it could be bad news.
        nfont &= ~(FONT_BOLD | FONT_UNDERLINE);
    }
    another_font(nfont);
    if (!fonts[nfont])
        nfont = FONT_NORMAL;

    colour_i fgi = (attr & ATTR_FGMASK) >> ATTR_FGSHIFT;
    colour_i bgi = (attr & ATTR_BGMASK) >> ATTR_BGSHIFT;

    if (term.rvideo) {
        if (fgi >= 256)
            fgi ^= 2;
        if (bgi >= 256)
            bgi ^= 2;
    }
    if (attr & ATTR_BOLD && cfg.bold_as_colour) {
        if (fgi < 8)
            fgi |= 8;
        else if (fgi >= 256 && !cfg.bold_as_font)
            fgi |= 1;
    }
    if (attr & ATTR_BLINK) {
        if (bgi < 8)
            bgi |= 8;
        else if (bgi >= 256)
            bgi |= 1;
    }

    colour fg = colours[fgi];
    colour bg = colours[bgi];

    if (attr & ATTR_DIM) {
        fg = (fg & 0xFEFEFEFE) >> 1; // Halve the brightness.
        if (!cfg.bold_as_colour || fgi >= 256)
            fg += (bg & 0xFEFEFEFE) >> 1; // Blend with background.
    }
    if (attr & ATTR_REVERSE) {
        colour t = fg; fg = bg; bg = t;
    }
    if (attr & ATTR_INVISIBLE)
        fg = bg;

    bool has_cursor = attr & (TATTR_ACTCURS | TATTR_PASCURS);
    colour cursor_colour = 0;

    if (has_cursor) {
        colour wanted_cursor_colour =
            colours[ime_open ? IME_CURSOR_COLOUR_I : CURSOR_COLOUR_I];

        bool too_close = colour_dist(wanted_cursor_colour, bg) < 32768;

        cursor_colour =
            too_close ? colours[CURSOR_TEXT_COLOUR_I] : wanted_cursor_colour;

        if ((attr & TATTR_ACTCURS) && term_cursor_type() == CUR_BLOCK) {
            bg = cursor_colour;
            fg = too_close ? wanted_cursor_colour : colours[CURSOR_TEXT_COLOUR_I];
        }
    }

    SelectObject(dc, fonts[nfont]);
    SetTextColor(dc, fg);
    SetBkColor(dc, bg);

    /* Check whether the text has any right-to-left characters */
    bool has_rtl = false;
    for (int i = 0; i < len && !has_rtl; i++)
        has_rtl = is_rtl(text[i]);

    uint eto_options = ETO_CLIPPED;
    if (has_rtl) {
        /* We've already done right-to-left processing in the screen buffer,
         * so stop Windows from doing it again (and hence undoing our work).
         * Don't always use this path because GetCharacterPlacement doesn't
         * do Windows font linking.
         */
        char classes[len];
        memset(classes, GCPCLASS_NEUTRAL, len);

        GCP_RESULTSW gcpr = {
            .lStructSize = sizeof(GCP_RESULTSW),
            .lpClass = (void *)classes,
            .lpGlyphs = text,
            .nGlyphs = len
        };

        GetCharacterPlacementW(dc, text, len, 0, &gcpr,
                               FLI_MASK | GCP_CLASSIN | GCP_DIACRITIC);
        len = gcpr.nGlyphs;
        eto_options |= ETO_GLYPH_INDEX;
    }

    bool combining = attr & TATTR_COMBINING;
    int width = char_width * (combining ? 1 : len);
    RECT box = {
        .left = x, .top = y,
        .right = min(x + width, font_width * term.cols + PADDING),
        .bottom = y + font_height
    };

    /* Array with offsets between neighbouring characters */
    int dxs[len];
    int dx = combining ? 0 : char_width;
    for (int i = 0; i < len; i++)
        dxs[i] = dx;

    int yt = y + cfg.row_spacing - font_height * (lattr == LATTR_BOT);

    /* Finally, draw the text */
    SetBkMode(dc, OPAQUE);
    ExtTextOutW(dc, x, yt, eto_options | ETO_OPAQUE, &box, text, len, dxs);

    /* Shadow bold */
    if (bold_mode == BOLD_SHADOW && (attr & ATTR_BOLD)) {
        SetBkMode(dc, TRANSPARENT);
        ExtTextOutW(dc, x + 1, yt, eto_options, &box, text, len, dxs);
    }

    /* Manual underline */
    if (lattr != LATTR_TOP &&
        (force_manual_underline ||
         (und_mode == UND_LINE && (attr & ATTR_UNDER)))) {
        int dec = (lattr == LATTR_BOT) ? descent * 2 - font_height : descent;
        HPEN oldpen = SelectObject(dc, CreatePen(PS_SOLID, 0, fg));
        MoveToEx(dc, x, y + dec, null);
        LineTo(dc, x + len * char_width, y + dec);
        oldpen = SelectObject(dc, oldpen);
        DeleteObject(oldpen);
    }

    if (has_cursor) {
        HPEN oldpen = SelectObject(dc, CreatePen(PS_SOLID, 0, cursor_colour));
        switch(term_cursor_type()) {
            when CUR_BLOCK:
                if (attr & TATTR_PASCURS) {
                    HBRUSH oldbrush = SelectObject(dc, GetStockObject(NULL_BRUSH));
                    Rectangle(dc, x, y, x + char_width, y + font_height);
                    SelectObject(dc, oldbrush);
                }
            when CUR_LINE: {
                int caret_width = 1;
                SystemParametersInfo(SPI_GETCARETWIDTH, 0, &caret_width, 0);
                if (caret_width > char_width)
                    caret_width = char_width;
                if (attr & TATTR_RIGHTCURS)
                    x += char_width - caret_width;
                if (attr & TATTR_ACTCURS) {
                    HBRUSH oldbrush = SelectObject(dc, CreateSolidBrush(cursor_colour));
                    Rectangle(dc, x, y, x + caret_width, y + font_height);
                    DeleteObject(SelectObject(dc, oldbrush));
                }
                else if (attr & TATTR_PASCURS) {
                    for (int dy = 0; dy < font_height; dy += 2)
                        Polyline(
                            dc, (POINT[]){{x, y + dy}, {x + caret_width, y + dy}}, 2);
                }
            }
            when CUR_UNDERSCORE:
                y += min(descent, font_height - 2);
                if (attr & TATTR_ACTCURS)
                    Rectangle(dc, x, y, x + char_width, y + 2);
                else if (attr & TATTR_PASCURS) {
                    for (int dx = 0; dx < char_width; dx += 2) {
                        SetPixel(dc, x + dx, y, cursor_colour);
                        SetPixel(dc, x + dx, y + 1, cursor_colour);
                    }
                }
        }
        DeleteObject(SelectObject(dc, oldpen));
    }
}

#endif  // if 0

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

