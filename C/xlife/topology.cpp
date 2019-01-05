/*
 * XLife Copyright 2012, 2013 Vladimir Lidovski vol.litwr@gmail.com
 * $Id: topology.cpp 310 2014-02-11 07:40:09Z litwr $
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
#include "tile.h"
#include "colors.h"

static unsigned bscale(void) {
   if (scale >= 0) return 0;
   if (scale == -1) return 2;
   if (scale == -2) return 4;
   return 8;
}

static unsigned y_max_scale(int dy) {
   return RYPOS(y_max_limit + dy - bscale()) + (scale < 0);
}

static unsigned x_max_scale(int dx) {
   return RXPOS(x_max_limit + dx - bscale()) + (scale < 0);
}

void adjust_topology(int dx, int dy, int s) {
   unsigned rect0x, rect0w, rect0y, rect0h, rect1x, rect1w, rect1y, rect1h;
   int lx = x_max_limit - x_min_limit, ly = y_max_limit - y_min_limit,
      x0 = RXPOS(x_min_limit), y0 = RYPOS(y_min_limit),
      w0 = x_max_scale(0) - RXPOS(x_min_limit), w1 = w0,
      h0 = y_max_scale(0) - RYPOS(y_min_limit), h1 = h0,
      x1 = RXPOS(x_min_limit + dx), y1 = RYPOS(y_min_limit + dy);
   if (dx == 0 && dy == 0) return;
   if ((limits&1) == 0) {
      if (dy == 0) return;
      x0 = x1 = 0, w0 = w1 = width;
      h1 = h0 = RSCALE(abs(dy));
      if (dy > 0)
          y1 = y_max_scale(0);
      else
          y0 = y_max_scale(dy);
      goto DRAW;
   }
   else if ((limits&2) == 0) {
      if (dx == 0) return;
      y0 = y1 = 0, h0 = h1 = height;
      if (dx > 0)
         x1 = x_max_scale(0);
      else
         x0 = x_max_scale(dx);
      w0 = w1 = RSCALE(abs(dx));
      goto DRAW;
   }
   if (abs(dy) >= ly || abs(dx) >= lx) {
      if (x0 < 0) w0 += x0, x0 = 0;
      if (y0 < 0) h0 += y0, y0 = 0;
      if (x1 < 0) w1 += x1, x1 = 0;
      if (y1 < 0) h1 += y1, y1 = 0;
      if (x0 + w0 > width) w0 -= x0 + w0 - width;
      if (x1 + w1 > width) w1 -= x1 + w1 - width;
      if (y0 + h0 > height) h0 -= y0 + h0 - height;
      if (y1 + h1 > height) h1 -= y1 + h1 - height;
DRAW:
      if (s == 0 && h0 > 0 && w0 > 0)
      XFillRectangle(disp, lifew, *cellgc, x0, y0, w0, h0);
      else if (s && h1 > 0 && w1 > 0)
      XFillRectangle(disp, lifew, cellgc[GRID_COLOR], x1, y1, w1, h1);
      return;
   }
/* the following code is terrible */
   if (dx > 0) {
      if (xpos - dx < x_min_limit && XPOS(width) > x_min_limit) {
         rect0x = max(RXPOS(x_min_limit), 0);
         if (xpos > x_min_limit)
            rect0w = RXPOS(x_min_limit + dx);
         else
            rect0w = min(RSCALE(dx), RSCALE(XPOS(width) - x_min_limit));
      }
      else
         rect0x = rect0w = 0;
      if (xpos - dx < x_max_limit && XPOS(width) > x_max_limit - 1) {
         rect1x = max(x_max_scale(0), 0);
         if (xpos > x_max_limit)
            rect1w = x_max_scale(dx);
         else
            rect1w = min(RSCALE(dx), width - x_max_scale(0));
      }
      else
         rect1x = rect1w = 0;
   }
   else if (dx < 0) {
      if (xpos < x_max_limit && XPOS(width) - dx > x_max_limit - 1) {
         rect0x = max(x_max_scale(dx), 0);
         if (xpos - dx > x_max_limit)
            rect0w = x_max_scale(0);
         else
            rect0w = min(RSCALE(-dx), width - x_max_scale(dx));
      }
      else
         rect0x = rect0w = 0;
      if (xpos < x_min_limit && XPOS(width) - dx > x_min_limit) {
         rect1x = max(RXPOS(x_min_limit + dx), 0);
         if (xpos > x_min_limit + dx)
            rect1w = RXPOS(x_min_limit);
         else
            rect1w = min(RSCALE(-dx), RSCALE(XPOS(width) - x_min_limit - dx));
      }
      else
         rect1x = rect1w = 0;
   }
   if (dy > 0) {
      if (ypos - dy < y_min_limit && YPOS(height) > y_min_limit) {
         rect0y = max(RYPOS(y_min_limit), 0);
         if (ypos > y_min_limit)
            rect0h = RYPOS(y_min_limit + dy);
         else
            rect0h = min(RSCALE(dy), RSCALE(YPOS(height) - y_min_limit));
      }
      else
         rect0y = rect0h = 0;
      if (ypos < y_max_limit + dy && YPOS(height) > y_max_limit - 1) {
         rect1y = max(y_max_scale(0), 0);
         if (ypos > y_max_limit)
            rect1h = y_max_scale(dy);
         else
            rect1h = min(RSCALE(dy), height - y_max_scale(0));
      }
      else
         rect1y = rect1h = 0;
   }
   else if (dy < 0) {
      if (ypos < y_max_limit && YPOS(height) - dy > y_max_limit - 1) {
         rect0y = max(y_max_scale(dy), 0);
         if (ypos - dy > y_max_limit)
            rect0h = y_max_scale(0);
         else
            rect0h = min(RSCALE(-dy), height - y_max_scale(dy));
      }
      else
         rect0y = rect0h = 0;
      if (ypos < y_min_limit && YPOS(height) - dy > y_min_limit) {
         rect1y = max(RYPOS(y_min_limit + dy), 0);
         if (ypos > y_min_limit + dy)
            rect1h = RYPOS(y_min_limit);
         else
            rect1h = min(RSCALE(-dy), RSCALE(YPOS(height) - y_min_limit - dy));
      }
      else
         rect1y = rect1h = 0;
   }
   if (dy) {
      if (s == 0)
         XFillRectangle(disp, lifew, cellgc[0], x0, rect0y, w0, rect0h);
      else
         XFillRectangle(disp, lifew, cellgc[GRID_COLOR], x1, rect1y, w1, rect1h);
   }
   if (dx) {
      if (s == 0)
         XFillRectangle(disp, lifew, cellgc[0], rect0x, y0, rect0w, h0);
      else
         XFillRectangle(disp, lifew, cellgc[GRID_COLOR], rect1x, y1, rect1w, h1);
   }
   if (dx && dy) {
      if (s == 0)
         XFillRectangle(disp, lifew, cellgc[0], rect0x, rect0y, rect0w, rect0h);
      else
         XFillRectangle(disp, lifew, cellgc[GRID_COLOR], rect1x, rect1y, rect1w,
                                                                       rect1h);
   }
}

