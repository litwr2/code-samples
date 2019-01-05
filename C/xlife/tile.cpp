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
 * $Id: tile.cpp 364 2018-03-31 08:57:04Z litwr $
 */

/*
 * This module encapsulates cell-box handling. Everything else in the program
 * except possibly the evolution function should go through these functions
 * to change the board state. Entry points are:
 *
 * void initcells(context)        -- initialize active pattern
 *
 * int lookcell(context, x, y)    -- get state of cell
 *
 * void chgcell(context, x, y, c) -- set cell to state c
 *
 * void changesize(n)             -- change the cellbox allocation size
 *
 * void center()                  -- center viewport on barycenter of pattern
 *
 * void clear_all()               -- clear the universe, freeing all storage
 *
 * void redisplay(int mode)        -- update visible display of active board
 *
 * void drawboundedgrid(void)
 *
 * void saveall()                 -- save entire board state to file
 *
 * This code knows that cells are packed in 8x8 tiles on a hash list, but
 * it doesn't know anything about the internals of cell representation within
 * cell boxes.  The function changesize() changes the size of cellboxes.
 *
 * This code relies on the existence of a triad of functions:
 *
 * getcell(ptr, dx, dy)      -- get state of cell at x, y in box *ptr
 * setcell(ptr, dx, dy, val) -- set cell at x, y in box *ptr to state val
 *
 * to access cell state. It also relies on the existence of these functions:
 *
 * displaybox(x, y, ptr)     -- post box state to Xrect arrays
 * fetchtile(context, x, y)  -- get tile containing cell (x, y), NULL if none
 * maketile(context, x, y)   -- get tile holding cell (x, y), creating if none
 */

#include <stddef.h>	/* we need offsetof() */
#include <stdlib.h>
#include "defs.h"
#include "patchlevel.h"
#include "colors.h"
#include "tile.h"
#include "file.h"
#include "framebuffer.h"
#include "history.h"
#include "topology.h"

#define MARK		'*'
#define SPACE		'.'
#define	USL_MAX		4294967295U	/* max decimal value of "unsigned" */

/* public variables */
pattern active, tentative, oscillator_check;
static tile *freep, *curtile;
static XTextProperty windowName;
static char window_name_pattern[91], *window_name = window_name_pattern;

#ifdef HASHARRAY
void hashstat(pattern *p) {
   unsigned hashsum = 0, hashcount = 0, i;
   tile *ptr;
   for (i = 0; i < HASHSIZE; i++)
      if (ptr = p->hashlist[i]) {
         do hashsum++; while (ptr = ptr->hnext);
         hashcount++;
      }
   sprintf(inpbuf, "Hash %s: X=%u Y=%u [%u] filled=%.4f badness=%.3f tiles=%ld",
                                             p == &active? "active": "tentative",
         HASHBITS - 3 - p->hxs, 3 + p->hxs, HASHBITS, (double)hashcount/HASHSIZE,
                        hashcount ? (double)hashsum/hashcount : 0, p->tilecount);
}

static void fixhash(pattern *p) {
   tile *ptr;
   memset(p->hashlist, 0, HASHSIZE*sizeof(tile*));
   for (ptr = p->tiles; ptr; ptr = ptr->next) {
      unsigned hv = HASH(ptr->x, ptr->y, p->hxm, p->hxs, p->hym);
      if (ptr->hnext = p->hashlist[hv])
         ptr->hnext->hfore = ptr;
      p->hashlist[hv] = ptr;
   }
}

static void inithash(pattern *p) {
   if (HASHBITS/2 - 3 != p->hxs) {
      p->hxs = HASHBITS/2 - 3;
      p->hxm = ((1 << HASHBITS - HASHBITS/2) - 1) << 3,
      p->hym = (1 << HASHBITS/2) - 1;
      fixhash(p);
   }
}

void adjhash(pattern *p) {
   if (p->tilecount < 16 << HASHBITS/2)
      inithash(p);
   else {
      unsigned xc = 0, yc = 0;
      bounding_box(p);
      while (p->xmax - p->xmin >> 3 + xc++);
      while (p->ymax - p->ymin >> 3 + yc++);
      xc = HASHBITS/(1 + (double)yc/xc);
      if (xc < 4)
         xc = 4;
      else if (xc > HASHBITS - 4)
         xc = HASHBITS - 4;
      yc = HASHBITS - xc;
      if (p->hxs != yc - 3) {
         p->hxs = yc - 3;
         p->hxm = ((1 << xc) - 1) << 3,
         p->hym = (1 << yc) - 1;
         fixhash(p);
      }
   }
   if (truehistory)
      fixhistoryhash();
}
#endif

