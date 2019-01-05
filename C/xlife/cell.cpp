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
 * A lot of modifications were added at 2011-13 by Vladimir Lidovski vol.litwr@gmail.com
 * $Id: cell.cpp 359 2014-09-11 18:40:28Z litwr $
 * (C) This version of XLife may be used under the same conditions as mentioned above
 */

#include "defs.h"
#include "tile.h"
#include "colors.h"
#include "framebuffer.h"

cellcount_t chgpoints[MAXCOLORS], chgrects[MAXCOLORS];
XPoint points[MAXCOLORS][MAXCHANGE];
XRectangle rects[MAXCOLORS][MAXCHANGE];
/*
 * This is how we define pseudocolor mappings.  The table we want is:
 *
 *  old  new                       result
 *   0    0     (never used)
 *   0    1     born this turn       3 (normally blue)
 *   1    0     died this turn       2 (normally red)
 *   1    1     stable               1 (normally white)
 *
 * Note that when pseudocolor is enabled we have to repaint the
 * whole screen every time.  This is the only way (?) we can be
 * sure to nuke all the red squares that should turn black.
 */
int pseudopaint[2][2] = {{0, 2}, {3, 1}};

int getcell(cellbox *ptr, const int xdx, const int ydx) {
/* get state of cell xdx, ydx within box *ptr */
   if (ev_mode != VALENCE_DRIVEN)
      return ptr->nstate.cell[ydx][xdx]; /* it is also gstate */
   else if (ydx > 3)
      return (ptr->twostate.live2 & 1 << (ydx - 4)*8 + xdx) != 0;
   else
      return (ptr->twostate.live1 & 1 << ydx*8 + xdx) != 0;
}

void setcell(cellbox *ptr, const int xdx, const int ydx, const int val) {
/* set state of cell xdx, ydx within box *ptr */
    if (ev_mode != VALENCE_DRIVEN)
	ptr->nstate.cell[ydx][xdx] = val;
    else if (val) {
	    if (ydx > 3)
		ptr->twostate.live2 |= 1 << ( (ydx - 4)*8  + xdx);
	    else
		ptr->twostate.live1 |= 1 << ( ydx*8  + xdx);
	}
	else
	{
	    if (ydx > 3)
		ptr->twostate.live2 &= 0xffffffff^(1 << ( (ydx - 4)*8  + xdx));
	    else
		ptr->twostate.live1 &= 0xffffffff^(1 << ( ydx*8  + xdx));
	}
}

static void displayline_pc(u32bits line, u32bits x, u32bits y, u32bits oline) {
   register int c;
   if (scale >= 0) {
      if (c = pseudopaint[(line&1) != 0][(oline&1) != 0]) fb_ins(x, y, c);
      if (c = pseudopaint[(line&2) != 0][(oline&2) != 0]) fb_ins(x + 1, y, c);
      if (c = pseudopaint[(line&4) != 0][(oline&4) != 0]) fb_ins(x + 2, y, c);
      if (c = pseudopaint[(line&8) != 0][(oline&8) != 0]) fb_ins(x + 3, y, c);
      if (c = pseudopaint[(line&16) != 0][(oline&16) != 0]) fb_ins(x + 4, y, c);
      if (c = pseudopaint[(line&32) != 0][(oline&32) != 0]) fb_ins(x + 5, y, c);
      if (c = pseudopaint[(line&64) != 0][(oline&64) != 0]) fb_ins(x + 6, y, c);
      if (c = pseudopaint[(line&128) != 0][(oline&128) != 0]) fb_ins(x + 7, y, c);
   }
   else if (scale == -1) {
      if (c = pseudopaint[(line&3) != 0][(oline&3) != 0]) fb_ins(x, y, c);
      if (c = pseudopaint[(line&12) != 0][(oline&12) != 0]) fb_ins(x + 2, y, c);
      if (c = pseudopaint[(line&48) != 0][(oline&48) != 0]) fb_ins(x + 4, y, c);
      if (c = pseudopaint[(line&192) != 0][(oline&192) != 0]) fb_ins(x + 6, y, c);
   }
   else if (scale == -2) {
      if (c = pseudopaint[(line&15) != 0][(oline&15) != 0]) fb_ins(x, y, c);
      if (c = pseudopaint[(line&240) != 0][(oline&240) != 0]) fb_ins(x + 4, y, c);
   }
   else
      if (c = pseudopaint[(line&255) != 0][(oline&255) != 0]) fb_ins(x, y, c);
}

