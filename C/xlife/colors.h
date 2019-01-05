/*
 * XLife Copyright 2011-13 Vladimir Lidovski vol.litwr@gmail.com
 * $Id: colors.h 310 2014-02-11 07:40:09Z litwr $
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of the copyright holders not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  The copyright holders make no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#define FREE_COLOR MAXSTATES
#define GRID_COLOR (MAXSTATES + 1)
#define GRID_ECOLOR (MAXSTATES + 2)
#define RED_EVER (MAXSTATES + 3)
#define ORANGE_EVER (MAXSTATES + 4)
#define CYAN_EVER (MAXSTATES + 5)
#define LOAD_BOX (MAXSTATES + 5)
#define BLACK_EVER (MAXSTATES + 6)
#define WHITE_EVER (MAXSTATES + 7)
#define GREEN_EVER (MAXSTATES + 8)
#define LOADW_BG (MAXSTATES + 9)
#define BLUE_EVER (MAXSTATES + 12)

struct AdjustColorPair {
   int cn;
   const char *c;
};

struct NewColorPair {
   int cn;
   char c[16];
};

extern unsigned long fcolor, bcolor;
extern struct AdjustColorPair historycolors[];
extern struct NewColorPair newcolors[];
extern char currentcolors[][16];
extern char currentcolors[][16];
extern const char *fixedcolors[];

void DefaultPalette(void);
void IniPalette(void);
void InitCurrentColors(void);
void HistoryPalette(void);
void setonecolor(int, const char*);