void setscale(const int sc) { /* preset scale for X rectangles describing active board */
    int min = 1, max = 1;
    if (sc > 0) {
	int	i, j, size = (1 << sc) - (sc > 1);
	for (i = 0; i < MAXCOLORS; i++)
	    for (j = 0; j < MAXCHANGE; j++) {
		rects[i][j].height = size;
		rects[i][j].width = size;
	    }
    }
    if (sc < 0)
       min = 1 << -sc;
    else
       max = 1 << sc;
    bigNumber minBig(sc < 0 ? (1 << (bigNumber)abs(sc)) : 1);
    if (hashmode)
       sprintf(window_name_pattern, "XLife v%d.%d.%d: a cellular-automaton laboratory [%s:%d]",
       majorversion, minorversion, patchlevel, minBig.toString(20, 0).c_str(), max);
    else
       sprintf(window_name_pattern, "XLife v%d.%d.%d: a cellular-automaton laboratory [%d:%d]",
       majorversion, minorversion, patchlevel, min, max);       
    if (XStringListToTextProperty(&window_name, 1, &windowName) == 0) {
       fprintf(stderr, "structure allocation for WindowName failed.\n");
       exit(5);
    }
    XSetWMName(disp, mainw, &windowName);
}

void initcells(pattern *context) {
#ifdef HASHARRAY
   memset(context, '\0', sizeof(pattern));
   inithash(context);
#endif
#if defined(BTREE) || defined(UNORDEREDMAP)
    context->tiles = 0;
    context->tilecount = context->chgcount = context->generations = context->osc1 = context->osc2
       = context->osc3 = context->osc4 = context->osc5 = context->xmax = context->xmin = context->ymax 
       = context->ymin = context->cellmass;
    for (int i = 0; i < MAXSTATES; i++)
       context->cellcount[i] = 0;
    context->tilemap.clear();
    context->pattern_name = 0;
#endif
}

int chgcell(pattern *context, coord_t x, coord_t y, cell_t c) {
/* turn a cell to state ON, return cell change status */
   if (hashmode) {
      return changeCellNode(x, y, c, 0);
   }
   tile *ptr = maketile(context, x, y);
   unsigned ydx = y - ptr->y, xdx = x - ptr->x;
   cell_t oldstate;
   if (c) ptr->dead = 0;
   oldstate = getcell(&ptr->cells, xdx, ydx);
   if (truehistory && c && context == &active) add2history(ptr);
   if (c != oldstate) {
      if (c)
         context->cellcount[c]++;
      setcell(&ptr->cells, xdx, ydx, c);
      context->cellcount[oldstate]--;
      return 1;
   }
   return 0;
}

void changesize(int newsize) { /* change the tile allocation size */
    if (tilesize != offsetof(tile, cells) + newsize) {
	tile	*ptr, *nptr;
        clear_pattern(&active); /* clear all patterns */
	for (ptr = freep; ptr; ptr = nptr) {
	    nptr = ptr->next;
	    free(ptr);
	}
	freep = 0;
	tilesize = offsetof(tile, cells) + newsize;
        redraw_lifew();
    }
}

#define checktore() if (topology == 'T' && context == &active) {\
         if (x >= x_max_limit)\
            x = x - x_max_limit + x_min_limit;\
         else if (x < x_min_limit)\
            x = x - x_min_limit + x_max_limit;\
         if (y >= y_max_limit)\
            y = y - y_max_limit + y_min_limit;\
         else if (y < y_min_limit)\
            y = y - y_min_limit + y_max_limit;\
    }

#ifdef HASHARRAY
void killtile(pattern *context, tile *ptr) {
/* nuke a tile, reclaiming all associated storage and fixing neighbor links */
    unsigned hv = HASH(ptr->x, ptr->y, context->hxm, context->hxs, context->hym);
    if (ptr != context->tiles)
	ptr->fore->next = ptr->next;
    else
	context->tiles = ptr->next;
    if (ptr == context->hashlist[hv])
	context->hashlist[hv] = ptr->hnext;
    else
	ptr->hfore->hnext = ptr->hnext;
    if (ptr->next) ptr->next->fore = ptr->fore;
    if (ptr->hnext) ptr->hnext->hfore = ptr->hfore;
    if (ptr->rt) ptr->rt->lf = 0;
    if (ptr->lf) ptr->lf->rt = 0;
    if (ptr->up) ptr->up->dn = 0;
    if (ptr->dn) ptr->dn->up = 0;
    ptr->next = freep;
    freep = ptr;
    context->tilecount--;
}

static tile *createtile(pattern *context, coord_t x, coord_t y, unsigned hv) {
/* create a tile to hold cells near life-world coordinates x, y */
/* universe it belongs in */
/* associated life-world coordinates */
/* tile-retrieval hash value */
    tile *ptr;

    if (freep == 0) {
	if ((ptr = (tile*)malloc(tilesize)) == 0)
	    fatal("create: malloc error: ");
    }
    else {
	ptr = freep;
	freep = freep->next;
    }
    memset(ptr, '\0', tilesize);
    ptr->next = context->tiles;
    context->tiles = ptr;

    if (ptr->next)
	ptr->next->fore = ptr;
    ptr->hnext = context->hashlist[hv];
    context->hashlist[hv] = ptr;
    if (ptr->hnext)
	ptr->hnext->hfore = ptr;
    ptr->x = x;
    ptr->y = y;
    context->tilecount++;
    return ptr;
}

static tile *findtile(pattern *context, coord_t x, coord_t y, unsigned hv) {
   tile *ptr = context->hashlist[hv];
   while (ptr) {
      if (ptr->x == x && ptr->y == y)
         return ptr;
      ptr = ptr->hnext;
   }
   return 0;
}