static void displayline(u32bits line, u32bits x, u32bits y) {
/* post changes in a half-cell to the framebuffer */
   if (scale >= 0) {
      if (line&1) fb_ins(x, y, 1);
      if (line&2) fb_ins(x + 1, y, 1);
      if (line&4) fb_ins(x + 2, y, 1);
      if (line&8) fb_ins(x + 3, y, 1);
      if (line&16) fb_ins(x + 4, y, 1);
      if (line&32) fb_ins(x + 5, y, 1);
      if (line&64) fb_ins(x + 6, y, 1);
      if (line&128) fb_ins(x + 7, y, 1);
   }
   else if (scale == -1) {
      if (line&3) fb_ins(x, y, 1);
      if (line&12) fb_ins(x + 2, y, 1);
      if (line&48) fb_ins(x + 4, y, 1);
      if (line&192) fb_ins(x + 6, y, 1);
   }
   else if (scale == -2) {
      if (line&15) fb_ins(x, y, 1);
      if (line&240) fb_ins(x + 4, y, 1);
   }
   else
      if (line&255) fb_ins(x, y, 1);
}

void displaybox(u32bits x, u32bits y, cellbox *ptr) {
/* post changes in a tile's state to the change arrays */
    register int i, j;
    if (ev_mode != VALENCE_DRIVEN) {
       register int ns;
       if (scale < 0)
          for (i = 0; i < BOXSIZE >> -scale; i++)
             for (j = 0; j < BOXSIZE >> -scale; j++) {
                register int k, l;
                for (k = 0; k < 1 << -scale; k++)
                   for (l = 0; l < 1 << -scale; l++)
                      if (ns = ptr->nstate.cell[(i << -scale) + k][(j << -scale) + l]) {
                         fb_ins(x + (j << -scale), y + (i << -scale), ns);
                         goto brk2;
                      }
brk2:
                continue;
             }
       else
          for (i = 0; i < BOXSIZE; i++)
             for (j = 0; j < BOXSIZE; j++)
                if (ns = ptr->nstate.cell[i][j])
                   fb_ins(x + j, y + i, ns);
    }
    else {
        register u32bits live1 = ptr->twostate.live1;
        register u32bits live2 = ptr->twostate.live2;
        if (pseudocolor) {
	   register u32bits olive1 = ptr->twostate.olive1;
	   register u32bits olive2 = ptr->twostate.olive2;
	   if (scale >= 0) {
	      displayline_pc(live1, x, y, olive1);
	      displayline_pc(live1>>8, x, ++y, olive1>>8);
	      displayline_pc(live1>>16, x, ++y, olive1>>16);
	      displayline_pc(live1>>24, x, ++y, olive1>>24);
	      displayline_pc(live2, x, ++y, olive2);
	      displayline_pc(live2>>8, x, ++y, olive2>>8);
	      displayline_pc(live2>>16, x, ++y, olive2>>16);
	      displayline_pc(live2>>24, x, ++y, olive2>>24);
	   }
	   else if (scale == -1) {
	      displayline_pc(live1|live1>>8, x, y, olive1|olive1>>8);
	      displayline_pc(live1>>16|live1>>24, x, y += 2, olive1>>16|olive1>>24);
	      displayline_pc(live2|live2>>8, x, y += 2, olive2|olive2>>8);
	      displayline_pc(live2>>16|live2>>24, x, y += 2, olive2>>16|olive2>>24);
	   }
	   else if (scale == -2) {
	      displayline_pc(live1|live1>>8|live1>>16|live1>>24, x, y,
	                       olive1|olive1>>8|olive1>>16|olive1>>24);
	      displayline_pc(live2|live2>>8|live2>>16|live2>>24, x, y+=4,
	                       olive2|olive2>>8|olive2>>16|olive2>>24);
	   }
	   else
              displayline_pc((live1|live2) != 0, x, y, (olive1|olive2) != 0);
        }
        else {
           if (scale >= 0) {
              displayline(live1, x, y);
              displayline(live1>>8, x, ++y);
              displayline(live1>>16, x, ++y);
              displayline(live1>>24, x, ++y);
              displayline(live2, x, ++y);
              displayline(live2>>8, x, ++y);
              displayline(live2>>16, x, ++y);
              displayline(live2>>24, x, ++y);
           }
           else if (scale == -1) {
              displayline(live1|live1>>8, x, y);
              displayline(live1>>16|live1>>24, x, y += 2);
              displayline(live2|live2>>8, x, y += 2);
              displayline(live2>>16|live2>>24, x, y += 2);
           }
           else if (scale == -2) {
              displayline(live1|live1>>8|live1>>16|live1>>24, x, y);
              displayline(live2|live2>>8|live2>>16|live2>>24, x, y += 4);
           }
           else
              displayline((live1|live2) != 0, x, y);
        }
    }
}

