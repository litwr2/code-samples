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
 * A lot of modifications were added at 2001, 2011-14 by Vladimir Lidovski vol.litwr@gmail.com
 * (C) This version of XLife may be used under the same conditions as mentioned above
 * $Id: utils.cpp 349 2014-05-30 08:48:16Z litwr $
 */

#include <pwd.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include "defs.h"
#include "tile.h"
#include "colors.h"

void announce(const char *s) { /* display a string in the input window */
   if (convmode) {
      fprintf(stderr, "%s\n", s);
      exit(2);
   }
   strcpy(inpbuf, s);
   DoExpose(inputw);
}

void announce_and_delay(const char *s) {
   XLowerWindow(disp, rulew);
   XLowerWindow(disp, coordw);
   XSetWindowBackground(disp, inputw, cellcolor[GREEN_EVER].pixel);
   announce(s);
   XFlush(disp);
   make_delay(0, 700000);
   XSetWindowBackground(disp, inputw, fcolor);
   XClearWindow(disp, inputw);
   XRaiseWindow(disp, rulew);
   XRaiseWindow(disp, coordw);
}

#ifdef MICROSOFT
void fatalw(const char *s) {
   XSelectInput(disp, mainw, KeyPressMask|ButtonPressMask|ExposureMask);
   XMapWindow(disp, mainw);
   XSetWindowBackground(disp, mainw, cellcolor[RED_EVER].pixel);
   XClearWindow(disp, mainw);
   for (;;) {
      XDrawString(disp, mainw, ntextgc, 10, 10, s, strlen(s));
      XMaskEvent(disp, KeyPressMask|ButtonPressMask|ExposureMask, &event);
      switch(event.type) {
      case ButtonPress:
      case KeyPress:
         goto finish;
      case Expose:;
      }
   }
finish: 
   exit(22);
}
#endif

void fatal(const char *s) {
   fprintf(stderr, s);
   exit(22);
}

void drawcell(coord_t x, coord_t y, cell_t c) {
   if (scale <= 0)
      XDrawPoint(disp, lifew, cellgc[c], x, y);
   else {
      int sc = (1 << scale) - (scale > 1);
      XFillRectangle(disp, lifew, cellgc[c], MOUSE2CELL(x), MOUSE2CELL(y), sc, sc);
   }
}

void drawpivot(void) {
   unsigned sc, x = RXPOS(loadx), y = RYPOS(loady);
   if (scale > 1) {
      sc = (1 << scale) - 1;
      XDrawLine(disp, lifew, xorgc, x, y, x + sc - 1, y + sc - 1);
      XDrawLine(disp, lifew, xorgc, x, y + sc - 1, x + sc - 1, y);
      XDrawPoint(disp, lifew, xorgc, x + sc/2, y + sc/2);
   }
   else
      XDrawPoint(disp, lifew, invgc, x, y);
   pivot = !pivot;
}

