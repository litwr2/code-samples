/*
 * XLife Copyright 2012, 2013 Vladimir Lidovski vol.litwr@gmail.com
 * $Id: topology.h 216 2013-07-28 18:06:27Z micro $
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

void adjust_topology(int, int, int);
void drawboundedgrid(void);
int set_topology(char*);

#define fix_plain(nextp)    if (topology == 'P' && context == &active)\
      for (cptr = context->tiles; cptr; cptr = nextp) {\
         nextp = cptr->next;\
         if ((limits&1) && (cptr->x >= x_max_limit || cptr->x < x_min_limit)\
          || (limits&2) && (cptr->y >= y_max_limit || cptr->y < y_min_limit))\
              killtile(context, cptr);\
      }
