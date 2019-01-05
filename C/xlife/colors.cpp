/*
 * XLife Copyright 2011-13 Vladimir Lidovski vol.litwr@gmail.com
 * $Id: colors.cpp 310 2014-02-11 07:40:09Z litwr $
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

#include "defs.h"
#include "colors.h"

XColor cellcolor[MAXCOLORS], exact;
XGCValues xgcv;
char currentcolors[256][16];
const char *fixedcolors[] = {
   "Black",
   "White",
   "Brown",
   "Blue",
   "Green",
   "#cb0000" /* lighted red */,
   "Yellow",
   "Purple",
   "Orange",
   "Magenta",
   "Cyan",
   "Grey",
   "Pink",
   "Plum",
   "#ADFF2F", /* lightgreen */
   "Tan",
   "#e50000",
   "#fe3e2d",
   "#ff5151",
   "#ff7575",
   "#ff9998",
   "#ffbcbc",
   "#ffd1d1",
   "#7a0000",
   "#913030",
   "#ac3939",
   "#ca6262",
   "#d37c7c",
   "#dc9797",
   "#e5b2b2",
   "#eecdcd",
   "#7a3300",
   "#9e4100",
   "#c15000",
   "#e55f00",
   "#ff700a",
   "#fe852d",
   "#ff9951",
   "#ffae75",
   "#ffc399",
   "#ffd8bc",
   "#ffe4d1",
   "#915830",
   "#c17a47",
   "#ca8d62",
   "#d3a17c",
   "#dcb497",
   "#e5c7b2",
   "#eedbcd",
   "#7a4700",
   "#9e5c00",
   "#c17100",
   "#e58500",
   "#ffb651",
   "#ffc575",
   "#564100",
   "#7a5b00",
   "#9e7600",
   "#c19100",
   "#e5ac00",
   "#ffc10a",
   "#feca2d",
   "#ffd351",
   "#ffdc75",
   "#ffe599",
   "#ffeebc",
   "#fff3d1",
   "#917930",
   "#c1a347",
   "#d3bd7c",
   "#dccb97",
   "#e5d8b2",
   "#eee6cd",
   "#564f00",
   "#7a7000",
   "#9e9000",
   "#c1b100",
   "#e5d200",
   "#ffea0a",
   "#fff051",
   "#fff375",
   "#fff699",
   "#fff9bc",
   "#fffbd1",
   "#918930",
   "#c1b747",
   "#d3cc7c",
   "#dcd697",
   "#4f5600",
   "#707a00",
   "#b1c100",
   "#d2e500",
   "#eaff0a",
   "#f0ff51",
   "#f3ff75",
   "#f6ff99",
   "#f9ffbc",
   "#899130",
   "#b7c147",
   "#c2ca62",
   "#d6dc97",
   "#e1e5b2",
   "#415600",
   "#91c100",
   "#ace500",
   "#c1ff0a",
   "#d3ff51",
   "#dcff75",
   "#e5ff99",
   "#eeffbc",
   "#f3ffd1",
   "#799130",
   "#8fac39",
   "#b0ca62",
   "#bdd37c",
   "#d8e5b2",
   "#e6eecd",
   "#005600",
   "#007a00",
   "#009e00",
   "#00c100",
   "#00e500",
   "#51ff51",
   "#75ff75",
   "#99ff99",
   "#bcffbc",
   "#d1ffd1",
   "#309130",
   "#39ac39",
   "#47c147",
   "#62ca62",
   "#7cd37c",
   "#97dc97",
   "#b2e5b2",
   "#cdeecd",
   "#005639",
   "#007a51",
   "#009e69",
   "#00c181",
   "#00e599",
   "#0affad",
   "#51ffc5",
   "#75ffd1",
   "#bcffe8",
   "#d1ffef",
    "#309171",
   "#47c199",
   "#62caa7",
   "#97dcc5",
   "#cdeee3",
   "#00564f",
   "#007a70",
   "#009e90",
   "#00c1b1",
   "#00e5d2",
   "#0affea",
   "#75fff3",
   "#99fff6",
   "#d1fffb",
   "#309189",
   "#47c1b7",
   "#7cd3cc",
   "#004856",
   "#00657a",
   "#00a1c1",
   "#00bfe5",
   "#0ad6ff",
   "#51e2ff",
   "#99edff",
   "#d1f7ff",
   "#308191",
   "#47adc1",
   "#7cc5d3",
   "#b2dde5",
   "#cde8ee",
   "#001c56",
   "#00287a",
   "#00349e",
   "#0040c1",
   "#2d73fe",
   "#518bff",
   "#75a3ff",
   "#99baff",
   "#bcd2ff",
   "#d1e0ff",
   "#305091",
   "#395fac",
   "#6285ca",
   "#7c99d3",
   "#97aedc",
   "#b2c3e5",
   "#cdd8ee",
   "#3d007a",
   "#6000c1",
   "#840aff",
   "#a851ff",
   "#cb99ff",
   "#ddbcff",
   "#e8d1ff",
   "#603091",
   "#7239ac",
   "#9662ca",
   "#ba97dc",
   "#cbb2e5",
   "#ddcdee",
   "#480056",
   "#65007a",
   "#a100c1",
   "#bf00e5",
   "#d60aff",
   "#dc2dfe",
   "#e251ff",
   "#e875ff",
   "#ed99ff",
   "#f3bcff",
   "#f7d1ff",
   "#813091",
   "#ad47c1",
   "#b962ca",
   "#c57cd3",
   "#d197dc",
   "#56004f",
   "#7a0070",
   "#9e0090",
   "#c100b1",
   "#e500d2",
   "#ff0ae2",
   "#fe2df7",
   "#ff51e8",
   "#ff75f3",
   "#ff99f6",
   "#913089",
   "#ac39a2",
   "#c147b7",
   "#ca62c2",
   "#d37ccc",
   "#dc97d6",
   "#e5b2e1",
   "#f2d4f0",
   "#7a003d",
   "#c10060",
   "#e50072",
   "#ff0a84",
   "#fe2d96",
   "#ff51a8",
   "#ff75ba",
   "#ff99cc",
   "#ffbcdd",
   "#ffd1e8",
   "#913060",
   "#ac395f",
   "#c14784",
   "#ca6296",
   "#d37ca8",
   "#e5b2cc",
   "#eecddd",
   "#ecb088", /* 256 */
   "#444444",
   "#777777",
   "#ff0000",
   "Orange", /*"#562400",*/
   "#d1fffb",
   "Black",
   "White",
   "Green",
   "#CCCCCC",  /* + 9 */
   "#000082",
   "#620000",
   "Blue",
   "#000000"}; /* reserve */