void drawbox(coord_t x1, coord_t y1, coord_t x2, coord_t y2, int color) {
/* box the area defined by two corners (life-world coordinates) */
   int xbase, ybase, xedge, yedge, cc, incr;
   cc = min(x1, x2) - xpos;
   xbase    = cc < 0 ? -1 : RSCALE(cc) - 1;
   cc = min(y1, y2) - ypos;
   ybase = cc < 0 ? -1 :RSCALE(cc) - 1;
   cc = max(x1, x2) - xpos;
   incr = scale < 1 ? 1 : ((1 << scale) - (scale > 1));
   xedge    = cc < 0 ? -1 : RSCALE(cc) + incr;
   cc = max(y1, y2) - ypos;
   yedge    = cc < 0 ? -1 : RSCALE(cc) + incr;
/* X server works only with shorts */
   if (xbase > 32767) xbase = 32767;
   if (ybase > 32767) ybase = 32767;
   if (xedge > 32767) xedge = 32767;
   if (yedge > 32767) yedge = 32767;

/* left-hand vertical line (doubled to cope with pixel aspect ratio) */
   if (dispmesh && scale > 1)
      if (color == 0) {
         XDrawLine(disp, lifew, cellgc[(xbase + 1)%5 ? GRID_COLOR : GRID_ECOLOR],
                                                   xbase, ybase, xbase, yedge);
         XDrawLine(disp, lifew, cellgc[(xedge + 1)%5 ? GRID_COLOR : GRID_ECOLOR],
                                                   xedge, ybase, xedge, yedge);
         XDrawLine(disp, lifew, cellgc[(ybase + 1)%5 ? GRID_COLOR : GRID_ECOLOR],
                                                   xbase, ybase, xedge, ybase);
         XDrawLine(disp, lifew, cellgc[(yedge + 1)%5 ? GRID_COLOR : GRID_ECOLOR],
                                                   xbase, yedge, xedge, yedge);
      }
      else
         XDrawRectangle(disp, lifew, cellgc[color], xbase, ybase, xedge - xbase,
                                                                yedge - ybase);
   else if (tentative.tiles) {
      XDrawRectangle(disp, lifew, xorgc, xbase, ybase, xedge - xbase,
                                                                yedge - ybase);
#ifndef MICROSOFT
      XDrawPoint(disp, lifew, xorgc, xbase, ybase);
#endif
   }
   else {
      XDrawRectangle(disp, lifew, invgc, xbase, ybase, xedge - xbase,
                                                                yedge - ybase);
#ifndef MICROSOFT
      XDrawPoint(disp, lifew, invgc, xbase, ybase);
#endif
   }
}

void erasebox(coord_t x1, coord_t y1, coord_t x2, coord_t y2) {
/* actually, overwrite the box outline in state 0 color (default = black) */
   drawbox(x1, y1, x2, y2, 0);
}

void showcoord(const int force) { /* update the coordinate & state display */
    static int f;
    /* Coordinate display added by Paul Callahan (callahan@cs.jhu.edu) */
    if (dispcoord) {
	coord_t x  = XPOS(event.xmotion.x), y = YPOS(event.xmotion.y);
	char pstring[50];
	f = 1;
	if (force || lastx != x || lasty != y) {
            int s = lookcell(&active, lastx = x, lasty = y);
            bigNumber bigX = XPOSBIG((bigNumber)event.xmotion.x), bigY = YPOSBIG((bigNumber)event.xmotion.y);
            if (hashmode)
	       sprintf(pstring, "(%s,%s)=%d", (bigX - xorigin).toString(7, 1).c_str(), (bigY - yorigin).toString(7, 1).c_str(), s);            
            else
	       sprintf(pstring, "(%d,%d)=%d", x - xorigin, y - yorigin, s);
            if (!hashmode && maxstates > 2 && dispboxes)
               if (s == 0)
                  strcat(pstring, "[*]");
               else
                  sprintf(pstring + strlen(pstring), "[%d]", active.cellcount[s]);
	    XClearWindow(disp, coordw);
	    XDrawString(disp, coordw, ntextgc, COORDXOFF, COORDYOFF,
		    pstring, strlen(pstring));
	}
    }
    else if (f) {
        XClearWindow(disp, coordw);
        f = 0;
    }
}

void showrules(void) {
   char buf[RULESNAMELEN], *cp, tmp[INPBUFLEN];
   strcpy(buf, active_rules);
   *buf = toupper(*buf);
   if ((ev_mode == TABLE_DRIVEN || ev_mode == TAB8_DRIVEN)
                                       && (cp = strrchr(buf, '.'))) *cp = '\0';
   if (topology != 'Q') {
      sprintf(tmp, ":%c", topology);
      if (limits) {
         if (limits&1)
            sprintf(tmp + strlen(tmp), "%ux", x_max_limit - x_min_limit);
         else
            strcat(tmp, "0x");
         if (limits&2)
            sprintf(tmp + strlen(tmp), "%u", y_max_limit - y_min_limit);
         else
            strcat(tmp, "0");
      }
      strcat(buf, tmp);
   }
   if ((strlen(buf) + 1)*FONTWIDTH != rulew_len) {
      rulew_len = (strlen(buf) + 1)*FONTWIDTH;
      ResizeLW(height - INPUTH - BORDERWIDTH*3);
   }
   XClearWindow(disp, rulew);
   XDrawString(disp, rulew, ntextgc, 1, FONTHEIGHT, buf, strlen(buf));
}