static void trdisplayline(u32bits line, u32bits x, u32bits y) {
/* post changes in a half-cell to the framebuffer */
   int c = wireframe ? LOAD_BOX : 1;
   if (scale >= 0) {
      if (line&1) fb_ins(Tx(x, y), Ty(x, y), c);
      if (line&2) fb_ins(Tx(x + 1, y), Ty(x + 1, y), c);
      if (line&4) fb_ins(Tx(x + 2, y), Ty(x + 2, y), c);
      if (line&8) fb_ins(Tx(x + 3, y), Ty(x + 3, y), c);
      if (line&16) fb_ins(Tx(x + 4, y), Ty(x + 4, y), c);
      if (line&32) fb_ins(Tx(x + 5, y), Ty(x + 5, y), c);
      if (line&64) fb_ins(Tx(x + 6, y), Ty(x + 6, y), c);
      if (line&128) fb_ins(Tx(x + 7, y), Ty(x + 7, y), c);
   }
   else if (scale == -1) {
      if (line&3) fb_ins(Tx(x, y), Ty(x, y), c);
      if (line&12) fb_ins(Tx(x + 2, y), Ty(x + 2, y), c);
      if (line&48) fb_ins(Tx(x + 4, y), Ty(x + 4, y), c);
      if (line&192) fb_ins(Tx(x + 6, y), Ty(x + 6, y), c);
   }
   else if (scale == -2) {
      if (line&15) fb_ins(Tx(x, y), Ty(x, y), c);
      if (line&240) fb_ins(Tx(x + 4, y), Ty(x + 4, y), c);
   }
   else
      if (line&255) fb_ins(Tx(x, y), Ty(x, y), c);
}