struct AdjustColorPair historycolors[] = {{0, "Black"}, {1, "Green"}, {2, "#000080"},
   {3, "#d8ffd8"}, {4, "Red"}, {5, "Yellow"}, {6, "#616161"}};
/*{{0, "Black"}, {1, "White"}, {2, "#333333"},
   {3, "#d1f7ff"}, {4, "#001c56"}, {5, "Green"}}*/;
struct NewColorPair newcolors[MAXCOLORS];

void InitCurrentColors(void) {
   int i;
   for (i = 0; i < 256; i++)
      strcpy(currentcolors[i], fixedcolors[i]);
}

void setonecolor(int i, const char* s) { /* sets global var exact */
   if (!XAllocNamedColor(disp, DefaultColormap(disp, screen), s, cellcolor + i,
                                                                       &exact))
      fatal("Color allocation failed!\n");
}

void IniPalette(void) {
   int i;
   InitCurrentColors();
   for (i = 0; i < MAXCOLORS; i++) {
      if (i < MAXSTATES)
         setonecolor(i, currentcolors[i]);
      else {
         if (!XAllocNamedColor(disp, DefaultColormap(disp, screen),
                      fixedcolors[i - MAXSTATES + 256], &cellcolor[i], &exact))
            fatal("Color allocation failed!\n");
      }
      xgcv.foreground = exact.pixel;
      if ((cellgc[i] = XCreateGC(disp, mainw, GCForeground, &xgcv)) == 0)
         fatal("Color creation failed!\n");
   }
}

void DefaultPalette(void) {
   int i;
   *colorfile = 0;
   for (i = 0; i < MAXSTATES; i++) {
      setonecolor(i, currentcolors[i]);
      xgcv.foreground = exact.pixel;
      XChangeGC(disp, cellgc[i], GCForeground, &xgcv);
   }
   XSetWindowBackground(disp, lifew, bcolor);
   redraw_lifew();
   showstates();
}

void HistoryPalette() {
   int i, k;
   for (i = 0; i < sizeof(historycolors)/sizeof(struct AdjustColorPair); i++) {
      k = historycolors[i].cn;
      setonecolor(k, historycolors[i].c);
      xgcv.foreground = exact.pixel;
      XChangeGC(disp, cellgc[k], GCForeground, &xgcv);
   }
   XSetWindowBackground(disp, lifew, cellcolor->pixel);
   redraw_lifew();
   showstates();
}