int lookcell(pattern *context, coord_t x, coord_t y) {
/* get the state of a cell from its world coordinates */
   if (hashmode)
      return changeCellNode(x, y, 0, 1);
   tile *ptr;
   coord_t hx = x & XMASK, hy = y & YMASK;
   unsigned hv = HASH(hx, hy, context->hxm, context->hxs, context->hym);
   if (ptr = findtile(context, hx, hy, hv))
      return getcell(&ptr->cells, x - hx, y - hy);
   else
      return 0;
}

tile *fetchtile(pattern *context, coord_t x, coord_t y) {
    checktore();
    return findtile(context, x, y, HASH(x, y, context->hxm, context->hxs, context->hym));
}

tile *maketile(pattern *context, coord_t x, coord_t y) {
    tile *ptr;
    unsigned hv;
    checktore();
    x &= XMASK;
    y &= YMASK;
    hv = HASH(x, y, context->hxm, context->hxs, context->hym);
    if (ptr = findtile(context, x, y, hv))
       return ptr;
    else
       return createtile(context, x, y, hv);
}
#elif defined(BTREE) || defined(UNORDEREDMAP)
void adjhash(pattern*) {}

void killtile(pattern *context, tile *ptr) {
/* nuke a tile, reclaiming all associated storage and fixing neighbor links */
    if (ptr != context->tiles)
	ptr->fore->next = ptr->next;
    else
	context->tiles = ptr->next;
    if (context->tilemap.find(make_pair(ptr->x, ptr->y)) != context->tilemap.end())
	 context->tilemap.erase(make_pair(ptr->x, ptr->y));
    if (ptr->next) ptr->next->fore = ptr->fore;
    if (ptr->rt) ptr->rt->lf = 0;
    if (ptr->lf) ptr->lf->rt = 0;
    if (ptr->up) ptr->up->dn = 0;
    if (ptr->dn) ptr->dn->up = 0;
    ptr->next = freep;
    freep = ptr;
    context->tilecount--;
}

static tile *createtile(pattern *context, coord_t x, coord_t y) {
/* create a tile to hold cells near life-world coordinates x, y */
/* universe it belongs in */
/* associated life-world coordinates */
/* tile-retrieval hash value */
    tile *ptr;

    if (freep == 0) {
	if ((ptr = (tile*)malloc(tilesize)) == 0)
	    fatal("create: malloc error! ");
    }
    else {
	ptr = freep;
	freep = freep->next;
    }
    memset(ptr, '\0', tilesize);
    ptr->next = context->tiles;
    context->tiles = ptr;

    if (ptr->next)
	ptr->next->fore = ptr;

    context->tilemap[make_pair(x, y)] = ptr;
/*if (context->tilemap.find(make_pair(x, y)) == context->tilemap.end())
   context->tilemap.insert(make_pair(make_pair(x, y), ptr));
else
   printf("no!\n");*/
    ptr->x = x;
    ptr->y = y;
    context->tilecount++;
    return ptr;
}

static tile *findtile(pattern *context, coord_t x, coord_t y) {
   if (context->tilemap.find(make_pair(x,y)) == context->tilemap.end())
      return 0;
   return context->tilemap.find(make_pair(x,y))->second;
}

int lookcell(pattern *context, coord_t x, coord_t y) {
/* get the state of a cell from its world coordinates */
   if (hashmode)
      return changeCellNode(x, y, 0, 1);
   tile *ptr;
   coord_t hx = x & XMASK, hy = y & YMASK;
   if (ptr = findtile(context, hx, hy))
      return getcell(&ptr->cells, x - hx, y - hy);
   else
      return 0;
}

tile *fetchtile(pattern *context, coord_t x, coord_t y) {
    checktore();
    return findtile(context, x, y);
}

tile *maketile(pattern *context, coord_t x, coord_t y) {
    tile *ptr;
    checktore();
    x &= XMASK;
    y &= YMASK;
    if (ptr = findtile(context, x, y))
       return ptr;
    else
       return createtile(context, x, y);
}
#endif

void center(void) {
/* center the display on the `center of mass' of the live boxes */
   double x = 0, y = 0;
   int ctr = 0;
   tile *ptr;
   for (ptr = active.tiles; ptr; ptr = ptr->next)
      if (!ptr->dead) {
         x += ptr->x;
         y += ptr->y;
         ctr++;
      }
   if (ctr > 0) {
      x /= ctr;
      y /= ctr;
   }
   else
      x = xorigin, y = yorigin;
   xpos = x - SCALE(width/2);
   ypos = y - SCALE(height/2);
}

void display_move(int force, int dx, int dy) {
#if VFREQ != 0
   prev_vsync.tv_sec--;
#endif
   if ((tentative.tiles || force) && wireframe && scale == 1)
      redraw_lifew();
   else if ((tentative.tiles || force) && wireframe && scale > 1 && !dispmesh) {
      griddisplay(0);
      redisplay(MESH);
   }
   else if (topology != 'Q' && (dx || dy)) {
      adjust_topology(dx, dy, 0);
      if (tentative.tiles && wireframe) 
         redisplay(MESH);
      else
         redisplay(0); 
      adjust_topology(dx, dy, 1);
      if (!tentative.tiles || !wireframe) {
         if (dispmesh && scale > 1) {
            griddisplay(1);
            if (tentative.tiles && !wireframe)
               boxpattern(CYAN_EVER);
         }
      }
   }
   else
      redisplay(tentative.tiles || force);
}