void showstates(void) {
   if (maxstates > 2) {
      int j;
      XClearWindow(disp, statew);
      for (j = 0; j < maxstates; j++)
         color_button(j, j, 0);
      setcolor(paintcolor, 0, 0);
      if (!(helpw_mode || eo_comments || state != STOP)) XRaiseWindow(disp, statew);
   }
}

void randomize(int width, int height, coord_t xpos, coord_t ypos) {
   cellcount_t num;
   coord_t x, y;
   if (limits)
      if ((limits&1) == 0)
         num = SCALE(width)*min(SCALE(height), y_max_limit - y_min_limit);
      else if ((limits&2) == 0)
         num = min(SCALE(width), x_max_limit - x_min_limit)*SCALE(height);
      else
         num = min(SCALE(width), x_max_limit - x_min_limit)
                           *min(SCALE(height), y_max_limit - y_min_limit);
   else
      num = SCALE(width)*SCALE(height);
   num *= rnd_density;
   while (num--) {
      x = random()%SCALE(width) + xpos;
      y = random()%SCALE(height) + ypos;
      if (!limits || chk_limits(x, y))
         chgcell(&active, x, y, historymode ? 5 : 1);
      else
         num++;
   }
   redisplay(0);
   displaystats();
}

void settimeout(unsigned long ms) {
   sprintf(inpbuf, "Delay %d msec", delay);
   DoExpose(inputw);
   timeout.tv_sec = ms/1000;
   timeout.tv_usec = (ms%1000)*1000;
}

void stamp(const char *leader, FILE *ofp) {
/* generate \n-terminated time and user info string */
    struct passwd *pw;
    char machine[80];
    time_t timeval;

    timeval = time(0);
    gethostname(machine, sizeof(machine));
    if (pw = getpwuid(getuid()))
        fprintf(ofp, "%s %s \"%s\"@%s %s",
                leader, pw->pw_name, pw->pw_gecos, machine, ctime(&timeval));
}

unsigned long mstime() {
   struct timeval start;
   gettimeofday(&start, 0);
   return start.tv_sec*1000 + start.tv_usec/1000;
}

void benchmark(void) {
   unsigned num, count;
   double tm;
   int save_dispboxes = dispboxes, save_dispchanges = dispchanges,
      save_dispspeed = dispspeed;
   struct timeval start, end;
   state = STOP;
   announce("Number of generations: ");
   minbuflen = 23;
   getxstring();
   if (sscanf(inpbuf + 23, "%u", &count) != 1) {
      displaystats();
      return;
   }
   dispspeed = dispchanges = dispboxes = 0;
   gettimeofday(&start, 0);
   for (num = 0; num < count; num++)
      if (tentative.tiles)
         generate(&tentative);
      else
         generate(&active);
   gettimeofday(&end, 0);
   dispspeed = save_dispspeed;
   dispchanges = save_dispchanges;
   if ((dispboxes = save_dispboxes) || dispchanges) bounding_box(&active);
   tm = end.tv_sec - start.tv_sec + (end.tv_usec - start.tv_usec)/1000000.0;
   redraw_lifew();
   sprintf(inpbuf, "Time: %fs, Speed: %u", tm,
                                           tm > 0 ? speed = count/tm + .5 : 0);
   announce_and_wait(0);
   displaystats();
}