static int set_torus(coord_t x, coord_t y) {
    x = (x + 7 >> 3)*8;
    y = (y + 7 >> 3)*8;
    limits = x_min_limit = x_max_limit = y_min_limit = y_max_limit = 0;
    if (x) {
          x_min_limit = STARTX - (x - x%16)/2;
          x_max_limit = STARTX + (x + x%16)/2;
          limits = 1;
    }
    if (y) {
          y_min_limit = STARTY - (y - y%16)/2;
          y_max_limit = STARTY + (y + y%16)/2;
          limits |= 2;
    }
    return limits;
}

static void clr_tile_links(void) {
   tile *ptr = active.tiles;
   for (; ptr; ptr = ptr->next)
      ptr->up = ptr->dn = ptr->lf = ptr-> rt = 0;
}

int set_topology(char *p) {
       char s1[20], s2[20], t, c[2];
       int x, y, dx = 0, dy = 0;

       clr_tile_links();
       if (*p == 'Q')
l1:
          return topology = 'Q', set_torus(0, 0);
       if (*p == 'P' || *p == 'T') {
          t = *p++;
          if (*p == 0) return topology = t, 0;
       }
       else if ((t = topology) == 'Q')
          return 12;
       if (strlen(p) < 2) return 8;
       if (sscanf(p, "%[0-9+-] %*[,*xX] %[0-9+-]", s1, s2) == 2) {
          if (sscanf(s1, "%d %[+-] %d", &x, c, &dx) == 3) {
             if (*c == '-') dx *= -1;
          }
          else if (sscanf(s1, "%d", &x) != 1)
             return 5;
          if (sscanf(s2, "%d %[+-] %d", &y, c, &dy) == 3) {
             if (*c == '-') dy *= -1;
          }
          else if (sscanf(s2, "%d", &y) != 1) return 6;
       }
       else if (sscanf(p, "%*[,*xX] %[0-9+-]", s1) == 1) {
          if (sscanf(s1, "%d %[+-] %d", &y, c, &dy) == 3) {
             if (*c == '-') dy *= -1;
          }
          else if (sscanf(s1, "%d", &y) != 1)
             return 5;
          x = x_max_limit - x_min_limit;
       }
       else if (sscanf(p, "%[0-9+-] %*[,*xX]", s1) == 1) {
          if (sscanf(s1, "%d %[+-] %d", &x, c, &dx) == 3) {
             if (*c == '-') dx *= -1;
          }
          else if (sscanf(s1, "%d", &x) != 1)
             return 4;
          y = y_max_limit - y_min_limit;
       }
       else
          return 11;
       if (x == 0 && y == 0)
          goto l1;
       if (set_torus(x, y)) {
          switch (t) {
          case 'P':
            if (dx != 0 || dy != 0) return 7;
            break;
          case 'T':
            if (dx != 0 || dy != 0) {
               strcpy(inpbuf, "Torus side shift is not supported");
               return 8;
            }
          }
          topology = t;
          if (x%8 != 0 || y%8 != 0) {
               sprintf(inpbuf, "Bounded grid size will be rounded to %dx%d",
                       x_max_limit - x_min_limit, y_max_limit - y_min_limit);
               announce_and_wait(GREEN_EVER);
          }
       }
       return 0;
}

