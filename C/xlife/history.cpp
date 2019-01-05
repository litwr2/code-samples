/*
 * XLife Copyright 2013 Vladimir Lidovski vol.litwr@gmail.com
 * $Id: history.cpp 363 2017-11-07 19:16:47Z litwr $
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

#include <stdlib.h>
#include "defs.h"
#include "tile.h"
#include "history.h"
#include "framebuffer.h"

historypattern history[2];
static historytile *historyfreep;

#ifdef HASHARRAY
void historyhashstat(int n) {
   unsigned hashsum = 0, hashcount = 0, i;
   historytile *ptr;
   for (i = 0; i < HASHSIZE; i++)
      if (ptr = history[n].hashlist[i]) {
         do hashsum++; while (ptr = ptr->hnext);
         hashcount++;
      }
   sprintf(inpbuf, "History hash %s: X=%u Y=%u [%u] filled=%.4f badness=%.3f tiles=%lu",
              n == 0 ? "init" : "main", HASHBITS - 3 -  history[n].hxs, 3 +  history[n].hxs,
              HASHBITS, (double)hashcount/HASHSIZE, hashcount ? (double)hashsum/hashcount : 0,
              history[n].tilecount);
}

void fixhistoryhash(void) {
   if (active.hxs != history->hxs) {
      int i;
      for (i = 0; i < 2; i++) {
         historytile *ptr;
         history[i].hxs = active.hxs;
         history[i].hxm = active.hxm;
         history[i].hym = active.hym;
         memset(history[i].hashlist, 0, HASHSIZE*sizeof(historytile*));
         for (ptr = history[i].tiles; ptr; ptr = ptr->next) {
            unsigned hv = HASH(ptr->x, ptr->y, history[i].hxm, history[i].hxs,
                                                               history[i].hym);
            ptr->hnext = history[i].hashlist[hv];
            history[i].hashlist[hv] = ptr;
         }
      }
   }
}

static historytile *createhistorytile(historypattern *context, coord_t x, coord_t y, unsigned hv) {
   historytile *ptr;
   if (historyfreep == 0) {
      if ((ptr = (historytile*) malloc(sizeof(historytile))) == 0)
         fatal("create: malloc error: ");
   }
   else {
      ptr = historyfreep;
      historyfreep = historyfreep->next;
   }
   memset(ptr, '\0', sizeof(historytile));
   ptr->next = context->tiles;
   context->tiles = ptr;
   ptr->hnext = context->hashlist[hv];
   context->hashlist[hv] = ptr;
   ptr->x = x;
   ptr->y = y;
   context->tilecount++;
   return ptr;
}

static historytile *findhistorytile(historypattern *context, coord_t x, coord_t y, unsigned hv) {
   historytile *ptr = context->hashlist[hv];
   while (ptr) {
      if (ptr->x == x && ptr->y == y)
         return ptr;
      ptr = ptr->hnext;
   }
   return 0;
}

historytile *makehistorytile(historypattern *context, coord_t x, coord_t y) {
   historytile *ptr;
   unsigned hv;
   hv = HASH(x, y, context->hxm, context->hxs, context->hym);
   x &= XMASK;
   y &= YMASK;
   if (ptr = findhistorytile(context, x, y, hv))
      return ptr;
   else
      return createhistorytile(context, x, y, hv);
}

historytile *fetchhistorytile(historypattern *context, coord_t x, coord_t y) {
    return findhistorytile(context, x, y, HASH(x, y, context->hxm, context->hxs, context->hym));
}

void clearhistorytiles(void) {
   historytile *ptr;
   for (int i = 0; i < 2; i++) {
      ptr = history[i].tiles;
      if (ptr) {
         while (ptr->next)
            ptr = ptr->next;
         ptr->next = historyfreep;
         historyfreep = history[i].tiles;
         history[i].tiles = 0;
      }
   }
}
#elif defined(BTREE) || defined(UNORDEREDMAP)
static historytile *createhistorytile(historypattern *context, coord_t x, coord_t y) {
   historytile *ptr;
   if (historyfreep == 0) {
      if ((ptr = (historytile*) malloc(sizeof(historytile))) == 0)
         fatal("create: malloc error: ");
   }
   else {
      ptr = historyfreep;
      historyfreep = historyfreep->next;
   }
   memset(ptr, '\0', sizeof(historytile));
   ptr->next = context->tiles;
   context->tiles = ptr;
   context->tilemap[make_pair(x, y)] = ptr;
   ptr->x = x;
   ptr->y = y;
   context->tilecount++;
   return ptr;
}

static historytile *findhistorytile(historypattern *context, coord_t x, coord_t y) {
//   historytile *ptr = ;
   if (context->tilemap.find(make_pair(x, y)) == context->tilemap.end())
      return 0;
   return context->tilemap.find(make_pair(x, y))->second;
}

historytile *makehistorytile(historypattern *context, coord_t x, coord_t y) {
   historytile *ptr;
   unsigned hv;
   x &= XMASK;
   y &= YMASK;
   if (ptr = findhistorytile(context, x, y))
      return ptr;
   else
      return createhistorytile(context, x, y);
}

historytile *fetchhistorytile(historypattern *context, coord_t x, coord_t y) {
    return findhistorytile(context, x, y);
}

void clearhistorytiles(void) {
   for (int i = 0; i < 2; i++)
      history[i].tilemap.clear();
}
#endif

void pattern2history(pattern *p, int hn) {
   tile *ptr;
   historytile *hptr;
   for (ptr = p->tiles; ptr; ptr = ptr->next) {
      hptr = makehistorytile(history + hn, ptr->x, ptr->y);
      if (ev_mode == VALENCE_DRIVEN) {
         hptr->cells.live1 |= ptr->cells.twostate.live1;
         hptr->cells.live2 |= ptr->cells.twostate.live2;
      }
      else {
         int i, j, s1 = 0, s2 = 0;
         for (i = 0; i < 8; i++)
           for (j = 0; j < 4; j++) {
              s1 += (ptr->cells.nstate.cell[j][i] != 0) << j*8 + i;
              s2 += (ptr->cells.nstate.cell[j + 4][i] != 0) << j*8 + i;
           }
         hptr->cells.live1 |= s1;
         hptr->cells.live2 |= s2;
      }
   }
}

void add2history(tile *ptr) {
   historytile *hptr = makehistorytile(history + 1, ptr->x, ptr->y);
   if (ev_mode == VALENCE_DRIVEN) {
      hptr->cells.live1 |= ptr->cells.twostate.live1;
      hptr->cells.live2 |= ptr->cells.twostate.live2;
   }
   else if ((hptr->cells.live1 & hptr->cells.live2) != 0xffffffffLU) {
      int i, j, s1 = 0, s2 = 0;
      for (i = 0; i < 8; i++)
         for (j = 0; j < 4; j++) {
            s1 += (ptr->cells.nstate.cell[j][i] != 0) << j*8 + i;
            s2 += (ptr->cells.nstate.cell[j + 4][i] != 0) << j*8 + i;
         }      
      hptr->cells.live1 |= s1;
      hptr->cells.live2 |= s2;
   }
}

int histpaint[2] = {HISTCOLOR1, HISTCOLOR2};

static void displayline_hist(u32bits line, u32bits x, u32bits y, int color) {
   int c = histpaint[color];
   if (scale >= 0) {
      if (line&1) fb_ins(x, y, c);
      if (line&2) fb_ins(x + 1, y, c);
      if (line&4) fb_ins(x + 2, y, c);
      if (line&8) fb_ins(x + 3, y, c);
      if (line&16) fb_ins(x + 4, y, c);
      if (line&32) fb_ins(x + 5, y, c);
      if (line&64) fb_ins(x + 6, y, c);
      if (line&128) fb_ins(x + 7, y, c);
   }
   else if (scale == -1) {
      if (line&3) fb_ins(x, y, c);
      if (line&12) fb_ins(x + 2, y, c);
      if (line&48) fb_ins(x + 4, y, c);
      if (line&192) fb_ins(x + 6, y, c);
   }
   else if (scale == -2) {
      if (line&15) fb_ins(x, y, c);
      if (line&240) fb_ins(x + 4, y, c);
   }
   else
      if (line&255) fb_ins(x, y, c);
}

void displayhistorybox(u32bits x, u32bits y, historytile *ptr, int color) {
   register int i, j;
   register u32bits live1 = ptr->cells.live1;
   register u32bits live2 = ptr->cells.live2;
   if (scale >= 0) {
      displayline_hist(live1, x, y, color);
      displayline_hist(live1>>8, x, ++y, color);
      displayline_hist(live1>>16, x, ++y, color);
      displayline_hist(live1>>24, x, ++y, color);
      displayline_hist(live2, x, ++y, color);
      displayline_hist(live2>>8, x, ++y, color);
      displayline_hist(live2>>16, x, ++y, color);
      displayline_hist(live2>>24, x, ++y, color);
   }
   else if (scale == -1) {
      displayline_hist(live1|live1>>8, x, y, color);
      displayline_hist(live1>>16|live1>>24, x, y += 2, color);
      displayline_hist(live2|live2>>8, x, y += 2, color);
      displayline_hist(live2>>16|live2>>24, x, y += 2, color);
   }
   else if (scale == -2) {
      displayline_hist(live1|live1>>8|live1>>16|live1>>24, x, y, color);
      displayline_hist(live2|live2>>8|live2>>16|live2>>24, x, y += 4, color);
   }
   else
      displayline_hist((live1|live2) != 0, x, y, color);
}