void color_button(int location, int color, int large) {
    char s;
    int vp, hp;
    vp = (FONTHEIGHT + 3)*(location/statescols);
    hp = location%statescols;
    XFillRectangle(disp, statew, cellgc[color],
                     1 + STATEOFF + hp*(FONTWIDTH + 3) - large, 3 - large + vp,
                                FONTWIDTH + large*2, FONTHEIGHT - 1 + large*2);
    if (large == 0) {
        if (color > 99) {
          s = itos(color/100);
          XDrawString(disp, statew, invgc, 3 + STATEOFF + hp*(FONTWIDTH + 3),
                                                   FONTHEIGHT - 6 + vp, &s, 1);
          s = itos((color/10)%10);
          XDrawString(disp, statew, invgc, STATEOFF + hp*(FONTWIDTH + 3),
                                                   FONTHEIGHT + 2 + vp, &s, 1);
          s = itos(color%10);
          XDrawString(disp, statew, invgc, 5 + STATEOFF + hp*(FONTWIDTH + 3),
                                                   FONTHEIGHT + 2 + vp, &s, 1);
        }
        else if (color > 9) {
          s = itos(color/10);
          XDrawString(disp, statew, invgc, STATEOFF + hp*(FONTWIDTH + 3),
                                                       FONTHEIGHT + vp, &s, 1);
          s = itos(color%10);
          XDrawString(disp, statew, invgc, 5 + STATEOFF + hp*(FONTWIDTH + 3),
                                                       FONTHEIGHT + vp, &s, 1);
        }
        else {
          s = itos(color%10);
          XDrawString(disp, statew, xorgc, 1 + STATEOFF + hp*(FONTWIDTH + 3),
                                                       FONTHEIGHT + vp, &s, 1);
        }
    }
}

void setcolor(int val, unsigned long x, unsigned long y) {
   if (maxstates == 2) return;
   if (val == -1) {
      val = x/(FONTWIDTH + 3) + y/(FONTHEIGHT + 3)*statescols;
      if (val >= maxstates) return;
   }
   color_button(paintcolor, WHITE_EVER, 2);
   color_button(paintcolor, paintcolor, 0);
   color_button(val, BLACK_EVER, 2);
   color_button(val, val, 0);
   paintcolor = val;
}

void set_paintcolor(void) {
   if (maxstates == 2)
      paintcolor = 1;
   else
      paintcolor %= maxstates;
}

void entercolor(unsigned long x, unsigned long y) {
   unsigned r, g, b, val = x/(FONTWIDTH + 3) + y/(FONTHEIGHT + 3)*statescols;
   if (val >= maxstates) return;
   sprintf(inpbuf, "Give decimal RGB values for state %d (%d %d %d): ", val,
                            cellcolor[val].red >> 8, cellcolor[val].green >> 8,
                                                     cellcolor[val].blue >> 8);
   minbuflen = strlen(inpbuf);
   DoExpose(inputw);
   getxstring();
   if (!inpbuf[minbuflen]) {
      displaystats();
      return;
   }
   if (sscanf(inpbuf + minbuflen, "%u %u %u", &r, &g, &b) != 3 || r > 255
                                                       || g > 255 || b > 255) {
      strcpy(inpbuf, "Wrong RGB values");
      announce_and_wait(RED_EVER);
   }
   else {
      char htmlcolor[8];
      sprintf(htmlcolor, "#%02x%02x%02x", r, g, b);
      strcpy(currentcolors[val], htmlcolor);
      setonecolor(val, htmlcolor);
      xgcv.foreground = exact.pixel;
      XChangeGC(disp, cellgc[val], GCForeground, &xgcv);
      if (val == 0) {
         XSetWindowBackground(disp, lifew, cellcolor->pixel);
         XClearWindow(disp, lifew);
      }
      redisplay(MESH);
      showstates();
   }
   displaystats();
}