static int cmpnum(const void *e1, const void *e2) {
   if (*(coord_t*)e1 < *(coord_t*)e2)
      return -1;
   return *(coord_t*)e1 > *(coord_t*)e2;
}

void median(void) {
/* return the median coordinates of the live cells in the context */
   coord_t *coordxlist, *coordylist;
   int ctr = 0, x = xorigin, y = yorigin;
   tile *ptr;
   if (!(coordxlist = (coord_t*)malloc(sizeof(coord_t)*active.tilecount)))
      return;
   if (!(coordylist = (coord_t*)malloc(sizeof(coord_t)*active.tilecount))) {
      free(coordxlist);
      return;
   }
   for (ptr = active.tiles; ptr; ptr = ptr->next) {
      if (!ptr->dead) {
         coordxlist[ctr] = ptr->x;
         coordylist[ctr++] = ptr->y;
      }
   }
   if (ctr > 0) {
      qsort(coordxlist, ctr, sizeof(coord_t), cmpnum);
      qsort(coordylist, ctr, sizeof(coord_t), cmpnum);
      x = coordxlist[ctr/2];
      y = coordylist[ctr/2];
   }
   free(coordylist);
   free(coordxlist);
   xpos = x - SCALE(width/2);
   ypos = y - SCALE(height/2);
}

void current_tile(void) {
   if (curtile == 0)
      curtile = active.tiles;
   if (curtile) {
      coord_t xnew, ynew;
      int lflag = 0;
      xnew = curtile->x - SCALE(width/2);
      ynew = curtile->y - SCALE(height/2);
      while (curtile->dead || labs((long)xnew - (long)xpos) < 10 << 6 - scale
                          && labs((long)ynew - (long)ypos) < 10 << 6 - scale) {
         curtile = curtile->next;
         if (curtile == 0) {
            if (lflag) return;
            curtile = active.tiles;
            lflag++;
         }
         xnew = curtile->x - SCALE(width/2);
         ynew = curtile->y - SCALE(height/2);
      }
      xpos = xnew;
      ypos = ynew;
      curtile = curtile->next;
   }
}

cellcount_t cellcnt(pattern *pp) {
   int i;
   cellcount_t sum = 0;
   for (i = 1; i < maxstates; i++)
      sum += pp->cellcount[i];
   return sum;
}

void displaystats(void) {
   extern unsigned threads;
   if (hashmode) {
      setStrBuf();
      sprintf(inpbuf, strBuf);
   } else if (shortText) {
      sprintf(inpbuf, "G%lu", active.generations);
      if (active.cellmass) strcat(inpbuf, "*");
      if (dispboxes)
         sprintf(inpbuf + strlen(inpbuf), " C%u", cellcnt(&active));
      else
         sprintf(inpbuf + strlen(inpbuf), " T%u", active.tilecount);
      if (dispchanges)
         sprintf(inpbuf + strlen(inpbuf), " C%u", active.chgcount);
      if (dispspeed)
         sprintf(inpbuf + strlen(inpbuf), " S%u", speed);
   }
   else {
      sprintf(inpbuf, "%u Generations: %lu", threads, active.generations);
      if (active.cellmass) strcat(inpbuf, "*");
      if (dispboxes)
         sprintf(inpbuf + strlen(inpbuf), ", Cells: %u", cellcnt(&active));
      else
         sprintf(inpbuf + strlen(inpbuf), ", Tiles: %u", active.tilecount);
      if (dispchanges)
         sprintf(inpbuf + strlen(inpbuf), ", Changes: %u", active.chgcount);
      if (dispspeed)
         sprintf(inpbuf + strlen(inpbuf), ", Speed: %u", speed);
   }
   DoExpose(inputw);
}

void clear_pattern(pattern *pp) {
/* free given pattern */
   tile *ptr = pp->tiles, *nptr;
   while (ptr) {
      nptr = ptr->next;
      killtile(pp, ptr);
      ptr = nptr;
   }
   initcells(pp);
}

void clear_all(void) {
/* clear the board or tentative pattern, freeing all storage */
   if (tentative.tiles)
      clear_pattern(&tentative);
   clear_pattern(&active);
   redraw_lifew();
}

static int fb_sync() {
#if VFREQ == 0
   return 1;
#else
   struct timeval cur;
   gettimeofday(&cur, 0);
   if ((cur.tv_sec - prev_vsync.tv_sec)*1000000 + cur.tv_usec
                                        - prev_vsync.tv_usec > 1000000/VFREQ) {
      prev_vsync = cur;
      return 1;
   } 
   else if (cur.tv_sec < prev_vsync.tv_sec)
      prev_vsync = cur;
   else if (cur.tv_sec < prev_vsync.tv_sec)
      prev_vsync = cur;
   return state == STOP;
#endif
}

static int ascale() {
   if (scale >= 0)
      return 8;
   else if (scale <= -3)
      return 1;
   else if (scale == -1)
      return 7;
   return 5;
}

