/*
 * XLife Copyright 2011-13 Vladimir Lidovski vol.litwr@gmail.com
 * $Id: clipboard.cpp 310 2014-02-11 07:40:09Z litwr $
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
#include "clipboard.h"

struct Clipboard clipboard;

static void clpbrd_setline(pattern *pp, char *cp, u32bits *x, u32bits *y, u32bits initx, int mod) {
   unsigned sum, i, dc;
   char c;
   while (*cp) {
      dc = sum = 0;
      while (*cp <= '9' && *cp >= '0')
         sum = sum*10 + *cp++ - '0';
      if (!sum) sum++;
      switch (*cp) {
         case '$':
            *y += sum;
            *x = initx;
            break;
         case '.':
         case 'b':
            *x += sum;
            break;
         case 'o':
            for (i = 0; i < sum; i++)
               chgcell(pp, xorigin + (*x)++, yorigin + *y, 1);
         case '\n':
         case '\t':
         case ' ':
         case '\r':
            break;
         case 'p':
         case 'q':
         case 'r':
         case 's':
         case 't':
         case 'u':
         case 'v':
         case 'w':
         case 'x':
         case 'y':
            dc = 24*(*cp++ - 'o');
         default:
            if (*cp > 'X' || *cp < 'A') return;
            if (c = (*cp - '@' + dc)%mod)
               for (i = 0; i < sum; i++)
                  chgcell(pp, xorigin + (*x)++, yorigin + *y, c);
            else
               *x += sum;
      }
      cp++;
   }
}

static void clpbrd_alloc() {
   if (clipboard.first)
      clipboard.last = clipboard.last->next =
                                (struct Bufmem*) malloc(sizeof(struct Bufmem));
   else
      clipboard.last = clipboard.first =
                                (struct Bufmem*) malloc(sizeof(struct Bufmem));
   clipboard.last->next = 0;
}

struct AR {
   coord_t x, y;
   unsigned val, q;
};

static int clpbrd_cmp(const void *p1, const void *p2) {
   if (((struct AR*)p1)->y < ((struct AR*)p2)->y) return -1;
   if (((struct AR*)p1)->y > ((struct AR*)p2)->y) return 1;
   if (((struct AR*)p1)->x < ((struct AR*)p2)->x) return -1;
   return ((struct AR*)p1)->x > ((struct AR*)p2)->x;
}

static void clpbrd_write(char **p, u32bits dx, u32bits dy, unsigned val) {
   if (dy > 1)
      sprintf(*p, "%u$", dy);
   else if (dy > 0)
      sprintf(*p, "$");
   *p +=  strlen(*p);
   if (val) {
      if (maxstates > 2)
         if (dx > 1)
            sprintf(*p, "%u%s", dx, rlestr(val));
         else
            sprintf(*p, "%s", rlestr(val));
      else
         if (dx > 1)
            sprintf(*p, "%uo", dx);
         else if (dx > 0)
            sprintf(*p, "o");
   }
   else {
      if (dx > 1)
         sprintf(*p, "%ub", dx);
      else if (dx > 0)
         sprintf(*p, "b");
   }
   *p += strlen(*p);
}

static void clpbrd_writeall(pattern *pp, cellcount_t q) {
   coord_t x, y, dx, dy, qp = 0;
   int val;
   char buf[BUFSIZ] = "", *p = buf;
   tile *tp = pp->tiles;
   struct AR *ar = (AR*) malloc(q*sizeof(struct AR));

   for (; tp; tp = tp->next)
      for (x = 0; x < 8; x++)
         for (y = 0; y < 8; y++)
            if (val = getcell(&tp->cells, x, y)) {
               ar[qp].x = tp->x + x;
               ar[qp].y = tp->y + y;
               ar[qp++].val = val;
            }
   qsort(ar, q, sizeof(struct AR), clpbrd_cmp);
   val = ar[qp = 0].val;
   y = ar[0].y;
   x = ar[0].x;
   for (;;) {
      dx = qp;
      while (++qp < q && ++x == ar[qp].x && val == ar[qp].val && y == ar[qp].y);
      ar[dx].q = qp - dx;
      if (qp >= q) break;
      val = ar[qp].val;
      y = ar[qp].y;
      x = ar[qp].x;
   }
   dx = ar[0].x - pp->xmin;
   dy = ar[0].y - pp->ymin;
   if (dx || dy) clpbrd_write(&p, dx, dy, 0);
   dx = qp = 0;
   for (;;) {
      clpbrd_write(&p, ar[qp].q, 0, ar[qp].val);
      qp += ar[qp].q;
      if (strlen(buf) > BUFSIZ - 14) {
         clpbrd_alloc();
         strcpy(clipboard.last->mem, p = buf);
         buf[0] = 0;
      }
      if (qp >= q) break;
      clpbrd_write(&p, ar[qp].y == ar[dx].y ?
                             ar[qp].x - ar[qp - 1].x - 1 : ar[qp].x - pp->xmin,
                                                       ar[qp].y - ar[dx].y, 0);
      dx = qp;
   }
   clpbrd_alloc();
   strcat(buf, "!");
   strcpy(clipboard.last->mem, buf);
   free(ar);
}

void clipboard_copy(pattern *pp) {
   cellcount_t q;
   if (q = bounding_box(pp)) {
      clipboard.x = pp->xmin - xorigin;
      clipboard.y = pp->ymin - yorigin;
      clpbrd_writeall(pp, q);
   }
}

void clipboard_flush(pattern *pp, int mod) {
   u32bits initx;
   struct Bufmem *p;
   if (p = clipboard.first) {
      initx = clipboard.x;
      do {
         clpbrd_setline(pp, p->mem, &clipboard.x, &clipboard.y, initx, mod);
         p = p->next;
         free(clipboard.first);
         clipboard.first = p;
      }
      while (p);
      display_move(0, 0, 0);
   }
}
