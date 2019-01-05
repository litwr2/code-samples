/*
 * XLife Copyright 2013 Vladimir Lidovski vol.litwr@gmail.com
 * $Id: framebuffer.cpp 358 2014-09-10 02:27:40Z litwr $
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
#include "history.h"
#include "framebuffer.h"

struct FB *first, *pfb;

void fb_ins(register unsigned xa, register unsigned ya, unsigned c) {
   register struct FB *p;  
   if (xa < xpos || ya < ypos || (ya = RYPOS(ya)) >= height
                                       || (xa = RXPOS(xa)) >= width)
      return;
   p = pfb + maxwidth*ya + xa;
   if (c && !p->is) {
      p->old = 0;
      p->is = 1;
      if (first)
         p->next = first;
      else
         p->next = 0;
      first = p;
   }
   if (p->is) {
      p->cur = c;
      //if (!p->checked || c) p->cur = c;
      p->checked = 1;
   }
}

void fb_ins_old(unsigned xa, unsigned ya, unsigned c) {
   struct FB *p;
   if (xa < xpos || ya < ypos || (ya = RYPOS(ya)) >= height ||
                                                     (xa = RXPOS(xa)) >= width)
      return;
   p = pfb + maxwidth*ya + xa;
   if (!p->is && c) {
      p->is = 1;
      if (first)
         p->next = first;
      else
         p->next = 0;
      first = p;
   }
   if (p->is) {
      p->cur = 0;
      p->old = c;
      p->checked = 1;
   }
}

void fb_ins_big(const bigNumber &xa, const bigNumber &ya, unsigned c) {
   register struct FB *p;  
   bigNumber ya0, xa0;   
   if (xa < bigXPos || ya < bigYPos || (ya0 = RYPOSBIG(ya)) >= height
                                       || (xa0 = RXPOSBIG(xa)) >= width)
      return;
   unsigned xa1 = xa0.toUint(), ya1 = ya0.toUint();       
   p = pfb + maxwidth*ya1 + xa1;
   if (c && !p->is) {
      p->old = 0;
      p->is = 1;
      if (first)
         p->next = first;
      else
         p->next = 0;
      first = p;
   }
   if (p->is) {
      p->cur = c;
      //if (!p->checked || c) p->cur = c;
      p->checked = 1;
   }
}

static inline void putcell(struct FB *p, int c) {
   unsigned xy = p - pfb;
   unsigned x = xy%maxwidth, y = xy/maxwidth;
   if (scale > 0) {
      rects[c][chgrects[c]].x = x;
      rects[c][chgrects[c]++].y = y;
      if (chgrects[c] == MAXCHANGE) {
         XFillRectangles(disp, lifew, cellgc[c], rects[c], MAXCHANGE);
         chgrects[c] = 0;
      }
   }
   else {
      points[c][chgpoints[c]].y = y;
      points[c][chgpoints[c]++].x = x;
      if (chgpoints[c] == MAXCHANGE) {
         XDrawPoints(disp, lifew, cellgc[c], points[c], MAXCHANGE, CoordModeOrigin);
         chgpoints[c] = 0;
      }
   }
}

static void fb_close(int cmax) {
   int i;
   if (scale <= 0) {
      for (i = 0; i < cmax; i++)
         if (chgpoints[i]) {
            XDrawPoints(disp, lifew, cellgc[i], points[i], chgpoints[i],
                                                              CoordModeOrigin);
            chgpoints[i] = 0;
         }
         if (chgpoints[i = LOAD_BOX]) {
            XDrawPoints(disp, lifew, cellgc[i], points[i], chgpoints[i],
                                                              CoordModeOrigin);
            chgpoints[i] = 0;
         }
   }
   else
      for (i = 0; i < cmax; i++)
         if (chgrects[i]) {
            XFillRectangles(disp, lifew, cellgc[i], rects[i], chgrects[i]);
            chgrects[i] = 0;
         }
   if (truehistory) {
      if (scale <= 0) {
         if (chgpoints[HISTCOLOR1]) {
            XDrawPoints(disp, lifew, cellgc[HISTCOLOR1], points[HISTCOLOR1],
                                       chgpoints[HISTCOLOR1], CoordModeOrigin);
            chgpoints[HISTCOLOR1] = 0;
         }
         if (chgpoints[HISTCOLOR2]) {
            XDrawPoints(disp, lifew, cellgc[HISTCOLOR2], points[HISTCOLOR2],
                                       chgpoints[HISTCOLOR2], CoordModeOrigin);
            chgpoints[HISTCOLOR2] = 0;
         }
      }
      else {
         if (chgrects[HISTCOLOR1]) {
            XFillRectangles(disp, lifew, cellgc[HISTCOLOR1],
                                      rects[HISTCOLOR1], chgrects[HISTCOLOR1]);
            chgrects[HISTCOLOR1] = 0;
         }
         if (chgrects[HISTCOLOR2]) {
            XFillRectangles(disp, lifew, cellgc[HISTCOLOR2],
                                      rects[HISTCOLOR2], chgrects[HISTCOLOR2]);
            chgrects[HISTCOLOR2] = 0;
         }
      }
   }
}

void fb_flush(void) {
   register struct FB *p, *po;
   for (p = first; p; p = p->next)
      if (p->checked == 0) {
         if (p->cur) putcell(p, 0);
         if (p == first)
           first = p->next;
         else
           po->next = p->next;
         p->is = 0;
      }
      else {
         p->checked = 0;
         po = p;
         if (p->old != p->cur) {
            putcell(p, p->cur);
            p->old = p->cur;
         }
      }
   fb_close(maxstates + 2*(pseudocolor && ev_mode == VALENCE_DRIVEN));
}

void fb_clear(void) {
   register struct FB *p;
   for (p = first; p; p = p->next)
      p->is = 0;
   first = 0;
}

void fb_flush_rep(void) {
   register struct FB *p;
   for (p = first; p; p = p->next)
      if (p->old)
         putcell(p, p->old);
   fb_close(maxstates + 2*(pseudocolor && ev_mode == VALENCE_DRIVEN));
}