int onscreen(coord_t x, coord_t y) {
   /*this is a bit complicated due to the big torus calculations */
   int px_min = x > xpos - ascale(), py_min = y > ypos - ascale(),
      px_max = x < xpos + SCALE(width), py_max = y < ypos + SCALE(height);
   return (px_min && px_max
      || (px_min || px_max) && (xpos - ascale() > xpos + SCALE(width)))
      && (py_min && py_max
      || (py_min || py_max) && (ypos - ascale() > ypos + SCALE(height)));
}

void griddisplay(int f) {
   coord_t cx = width >> scale, cy = height >> scale;
   int small = 1 << scale, x, y;
   for (x = 0; x <= cx; x++)
      if (x%5 == 0)
         XDrawLine(disp, lifew, cellgc[GRID_ECOLOR*f], x*small - 1, 0, x*small - 1, height);
      else
         XDrawLine(disp, lifew, cellgc[GRID_COLOR*f], x*small - 1, 0, x*small - 1, height);
   for (y = 0; y <= cy; y++) {
      if (y%5 == 0)
         XDrawLine(disp, lifew, cellgc[GRID_ECOLOR*f], 0, y*small - 1, width, y*small - 1);
      else
         XDrawLine(disp, lifew, cellgc[GRID_COLOR*f], 0, y*small - 1, width, y*small - 1);
   }
}

static void tentative_fb_update(void) {
   tile *ptr;
   coord_t y, tentx, tenty, ns;
   if (tentative.tiles && (!wireframe || scale < 1)) {
      /* draw tentative points with appropriate transformation */
      for (ptr = tentative.tiles; ptr; ptr = ptr->next) {
         tile *nptr;
         tentx = ptr->x - STARTX;
         tenty = ptr->y - STARTY;
         if (scale < -3) {
            if (ev_mode == VALENCE_DRIVEN) {
               if (ptr->cells.twostate.live1 | ptr->cells.twostate.live2)
                  fb_ins(Tx(tentx, tenty), Ty(tentx, tenty), wireframe ? LOAD_BOX : 1);
            }
            else {
               for (y = 0; y < 8; y++)
                  if (ptr->cells.nstate.live[y][0]) {
                      if (ns = ptr->cells.nstate.cell[y][0]);
                      else if (ns = ptr->cells.nstate.cell[y][1]);
                      else if (ns = ptr->cells.nstate.cell[y][2]);
                      else ns = ptr->cells.nstate.cell[y][3];
                      fb_ins(Tx(tentx, tenty), Ty(tentx, tenty), wireframe ? LOAD_BOX : ns);
                      break;
                  }
                  else if (ptr->cells.nstate.live[y][1]) {
                      if (ns = ptr->cells.nstate.cell[y][4]);
                      else if (ns = ptr->cells.nstate.cell[y][5]);
                      else if (ns = ptr->cells.nstate.cell[y][6]);
                      else ns = ptr->cells.nstate.cell[y][7];
                      fb_ins(Tx(tentx, tenty), Ty(tentx, tenty), wireframe ? LOAD_BOX : ns);
                      break;
                  }
            }
         }
         else if (onscreen(Tx(tentx,tenty), Ty(tentx,tenty)))
            trdisplaybox(tentx, tenty, &ptr->cells);
      }
   }
}

static void tile_fb_update(void) {
   tile *ptr;
   coord_t x, y, tentx, tenty, ns;
   int curstate;
   if (truehistory) {
      historytile *ptr;
      for (curstate = 1; curstate >= 0; curstate--)
         for (ptr = history[curstate].tiles; ptr; ptr = ptr->next) {
            x = ptr->x;
            y = ptr->y;
            if (scale < -3)
               fb_ins(x, y, histpaint[curstate]);
            else if (onscreen(x, y))
               displayhistorybox(x, y, ptr, curstate);
         }
   }
   for (ptr = active.tiles; ptr; ptr = ptr->next) {
      x = ptr->x;
      y = ptr->y;
      if (scale < -3) {
         if (ev_mode == VALENCE_DRIVEN) {
            if (ptr->cells.twostate.live1 | ptr->cells.twostate.live2)
               fb_ins(x, y, 1);
         }
         else {
            for (int i = 0; i < 8; i++)
               if (ptr->cells.nstate.live[i][0]) {
                  if (ns = ptr->cells.nstate.cell[i][0]);
                  else if (ns = ptr->cells.nstate.cell[i][1]);
                  else if (ns = ptr->cells.nstate.cell[i][2]);
                  else ns = ptr->cells.nstate.cell[i][3];
                  fb_ins(x, y, ns);
                  break;
               }
               else if (ptr->cells.nstate.live[i][1]) {
                  if (ns = ptr->cells.nstate.cell[i][4]);
                  else if (ns = ptr->cells.nstate.cell[i][5]);
                  else if (ns = ptr->cells.nstate.cell[i][6]);
                  else ns = ptr->cells.nstate.cell[i][7];
                  fb_ins(x, y, ns);
                  break;
               }
         }
      }
      else if (onscreen(x, y))
         displaybox(x, y, &ptr->cells);
   }
   tentative_fb_update();
}