#define conf_y()\
      if (ypos < y_min_limit) {\
         y1 = RYPOS(y_min_limit);\
         if (YPOS(height) > y_max_limit)\
            y2 = y_max_scale(0) - y1;\
         else\
            y2 = height - RYPOS(y_min_limit);\
      }\
      else {\
         y1 = 0;\
         if (YPOS(height) > y_max_limit)\
            y2 = y_max_scale(0);\
         else\
            y2 = height;\
      }\
      XFillRectangle(disp, lifew, cellgc[GRID_COLOR], x1, y1, x2, y2);


void drawboundedgrid(void) {
   if (limits & 2) {
      int x1 = 0, y1, x2, y2;
      if (ypos < y_min_limit)
         if (height <= RYPOS(y_min_limit))
            goto FILLALL;
         else
            XFillRectangle(disp, lifew, cellgc[GRID_COLOR], 0, 0, width,
                                                           RYPOS(y_min_limit));
      if (YPOS(height) > y_max_limit)
         if (y_max_limit > ypos)
            XFillRectangle(disp, lifew, cellgc[GRID_COLOR], 0, y_max_scale(0),
                                                width, height - y_max_scale(0));
         else
            goto FILLALL;
      if ((limits & 1) && YPOS(height) > y_min_limit && ypos < y_max_limit) {
         if (xpos < x_min_limit) {
            if (XPOS(width) < x_min_limit)
               x2 = width;
            else
               x2 = RXPOS(x_min_limit);
            conf_y();
         }
         if (XPOS(width) + 1 > x_max_limit) {
            if (xpos > x_max_limit)
               x2 = width;
            else {
               x1 = x_max_scale(0);
               x2 = width - x_max_scale(0);
            }
            conf_y();
         }
      }
   }
   else if (limits) {
      if (xpos < x_min_limit)
         XFillRectangle(disp, lifew, cellgc[GRID_COLOR], 0, 0,
                                       min(width, RYPOS(x_min_limit)), height);
      if (XPOS(width) > x_max_limit)
         if (x_max_limit > xpos)
            XFillRectangle(disp, lifew, cellgc[GRID_COLOR], x_max_scale(0), 0,
                                                width - x_max_scale(0), height);
         else
FILLALL:
            XFillRectangle(disp, lifew, cellgc[GRID_COLOR], 0, 0, width,
                                                                       height);
   }
}