void trdisplaybox(u32bits x, u32bits y, cellbox *ptr) {
/* post changes in a tile's state to the change arrays */
   register int i, j;
   if (ev_mode != VALENCE_DRIVEN) {
      register int ns;
      if (scale < 0)
         for (i = 0; i < BOXSIZE >> -scale; i++)
            for (j = 0; j < BOXSIZE >> -scale; j++) {
               register int k, l;
               for (k = 0; k < 1 << -scale; k++)
                  for (l = 0; l < 1 << -scale; l++)
                     if (ns = ptr->nstate.cell[(i << -scale) + k][(j << -scale) + l]) {
                        if (wireframe) ns = LOAD_BOX;
                        fb_ins(Tx(x + (j << -scale), y + (i << -scale)), Ty(x + (j << -scale), y + (i << -scale)), ns);
                     }
            }
      else
         for (i = 0; i < BOXSIZE; i++)
            for (j = 0; j < BOXSIZE; j++)
               if (ns = ptr->nstate.cell[i][j]) {
                  if (wireframe) ns = LOAD_BOX;
                  fb_ins(Tx(x + j, y + i), Ty(x + j, y + i), ns);
               }
   }
   else {
      register u32bits live1 = ptr->twostate.live1;
      register u32bits live2 = ptr->twostate.live2;
      if (scale >= 0) {
         trdisplayline(live1, x, y);
         trdisplayline(live1>>8, x, ++y);
         trdisplayline(live1>>16, x, ++y);
         trdisplayline(live1>>24, x, ++y);
         trdisplayline(live2, x, ++y);
         trdisplayline(live2>>8, x, ++y);
         trdisplayline(live2>>16, x, ++y);
         trdisplayline(live2>>24, x, ++y);
      }
      else if (scale == -1) {
         trdisplayline(live1|live1>>8, x, y);
         trdisplayline(live1>>16|live1>>24, x, y += 2);
         trdisplayline(live2|live2>>8, x, y += 2);
         trdisplayline(live2>>16|live2>>24, x, y += 2);
      }
      else if (scale == -2) {
         trdisplayline(live1|live1>>8|live1>>16|live1>>24, x, y);
         trdisplayline(live2|live2>>8|live2>>16|live2>>24, x, y += 4);
      }
      else
        trdisplayline((live1|live2) != 0, x, y);
   }
}

#define upcst(m) if (line&m) {\
		rects[1][chgrects[1]].x = TX(x, y) << scale;\
		rects[1][chgrects[1]++].y = TY(x, y) << scale;\
	    }

static void trdisplayline_nofb(u32bits line, u32bits x, u32bits y) {
/* post changes in a half-cell to the X structure arrays */
/* use global geometric transformations */
   upcst(0x1);
   ++x;
   upcst(0x2);
   ++x;
   upcst(0x4);
   ++x;
   upcst(0x8);
   ++x;
   upcst(0x10);
   ++x;
   upcst(0x20);
   ++x;
   upcst(0x40);
   ++x;
   upcst(0x80);
}

void trdisplaybox_nofb(u32bits x, u32bits y, cellbox *ptr) {
   register int i, j;
   if (ev_mode != VALENCE_DRIVEN) {
      int ns, vc = 0;
      for (i = 0; i < BOXSIZE; i++)
         for (j = 0; j < BOXSIZE; j++) {
	    if ((ns = ptr->nstate.cell[i][j]) == 0) continue;
               rects[ns][chgrects[ns]].x = TX(x + j, y + i) << scale;
	       rects[ns][chgrects[ns]++].y = TY(x + j, y + i) << scale;
	 }
   }
   else {
      register u32bits live1 = ptr->twostate.live1;
      register u32bits live2 = ptr->twostate.live2;
      trdisplayline_nofb(live1, x, y);
      trdisplayline_nofb(live1>>8, x, ++y);
      trdisplayline_nofb(live1>>16, x, ++y);
      trdisplayline_nofb(live1>>24, x, ++y);
      trdisplayline_nofb(live2, x, ++y);
      trdisplayline_nofb(live2>>8, x, ++y);
      trdisplayline_nofb(live2>>16, x, ++y);
      trdisplayline_nofb(live2>>24, x, ++y);
   }
}

void trdrawbox(coord_t xmin, coord_t ymin, coord_t xmax, coord_t ymax, int color) {
/* box the area defined by two corners (using global transform) */
    coord_t x1, y1, x2, y2;
    x1 = xmin - STARTX;
    x2 = xmax - STARTX;
    y1 = ymin - STARTY;
    y2 = ymax - STARTY;
    xmin = tx(x1, y1, txx, txy) + loadx;
    ymin = ty(x1, y1, tyx, tyy) + loady;
    xmax = tx(x2, y2, txx, txy) + loadx;
    ymax = ty(x2, y2, tyx, tyy) + loady;
    drawbox(xmin, ymin, xmax, ymax, color);
}

/* cell.c ends here */
