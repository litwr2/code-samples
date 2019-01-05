/*
 * XLife Copyright 1989 Jon Bennett jb7m+@andrew.cmu.edu, jcrb@cs.cmu.edu
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

/*
A lot of modifications were added at 2001, 2011-14 by Vladimir Lidovski vol.litwr@gmail.com
(C) This version of XLife may be used under the same conditions as mentioned above
$Id: data.cpp 323 2014-03-06 14:51:11Z qiray $
*/

#include <ctype.h>
#include "defs.h"

#if STATEBITS > 3
int is_state(char c) {
   return isalnum(c) || c == '~' || c == '@';
}

int stoi(char c) {
   if (isdigit(c)) return c - '0';
   if (isupper(c)) return c - '7';
   if (islower(c)) return c - 61;
   if (c == '@') return 62;
   if (c == '~') return 63;
   return -1;
}

char itos(int n) {
   if (n < 10) return '0' + n;
   if (n < 36) return '7' + n;
   if (n < 62) return 61 + n;
   if (n == 62) return '@';
   return '~';
}
#endif

char saveformat = 'M', scriptformat = 'I';

/* X I/O state information */
Display *disp;
Window rootw, mainw, lifew, helpw, inputw, coordw, rulew, statew, loadw, modew;
int screen;
unsigned long fcolor, bcolor;
XEvent event;
XFontStruct *nfont, *cfont;
GC ntextgc, itextgc, cellgc[MAXCOLORS], xorgc, invgc;
KeySym ks;

char inpbuf[INPBUFLEN];
int minbuflen, numcomments;
char comments[MAXCOMMENTS][MAXCOMMENTLINELEN];
char keybuf[16];
u32bits lookup4[0x888900];
u16bits *lookup2 = (u16bits*)lookup4, tab6[65536];
u8bits *lookup = (u8bits*)lookup4, tab4[65536];
char fname[PATNAMESIZ];

unsigned maxstates, statescols, speed;
coord_t xpos, ypos;      /* coords of upper left corner of viewport */
coord_t xorigin, yorigin;/* virtual origin */
coord_t lastx, lasty;    /* last coord pair displayed on status line */
coord_t loadx, loady;    /* location to load new points */
coord_t iloadx, iloady;  /* initial location of load points */
int txx, txy, tyx, tyy;  /* transform for tentative points */

int scale;
u32bits born = 8, live = 12;

int width, maxwidth, height, inputlength, state, ev_mode = VALENCE_DRIVEN,
  paintcolor = 1;
int dispcoord, dispboxes, dispchanges, dispspeed = 1;
struct timeval timeout;

char active_rules[RULESNAMELEN], saved_rules[RULESNAMELEN];
int oldp, convmode, wireframe, dispmesh = 1, tilesize, limits, helpw_mode,
   pseudocolor, oscillators, historymode, palettemod, nosymm, aloadmode, pivot,
   rotate4reflect, truehistory, save_dispboxes, save_dispchanges, passive,
   hashmode;
char *dirs[MAXDIRS], rfullpath[PATNAMESIZ];
unsigned delay, hideperiod = 1, randomseed, eo_comments, rulew_len, runcounter;
char stashed[PATNAMESIZ], outcome[PATNAMESIZ], topology = 'Q', topologyinfo[32],
   pathtorules[PATNAMESIZ];

coord_t x_min_limit, x_max_limit, y_min_limit, y_max_limit;
float rnd_density = .9;

char curdir[PATNAMESIZ], inidir[PATNAMESIZ], colorfile[PATNAMESIZ];
cell_t *trans;

#if VFREQ != 0
struct timeval prev_vsync;
#endif

bigNumber bigXPos(xpos), bigYPos(ypos), bigDX(0), bigDY(0), bigScreenWidth(0), bigScreenHeight(0);