void redisplay(int mode) {
/* re-display the visible part of the board*/
   tile *ptr;
   coord_t x, y, tentx, tenty;
   int curstate, q;
   if (state == HIDE) return;
   if (dispmesh && scale > 1 && (mode & MESH)) griddisplay(1);
   if (!fb_sync()) return;
   if (tentative.tiles && pivot) { /* remove pivot of tentative points */
      if (!wireframe) boxpattern(CYAN_EVER);
      drawpivot();
   }
   if (mode & EXPOSE)
      fb_flush_rep();
   else {
      if (hashmode) {
         hash_fb_update(&scale);
         //tentative_fb_update();
      }
      else
         tile_fb_update();
      fb_flush();
   }
   if (tentative.tiles && wireframe && scale >= 1)
      /* draw tentative points with appropriate transformation */
      for (ptr = tentative.tiles; ptr; ptr = ptr->next) {
         tile *nptr;
         tentx = ptr->x - STARTX;
         tenty = ptr->y - STARTY;
         if (onscreen(tx(tentx,tenty,txx,txy) + loadx,
                                            ty(tentx,tenty,tyx,tyy) + loady)) {
            trdisplaybox_nofb(tentx, tenty, &ptr->cells);
            for (curstate = 1; curstate < maxstates; curstate++)
               if (chgrects[curstate]) {
                  if (scale > 1)
                     for (q = 0; q < chgrects[curstate]; q++) {
                        rects[curstate][q].x--;
                        rects[curstate][q].y--;
                        rects[curstate][q].height++;
                        rects[curstate][q].width++;
                     }
                  XDrawRectangles(disp, lifew, cellgc[LOAD_BOX],
                                          rects[curstate], chgrects[curstate]);
                  if (scale > 1)
                     for (q = 0; q < chgrects[curstate]; q++) {
                        rects[curstate][q].height--;
                        rects[curstate][q].width--;
                     }
                  chgrects[curstate] = 0;
               }
         }
      }
   /* removedraw pivot of tentative points and box them */
   if (tentative.tiles) {
      if (!wireframe) boxpattern(CYAN_EVER); /* outline box */
      drawpivot();
   }
   XFlush(disp);
}

cellcount_t bounding_box(pattern *context) {
/* get the bounding box of a pattern */
   int dx, dy, state;
   tile *ptr;
   context->ymin = context->xmin = USL_MAX;
   context->ymax = context->xmax = 0;
   if (context->tiles == 0)
      return context->ymin = context->xmin = 0;
   for (state = 1; state < maxstates; state++)
      context->cellcount[state] = 0;
   FOR_CELLS(context, ptr, dx, dy) {
      if (state = getcell(&ptr->cells, dx, dy)) {
         context->cellcount[state]++;
         if (ptr->x + dx < context->xmin)
            context->xmin = ptr->x + dx;
         if (ptr->y + dy < context->ymin)
            context->ymin = ptr->y + dy;
         if (ptr->x + dx > context->xmax)
            context->xmax = ptr->x + dx;
         if (ptr->y + dy > context->ymax)
            context->ymax = ptr->y + dy;
      }
   }
   return cellcnt(context);
}

static void flood_fill(pattern *from, coord_t x, coord_t y, pattern *to) {
/* transplant all points in a blob touching (x,y) between contexts */
    static offsets neighbors[] =
    {
	{-1, -1},
	{-1,  0},
	{-1,  1},
	{ 0, -1},
	{ 0,  1},
	{ 1, -1},
	{ 1,  0},
	{ 1,  1},
#ifdef DEPTH2
	{-2, -2},
	{-2, -1},
	{-2,  0},
	{-2,  1},
	{-2,  2},
	{-1,  2},
	{ 0,  2},
	{ 1,  2},
	{ 2,  2},
	{ 2,  1},
	{ 2,  0},
	{ 2, -1},
	{ 2, -2},
	{ 1, -2},
	{ 0, -2},
	{-1, -2},
#endif /* DEPTH2 */
    };
    int i;

    /* skip empty cells */
    if (!lookcell(from, x, y))
	return;

    /* skip cells that are already colored */
    if (lookcell(to, x, y))
	return;

    /* transfer this cell from the copy to the new context */
    chgcell(to,   x, y, lookcell(from, x, y));
    chgcell(from, x, y, 0);

    for (i = 0; i < sizeof(neighbors)/sizeof(neighbors[0]); i++) {
	int nx = x + neighbors[i].xi;
	int ny = y + neighbors[i].yi;

	/* recursively transfer all its neighbors */
	flood_fill(from, nx, ny, to);
    }
}

static int refcmp(const void *r1, const void *r2) {
/* sort blobs first by least Y coordinate, then by least X coordinate */
    int	cmp = (((pattern*)r1)->ymin - ((pattern*)r2)->ymin);

    if (!cmp)
	cmp = (((pattern*)r1)->xmin - ((pattern*)r2)->xmin);
    return cmp;
}

