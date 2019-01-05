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
 * A lot of modifications were added at 2001, 2011-13 by Vladimir Lidovski vol.litwr@gmail.com
 * (C) This version of XLife may be used under the same conditions as mentioned above
 * $Id: tentative.cpp 346 2014-04-26 12:09:12Z qiray $
 */

#include <stddef.h>     /* we need offsetof() */
#include "defs.h"
#include "tile.h"

void prep_tentative(void) {
   if (tentative.tiles) {
      drawpivot();
      if (!wireframe) boxpattern(0);
   }
}

static void tentative_move(void) {
   if (topology != 'Q' || scale == 1 && wireframe)
      redraw_lifew();
   else {
      if  (scale > 1 && !dispmesh && wireframe)
         griddisplay(0);
      redisplay(MESH);
   }
}

void moveload(void) {
/* Move loaded pattern to active cell */
   prep_tentative();
   loadx = XPOS(event.xmotion.x);
   loady = YPOS(event.xmotion.y);
   if (hashmode) {
      hash_moveload(XPOSBIG(event.xmotion.x), YPOSBIG(event.xmotion.y));
      return;
   }
   tentative_move();
}

void turnload(void) {
/* Turn loaded pattern 90 degrees about its origin */
   int t;
   prep_tentative();
   t = -tyx; tyx = txx; txx = t;
   t = -tyy; tyy = txy; txy = t;
   tentative_move();
} 

void flipload(void) {
/* Flip pattern about its x axis */
   prep_tentative();
   tyx = -tyx;
   tyy = -tyy;
   tentative_move();
} 

void boxpattern(int color) {
/* Draw a bounding box in a given color around a tentative pattern */
   if (color) bounding_box(&tentative);
   trdrawbox(tentative.xmin, tentative.ymin, tentative.xmax, tentative.ymax, color);
}

void make_tentative(coord_t x1, coord_t y1, coord_t x2, coord_t y2) {
/* make all active cells in a bounding box tentative */
    coord_t xmin, ymin, xmax, ymax;
    tile *ptr;
    int dx, dy, ddx, ddy, f = 1;
    erasebox(x1, y1, x2, y2);
    xmin = min(x1, x2);
    ymin = min(y1, y2);
    xmax = max(x1, x2);
    ymax = max(y1, y2);
    txx = tyy = 1;
    txy = tyx = 0;
    initcells(&tentative);
    for (ptr = active.tiles; ptr; ptr = ptr->next) {
	int	state;
	if (ptr->dead)
	    continue;
	else if (ptr->x + BOXSIZE < xmin || ptr->y + BOXSIZE < ymin
                                          || ptr->x > xmax || ptr->y > ymax)
	    continue;
	for (dx = 0; dx < BOXSIZE; dx++)
	    for (dy = 0; dy < BOXSIZE; dy++)
		if (ptr->x + dx < xmin || ptr->y + dy < ymin 
			        || ptr->x + dx > xmax || ptr->y + dy > ymax)
		    continue;
		else if (state = getcell(&ptr->cells, dx, dy)) {
		    setcell(&ptr->cells, dx, dy, 0);
                    active.cellcount[state]--;
		    if (f) {
		        f = 0;
			iloadx = loadx = ptr->x + dx;
			ddx = STARTX - loadx;
			iloady = loady = ptr->y + dy;
			ddy = STARTY - loady;
		    }
		    chgcell(&tentative, ptr->x + dx + ddx, ptr->y + dy + ddy, state);
		}
    }
}

void copy_tentative(void) {
   tile *ptr;
   coord_t dx, dy, x, y;  
/* officially add loaded cells with given transformations */
   for (ptr = tentative.tiles; ptr; ptr = ptr->next) {
      int state;
      if (!ptr->dead)
         for (dx = 0; dx < BOXSIZE; dx++)
            for (dy = 0; dy < BOXSIZE; dy++)
               if (state = getcell(&ptr->cells, dx, dy)) {
                  x = ptr->x + dx - STARTX;
                  y = ptr->y + dy - STARTY;
                  if (!limits || chk_limits(Tx(x, y), Ty(x, y)))
                     chgcell(&active, Tx(x, y), Ty(x, y), state);
               }
   } 
}

static int tiledatasize(void) {
   if (ev_mode == VALENCE_DRIVEN)
      return 2*sizeof(u32bits);
   return sizeof(cell_t[BOXSIZE][BOXSIZE]);
}

void copypattern(pattern *from, pattern *to) {
   tile *ptr;
   int sz = tiledatasize();
   clear_pattern(to);
   for (ptr = from->tiles; ptr; ptr = ptr->next)
      if (!ptr->dead)
         memcpy(&maketile(to, ptr->x, ptr->y)->cells, &ptr->cells, sz);
}

int comparepatterns(pattern *from, pattern *to) {
   tile *ptr;
   int sz = tiledatasize();
   for (ptr = from->tiles; ptr; ptr = ptr->next)
      if (memcmp(&maketile(to, ptr->x, ptr->y)->cells, &ptr->cells, sz))
         return 0;
   for (ptr = to->tiles; ptr; ptr = ptr->next)
      if (!fetchtile(from, ptr->x, ptr->y) && memcmp(&maketile(from, ptr->x, ptr->y)->cells, &ptr->cells, sz))
         return 0;
   return 1;
}

void change_tentative_color(void) {
   tile *ptr;
   unsigned dx, dy, state;
   prep_tentative();
   for (ptr = tentative.tiles; ptr; ptr = ptr->next) {
      if (!ptr->dead)
         for (dx = 0; dx < BOXSIZE; dx++)
            for (dy = 0; dy < BOXSIZE; dy++)
               if ((state = getcell(&ptr->cells, dx, dy)) == 1)
                  setcell(&ptr->cells, dx, dy, paintcolor);
   }
   redisplay(MESH);
}

/* tentative.c ends here */