pattern *analyze(pattern *context) {
/* analyze a pattern into an allocated list of blobs */
    cell_t	state;
    tile	*ptr;
    int		dx, dy, color = 0;
    pattern	*parts, *pp;
    static pattern analysand; /* too big to be automatic */

    initcells(&analysand);

    /* copy the pattern */
    FOR_CELLS(context, ptr, dx, dy)
	if ((state = getcell(&ptr->cells, dx, dy)))
	    chgcell(&analysand, ptr->x + dx, ptr->y + dy, state);
    /*
     * This loop is deceptively simple-looking.  It relies on flood_fill()
     * not returning until it has transpanted the largest blob it can find
     * connected to a cell.
     */
    parts = (pattern*)calloc(sizeof(pattern), 1);
    FOR_CELLS(&analysand, ptr, dx, dy)
	if (getcell(&ptr->cells, dx, dy)) {
	    parts = (pattern *)realloc(parts, sizeof(pattern) * (color + 1));
	    initcells(&parts[color]);
	    flood_fill(&analysand, ptr->x + dx, ptr->y + dy, &parts[color]);
	    color++;
	}

    /* create an extra, blank pattern entry as a fencepost */
    parts = (pattern *)realloc(parts, sizeof(pattern) * (color + 1));
    initcells(&parts[color]);

    /* this cleans up the blob order -- strictly cosmetic */
    for (pp = parts; pp->tiles; pp++)
	bounding_box(pp);
    qsort(parts, color, sizeof(pattern), refcmp);

    /* return the analyzed parts */
    return parts;
}

void saveformatsign(char f, int xoff, int yoff, FILE* ofp) {
   fprintf(ofp, "#%c", f);
   if (xoff || yoff)
      fprintf(ofp, " %d %d", xoff, yoff);
   fputs("\n", ofp);
}

void savesimple(pattern *pp, int xoff, int yoff, FILE *ofp) {
/* save a pattern as a simple picture block */
    coord_t x, y;
    cellcount_t cellcount = bounding_box(pp);
    if (cellcount == 0)
	return;
    saveformatsign('P', xoff, yoff, ofp);
    for (y = pp->ymin; y <= pp->ymax; y++) {
	for (x = pp->xmin; x <= pp->xmax; x++) {
	    int val;
	    if (val = lookcell(pp, x, y)) {
		if (maxstates > 2)
		    fputc(itos(val), ofp);
		else
		    fputc(MARK, ofp);
	    }
	    else
		fputc(SPACE, ofp);
	}
	fputc('\n', ofp);
    }
}

static void savecomplex(pattern *context, FILE *ofp) {
/* do a pattern analysis and save the pattern as a blob sequence */
    pattern	*pp, *parts = analyze(context);
    int		xcenter, ycenter, blobcount = 0;
    if (parts == NULL) {
	perror("Pattern analysis failed; too many blobs in picture.\n");
	return;
    }
    for (pp = parts; pp->tiles; pp++)
	blobcount++;
    if (blobcount > 1)
	fprintf(ofp, "#C %d blobs\n", blobcount);
    for (pp = parts; pp->tiles; pp++)
	savesimple(pp, pp->xmin - STARTX, pp->ymin - STARTY, ofp);
}

char* rlestr(int c) {
   static char s[3];
   if (c < 25) {
      s[1] = 0;
      if (c == 0)
        s[0] = 'b';
      else if (c == 1)
        s[0] = 'o';
      else
        s[0] = c + '@';
   }
   else {
      s[0] = 'o' + (c - 1)/24;
      s[1] = (c - 1)%24 + 'A';
      s[2] = 0;
   }
   return s;
}

#define rleflush(n) if (strlen(buf) > n) {\
       t = *p;\
       *p = '\0';\
       fprintf(ofp, "%s\n", buf);\
       *p = t;\
       strcpy(buf, p);\
   }\
   p = buf + strlen(buf);


void saveall(FILE *ofp, char mode) {
/* save the entire board contents (or the tentative pattern if there is one) */
    tile *ptr;
    char buf[100], *p = buf, t;
    int dx, dy, val;
    coord_t x = 0, y;
    cellcount_t cellcount;
    pattern *pp;
    if (hashmode) goto L1;
    pp = tentative.tiles ? &tentative : &active;
/*
 * Determine the bounding box of the live cells.
 */
    cellcount = bounding_box(pp);
    if (mode == '4') { /* 8-bit format */
          if (ev_mode != VALENCE_DRIVEN || (born&1) || pp->xmax - pp->xmin >= 256
                || pp->ymax - pp->ymin >= 256) {
             fprintf(stderr, "can't convert this pattern\n");
             convst = 1;
             return;
          }
          fprintf(ofp, "%c%c", pp->xmax - pp->xmin + 1, pp->ymax - pp->ymin + 1);
          fwrite(&live, 2, 1, ofp);
          fwrite(&born, 2, 1, ofp);
	  FOR_CELLS(pp, ptr, dx, dy)
		if (val = getcell(&ptr->cells, dx, dy))
		    fprintf(ofp, "%c%c",
			       ptr->x + dx - pp->xmin, ptr->y + dy - pp->ymin);
	  return;
    }
L1:
    fprintf(ofp, "##XLife\n");
    if (fname[0])
        fprintf(ofp, "#N %s\n", fname);
    else if (stashed[0]) {
        char *p = strrchr(stashed, '/');
        if (p)
             p++;
        else
             p = stashed;
        if (strchr(p, ':'))
           fprintf(ofp, "#N %s\n", p);
    }
    stamp("#0", ofp);
    if (outcome[0])
        fprintf(ofp, "#K %s\n", outcome);
    while (x < numcomments)
	fprintf(ofp, "#C %s\n", comments[x++]);
    if (fname[0])
	fprintf(ofp, "#N %s\n", fname);
    strcpy(buf, active_rules);
    if (!strcmp(buf, "life"))
        strcpy(buf, "23/3");
    t = 'U';
    if (strchr(buf, '$') || !strcmp(buf, "wireworld") || !strcmp(buf, "brian's brain")
             || strchr(buf, '/') || !strcmp(buf, "life+") || !strcmp(buf, "lifehistory"))
        t = 'T';
    fprintf(ofp, "#%c %s", t, buf);
    if (topology != 'Q')
        fprintf(ofp, ":%c%d,%d", topology, x_max_limit - x_min_limit, y_max_limit - y_min_limit);
    fprintf(ofp, "\n");
    if (*colorfile)
       fprintf(ofp, "#X %s\n", strrchr(colorfile, '/') + 1);
    if (hashmode) {
       saveHashFile(ofp);
       return;
    }
    buf[0] = 0;
/* caller may want save to shortest format */
    if (mode == '\0')
	if ((pp->ymax - pp->ymin + 1)*(pp->xmax - pp->xmin + 1) > cellcount * 8)
	    mode = 'R';
	else
	    mode = 'P';

/* now that we have the bounding box, we can gen the other formats */
    switch (mode) {
	case 'A': /* save in absolute mode */
	  fputs("#A\n", ofp);
	  FOR_CELLS(pp, ptr, dx, dy)
			if (val = getcell(&ptr->cells, dx, dy))
			    if (maxstates > 2)
				fprintf(ofp, "%d %d %d\n",
					       ptr->x + dx - xorigin, 
                                               ptr->y + dy - yorigin, val);
			    else
				fprintf(ofp, "%d %d\n",
                                               ptr->x + dx - xorigin, 
                                               ptr->y + dy - yorigin);
	return;

	case 'R':
          saveformatsign('R', pp->xmin - STARTX, pp->ymin - STARTY, ofp);
	  FOR_CELLS(pp, ptr, dx, dy)
		if (val = getcell(&ptr->cells, dx, dy))
		    if (maxstates > 2)
			  fprintf(ofp, "%d %d %d\n",
				       ptr->x + dx - pp->xmin, ptr->y + dy - pp->ymin,
				       val);
		    else
		          fprintf(ofp, "%d %d\n",
				       ptr->x + dx - pp->xmin, ptr->y + dy - pp->ymin);
	  return;

	case 'P':
	  savesimple(pp, pp->xmin - STARTX, pp->ymin - STARTY, ofp);
	  return;

	case 'D':
          saveformatsign('D', pp->xmin - STARTX, pp->ymin - STARTY, ofp);
	  x = pp->xmin;
	  y = pp->ymin;
	  FOR_CELLS(pp, ptr, dx, dy)
			if (val = getcell(&ptr->cells, dx, dy)) {
			    if (maxstates > 2)
				fprintf(ofp, "%d %d %d\n",
					       ptr->x + dx - x, ptr->y + dy - y,
					       val);
			    else
				fprintf(ofp, "%d %d\n",
					       ptr->x + dx - x, ptr->y + dy - y);
			    x = ptr->x + dx;
			    y = ptr->y + dy;
			}
	  return;

	case 'M':
          saveformatsign('M', pp->xmin - STARTX, pp->ymin - STARTY, ofp);
	  dy = 0;
	  for (y = pp->ymin; y <= pp->ymax; y++) {
	    for (x = pp->xmin; x <= pp->xmax; x = dx) {
		val = lookcell(pp, x, y);
		for (dx = x + 1; dx <= pp->xmax && lookcell(pp, dx, y) == val; dx++);
		if (val) {
		    if (dy > 1)
			sprintf(p, "%d$", dy);
		    else if (dy > 0)
			strcpy(p, "$");
                    rleflush(70);
		    dy = 0;
		    if (maxstates > 2)
			if (dx - x > 1)
			    sprintf(p, "%d%s", dx - x, rlestr(val));
			else
			    sprintf(p, "%s", rlestr(val));
		    else
			if (dx - x > 1)
			    sprintf(p, "%do", dx - x);
			else
			    strcpy(p, "o");
		}
		else if (dx <= pp->xmax) {
		    if (dy > 1)
			sprintf(p, "%d$", dy);
		    else if (dy > 0)
			strcpy(p, "$");
                    rleflush(70);
		    dy = 0;
		    if (dx - x > 1)
		        sprintf(p, "%db", dx - x);
		    else
		        strcpy(p, "b");
		}
                rleflush(70);
	    }
	    dy++;
	  }
	  if (buf[0])
	    fprintf(ofp, "%s", buf);
	  fprintf(ofp, "!\n");
	  return;

	case 'I':
	  savestructured(pp, ofp);
	  return;

	case 'S':
	  savecomplex(pp, ofp);
	  return;
    }
}
/* tile.c ends here */
