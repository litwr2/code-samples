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
 *
 * CMU SUCKS
 */

/*
A lot of modifications were added at 2001, 2011-13 by Vladimir Lidovski vol.litwr@gmail.com
(C) This version of XLife may be used under the same conditions as mentioned above
$Id: generate.cpp 364 2018-03-31 08:57:04Z litwr $
*/

/*
 * This module contains the evolution and rule-setting code.  It has three
 * entry points:
 *
 *	evolve() --- perform evolution on the board
 *	newrules() --- prompt for & accept new 2-state/generated/named/historical+topology rules
 *	readrules() --- read N-state rules+topology from given file
 */

#include <thread>
#include <condition_variable>
#include <mutex>
#include <atomic>
#include <chrono>
#include <stdlib.h>
#include <errno.h>
#include "defs.h"
#include "tab.h"
#include "tile.h"
#include "file.h"
#include "colors.h"
#include "history.h"
#include "topology.h"

float payoffs[2][2] = {1, 0, 0, 0};//static float payoffs[2][2] = {1, 0, 0, 0};
static unsigned long dm_given;
#define NMAX ((MAXSTATES > 64) ? 64 : MAXSTATES)

/* strategy indices */
#define COOPERATE	0
#define DEFECT		1

/* cell state values */
#define COOPERATOR	1
#define DEFECTOR	2

#define adjacency_links(istate, live1, live2)\
        cptrup = cptr->up;\
        cptrdn = cptr->dn;\
        cptrlf = cptr->lf;\
        cptrrt = cptr->rt;\
        t1 = live1 &0xff;\
        if (t1) {\
            if (cptrup == NULL) {\
                cptrup = maketile(context, cptr->x, cptr->y - BOXSIZE);\
                cptrup->dn = cptr;\
            }\
            t2 = tab1[t1];\
            cptrup->cells.istate.on[7] += t2;\
            cptr->cells.istate.on[1] += t2;\
            cptr->cells.istate.on[0] += tab2[t1];\
        }\
        t1 = (live2 & 0xff000000) >> 24;\
        if (t1) {\
            if (cptrdn == NULL) {\
                cptrdn = maketile(context, cptr->x, cptr->y + BOXSIZE);\
                cptrdn->up = cptr;\
            }\
            t2 = tab1[t1];\
            cptrdn->cells.istate.on[0] += t2;\
            cptr->cells.istate.on[6] += t2;\
            cptr->cells.istate.on[7] += tab2[t1];\
        }\
        t1 = live1;\
        t2 = live2;\
        if (t1 & 0x1010101) {\
            if (cptrlf == NULL) {\
                cptrlf = maketile(context, cptr->x - BOXSIZE, cptr->y);\
                cptrlf->rt = cptr;\
            }\
            if (t1 & 0x1) {\
                cptrlf->cells.istate.on[0] += 0x10000000;\
                cptrlf->cells.istate.on[1] += 0x10000000;\
                if (cptrlf->up == NULL)\
                    cptrlf->up = maketile(context, cptr->x - BOXSIZE, cptr->y - BOXSIZE);\
                cptrlf->up->cells.istate.on[7] += 0x10000000;\
                cptrlf->up->dn = cptrlf;\
            }\
            if (t1 & 0x100) {\
                cptrlf->cells.istate.on[0] += 0x10000000;\
                cptrlf->cells.istate.on[1] += 0x10000000;\
                cptrlf->cells.istate.on[2] += 0x10000000;\
            }\
            if (t1 & 0x10000) {\
                cptrlf->cells.istate.on[1] += 0x10000000;\
                cptrlf->cells.istate.on[2] += 0x10000000;\
                cptrlf->cells.istate.on[3] += 0x10000000;\
            }\
            if (t1 & 0x1000000) {\
                cptrlf->cells.istate.on[2] += 0x10000000;\
                cptrlf->cells.istate.on[3] += 0x10000000;\
                cptrlf->cells.istate.on[4] += 0x10000000;\
            }\
        }\
        if (t2 & 0x1010101) {\
            if (cptrlf == NULL) {\
                cptrlf = maketile(context, cptr->x - BOXSIZE, cptr->y);\
                cptrlf->rt = cptr;\
            }\
            if (t2 & 0x1) {\
                cptrlf->cells.istate.on[3] += 0x10000000;\
                cptrlf->cells.istate.on[4] += 0x10000000;\
                cptrlf->cells.istate.on[5] += 0x10000000;\
            }\
            if (t2 & 0x100) {\
                cptrlf->cells.istate.on[4] += 0x10000000;\
                cptrlf->cells.istate.on[5] += 0x10000000;\
                cptrlf->cells.istate.on[6] += 0x10000000;\
            }\
            if (t2 & 0x10000) {\
                cptrlf->cells.istate.on[5] += 0x10000000;\
                cptrlf->cells.istate.on[6] += 0x10000000;\
                cptrlf->cells.istate.on[7] += 0x10000000;\
            }\
            if (t2 & 0x1000000) {\
                cptrlf->cells.istate.on[6] += 0x10000000;\
                cptrlf->cells.istate.on[7] += 0x10000000;\
                if (cptrlf->dn == NULL)\
                    cptrlf->dn = maketile(context, cptr->x - BOXSIZE, cptr->y + BOXSIZE);\
                cptrlf->dn->cells.istate.on[0] += 0x10000000;\
                cptrlf->dn->up = cptrlf;\
            }\
        }\
        if (t1 & 0x80808080) {\
            if (cptrrt == NULL) {\
                cptrrt = maketile(context, cptr->x + BOXSIZE, cptr->y);\
                cptrrt->lf = cptr;\
            }\
            if (t1 & 0x80) {\
                cptrrt->cells.istate.on[0] += 0x1;\
                cptrrt->cells.istate.on[1] += 0x1;\
                if (cptrrt->up == NULL)\
                    cptrrt->up = maketile(context, cptr->x + BOXSIZE, cptr->y - BOXSIZE);\
                cptrrt->up->cells.istate.on[7] += 0x1;\
                cptrrt->up->dn = cptrrt;\
            }\
            if (t1 & 0x8000) {\
                cptrrt->cells.istate.on[0] += 0x1;\
                cptrrt->cells.istate.on[1] += 0x1;\
                cptrrt->cells.istate.on[2] += 0x1;\
            }\
            if (t1 & 0x800000) {\
                cptrrt->cells.istate.on[1] += 0x1;\
                cptrrt->cells.istate.on[2] += 0x1;\
                cptrrt->cells.istate.on[3] += 0x1;\
            }\
            if (t1 & 0x80000000) {\
                cptrrt->cells.istate.on[2] += 0x1;\
                cptrrt->cells.istate.on[3] += 0x1;\
                cptrrt->cells.istate.on[4] += 0x1;\
            }\
        }\
        if (t2 & 0x80808080) {\
            if (cptrrt == NULL) {\
                cptrrt = maketile(context, cptr->x + BOXSIZE, cptr->y);\
                cptrrt->lf = cptr;\
            }\
            if (t2 & 0x80) {\
                cptrrt->cells.istate.on[3] += 0x1;\
                cptrrt->cells.istate.on[4] += 0x1;\
                cptrrt->cells.istate.on[5] += 0x1;\
            }\
            if (t2 & 0x8000) {\
                cptrrt->cells.istate.on[4] += 0x1;\
                cptrrt->cells.istate.on[5] += 0x1;\
                cptrrt->cells.istate.on[6] += 0x1;\
            }\
            if (t2 & 0x800000) {\
                cptrrt->cells.istate.on[5] += 0x1;\
                cptrrt->cells.istate.on[6] += 0x1;\
                cptrrt->cells.istate.on[7] += 0x1;\
            }\
            if (t2 & 0x80000000) {\
                cptrrt->cells.istate.on[6] += 0x1;\
                cptrrt->cells.istate.on[7] += 0x1;\
                if (cptrrt->dn == NULL)\
                    cptrrt->dn = maketile(context, cptr->x + BOXSIZE, cptr->y + BOXSIZE);\
                cptrrt->dn->cells.istate.on[0] += 0x1;\
                cptrrt->dn->up = cptrrt;\
            }\
        }\
        t1 = (live1 & 0xff00) >> 8;\
        t2 = (live1 & 0xff0000) >> 16;\
        if (t1) {\
            t3 = tab1[t1];\
            cptr->cells.istate.on[1] += tab2[t1];\
            cptr->cells.istate.on[0] += t3;\
            cptr->cells.istate.on[2] += t3;\
        }\
        t1 = (live1 & 0xff000000) >> 24;\
        if (t2) {\
            t3 = tab1[t2];\
            cptr->cells.istate.on[2] += tab2[t2];\
            cptr->cells.istate.on[1] += t3;\
            cptr->cells.istate.on[3] += t3;\
        }\
        t2 = (live2 & 0xff);\
        if (t1) {\
            t3 = tab1[t1];\
            cptr->cells.istate.on[3] += tab2[t1];\
            cptr->cells.istate.on[2] += t3;\
            cptr->cells.istate.on[4] += t3;\
        }\
        t1 = (live2 & 0xff00) >> 8;\
        if (t2) {\
            t3 = tab1[t2];\
            cptr->cells.istate.on[4] += tab2[t2];\
            cptr->cells.istate.on[3] += t3;\
            cptr->cells.istate.on[5] += t3;\
        }\
        t2 = (live2 & 0xff0000) >> 16;\
        if (t1) {\
            t3 = tab1[t1];\
            cptr->cells.istate.on[5] += tab2[t1];\
            cptr->cells.istate.on[4] += t3;\
            cptr->cells.istate.on[6] += t3;\
        }\
        if (t2) {\
            t3 = tab1[t2];\
            cptr->cells.istate.on[6] += tab2[t2];\
            cptr->cells.istate.on[5] += t3;\
            cptr->cells.istate.on[7] += t3;\
        }\
        cptr->up = cptrup;\
        cptr->dn = cptrdn;\
        cptr->lf = cptrlf;\
        cptr->rt = cptrrt;

#define xadjacency_links(istate, live1, live2) {\
        cptrup = cptr->up;\
        cptrdn = cptr->dn;\
        cptrlf = cptr->lf;\
        cptrrt = cptr->rt;\
        if (t1 = live1 &0xff) {\
            if (cptrup == NULL) {\
                cptrup = maketile(context, cptr->x, cptr->y - BOXSIZE);\
                cptrup->dn = cptr;\
            }\
            cptrup->cells.istate.edges1 |= t1 << 1;\
        }\
        if (t1 = (live2 & 0xff000000) >> 24) {\
            if (cptrdn == NULL) {\
                cptrdn = maketile(context, cptr->x, cptr->y + BOXSIZE);\
                cptrdn->up = cptr;\
            }\
            cptrdn->cells.istate.edges2 |= t1 << 1;\
        }\
        t1 = live1;\
        t2 = live2;\
        if (t1 & 0x1010101) {\
            if (cptrlf == NULL) {\
                cptrlf = maketile(context, cptr->x - BOXSIZE, cptr->y);\
                cptrlf->rt = cptr;\
            }\
            if (t1 & 0x1) {\
                cptrlf->cells.istate.edges2 |= 0x20000;\
                if (cptrlf->up == NULL)\
                    cptrlf->up = maketile(context, cptr->x - BOXSIZE, cptr->y - BOXSIZE);\
                cptrlf->up->cells.istate.edges1 |= 0x200;\
                cptrlf->up->dn = cptrlf;\
            }\
            if (t1 & 0x100)\
                cptrlf->cells.istate.edges2 |= 0x10000;\
            if (t1 & 0x10000)\
                cptrlf->cells.istate.edges1 |= 0x1000000;\
            if (t1 & 0x1000000)\
                cptrlf->cells.istate.edges1 |= 0x40000;\
        }\
        if (t2 & 0x1010101) {\
            if (cptrlf == NULL) {\
                cptrlf = maketile(context, cptr->x - BOXSIZE, cptr->y);\
                cptrlf->rt = cptr;\
            }\
            if (t2 & 0x1)\
                cptrlf->cells.istate.edges1 |= 0x4000;\
            if (t2 & 0x100)\
                cptrlf->cells.istate.edges1 |= 0x10000;\
            if (t2 & 0x10000)\
                cptrlf->cells.istate.edges1 |= 0x20000;\
             if (t2 & 0x1000000) {\
                cptrlf->cells.istate.edges1 |= 0x800;\
                if (cptrlf->dn == NULL)\
                    cptrlf->dn = maketile(context, cptr->x - BOXSIZE, cptr->y + BOXSIZE);\
                cptrlf->dn->cells.istate.edges2 |= 0x200;\
                cptrlf->dn->up = cptrlf;\
            }\
        }\
        if (t1 & 0x80808080) {\
            if (cptrrt == NULL) {\
                cptrrt = maketile(context, cptr->x + BOXSIZE, cptr->y);\
                cptrrt->lf = cptr;\
            }\
            if (t1 & 0x80) {\
                cptrrt->cells.istate.edges2 |= 0x1000;\
                if (cptrrt->up == NULL)\
                    cptrrt->up = maketile(context, cptr->x + BOXSIZE, cptr->y - BOXSIZE);\
                cptrrt->up->cells.istate.edges1 |= 0x1;\
                cptrrt->up->dn = cptrrt;\
            }\
            if (t1 & 0x8000)\
                cptrrt->cells.istate.edges2 |= 0x40000;\
            if (t1 & 0x800000)\
                cptrrt->cells.istate.edges1 |= 0x200000;\
            if (t1 & 0x80000000)\
                cptrrt->cells.istate.edges1 |= 0x800000;\
        }\
        if (t2 & 0x80808080) {\
            if (cptrrt == NULL) {\
                cptrrt = maketile(context, cptr->x + BOXSIZE, cptr->y);\
                cptrrt->lf = cptr;\
            }\
            if (t2 & 0x80)\
                cptrrt->cells.istate.edges1 |= 0x1000;\
            if (t2 & 0x8000)\
                cptrrt->cells.istate.edges1 |= 0x2000;\
            if (t2 & 0x800000)\
                cptrrt->cells.istate.edges1 |= 0x8000;\
            if (t2 & 0x80000000) {\
                cptrrt->cells.istate.edges1 |= 0x400;\
                if (cptrrt->dn == NULL)\
                    cptrrt->dn = maketile(context, cptr->x + BOXSIZE, cptr->y + BOXSIZE);\
                cptrrt->dn->cells.istate.edges2 |= 0x1;\
                cptrrt->dn->up = cptrrt;\
            }\
        }\
        cptr->up = cptrup;\
        cptr->dn = cptrdn;\
        cptr->lf = cptrlf;\
        cptr->rt = cptrrt;}

static void osc_values(pattern *context, tile *cptr) {
   long t;
   if (ev_mode == VALENCE_DRIVEN) {
      context->osc1 += ((cptr->y >> 3) + cptr->cells.twostate.live1)*((cptr->x >> 3)
         + cptr->cells.twostate.live2);
      t = (cptr->cells.twostate.live1 ^ cptr->cells.twostate.live2) + cptr->x;
      context->osc2 += (u32bits)((double) t*t);
      t = (cptr->cells.twostate.live1 ^ cptr->cells.twostate.live2) + cptr->y;
      context->osc3 += (u32bits)((double) t*t);
      context->osc4 += cptr->cells.twostate.live1 << 1 ^ cptr->cells.twostate.live2;
      context->osc5 += cptr->cells.twostate.live1 ^ cptr->cells.twostate.live2 << 1;
   }
   else {
      context->osc1 += ((cptr->y >> 3) + cptr->cells.nstate.live[7][0]
         + cptr->cells.nstate.live[5][1])*((cptr->x >> 3) + cptr->cells.nstate.live[3][0]
         + cptr->cells.nstate.live[1][1]);
      t = ((cptr->cells.nstate.live[4][0] ^ cptr->cells.nstate.live[4][1])
         ^ (cptr->cells.nstate.live[5][0] ^ cptr->cells.nstate.live[5][1]) << 1
         ^ (cptr->cells.nstate.live[6][0] ^ cptr->cells.nstate.live[6][1]) << 2
         ^ (cptr->cells.nstate.live[7][0] ^ cptr->cells.nstate.live[7][1]) << 3) + cptr->x;
      context->osc2 += (u32bits)((double) t*t);
      t = ((cptr->cells.nstate.live[0][0] ^ cptr->cells.nstate.live[0][1])
         ^ (cptr->cells.nstate.live[1][0] ^ cptr->cells.nstate.live[1][1]) << 1
         ^ (cptr->cells.nstate.live[2][0] ^ cptr->cells.nstate.live[2][1]) << 2
         ^ (cptr->cells.nstate.live[3][0] ^ cptr->cells.nstate.live[3][1]) << 3) + cptr->y;
      context->osc3 += (u32bits)((double) t*t);
      context->osc4 += (cptr->cells.nstate.live[4][0] ^ cptr->cells.nstate.live[4][1]) << 3
         ^ (cptr->cells.nstate.live[5][0] ^ cptr->cells.nstate.live[5][1]) << 2
         ^ (cptr->cells.nstate.live[6][0] ^ cptr->cells.nstate.live[6][1]) << 1
         ^ (cptr->cells.nstate.live[7][0] ^ cptr->cells.nstate.live[7][1]);
      context->osc5 += (cptr->cells.nstate.live[0][0] ^ cptr->cells.nstate.live[0][1]) << 3
         ^ (cptr->cells.nstate.live[2][0] ^ cptr->cells.nstate.live[1][1]) << 2
         ^ (cptr->cells.nstate.live[1][0] ^ cptr->cells.nstate.live[2][1]) << 1
         ^ (cptr->cells.nstate.live[3][0] ^ cptr->cells.nstate.live[3][1]);
   }
}

mutex mx1, mx4, mx5;
u32bits live_total;
thread_local unsigned int cellcount, chgcount;

#define gather_statistics()\
        if (t1 || t2) {\
          cptr->dead = 0;\
          if (oscillators) { lock_guard<mutex> lk(mx4); osc_values(context, cptr); }\
          live_total = 1;\
          if (dispboxes) {\
            cellcount += tab3[t1 & 0xff] + tab3[t1 >> 8 & 0xff]\
               + tab3[t1 >> 16 & 0xff] + tab3[t1 >> 24]\
               + tab3[t2 & 0xff] + tab3[t2 >> 8 & 0xff]\
               + tab3[t2 >> 16 & 0xff] + tab3[t2 >> 24];}\
        }\
        else\
            cptr->dead++;\
        t1 ^=  cptr->cells.twostate.olive1;\
        t2 ^=  cptr->cells.twostate.olive2;\
        if (t1 || t2) {\
            if (truehistory && context == &active) { lock_guard<mutex> lk(mx5); add2history(cptr); }\
            if (dispchanges) {\
              chgcount += tab3[t1 & 0xff] + tab3[t1 >> 8 & 0xff]\
                 + tab3[t1 >> 16 & 0xff] + tab3[t1 >> 24]\
                 + tab3[t2 & 0xff] + tab3[t2 >> 8 & 0xff]\
                 + tab3[t2 >> 16 & 0xff] + tab3[t2 >> 24];}\
            else \
               chgcount = 1;}

unsigned threads = 1;
thread thread_ref[MAX_THREADS];

static void evolve2_1(pattern *context, tile *cptr, tile *finish) {
    u32bits t1, t2, t3, t4, t5;
    cellcount = chgcount = 0;
    while (cptr != finish) {
        cptr->cells.twostate.olive1 = t1 = cptr->cells.twostate.live1;
        cptr->cells.twostate.olive2 = t2 = cptr->cells.twostate.live2;
        cptr->cells.twostate.live2 = t2 =
            (lookup[t4 = (t3 = cptr->cells.twostate.edges1) >> 4 & 0x3f
              | (t2 & 0xf8000000) >> 21 | t3 & 0x20800
            | (t2 & 0xf80000) >> 7] << 4
              | lookup[t5 = t3 & 0x3f
            | (t2 & 0x1f000000) >> 17 | (t3 & 0x400) >> 4
            | (t2 & 0x1f0000 | t3 & 0x8000) >> 3]) << 24
            | (lookup[t4 = t4 >> 6 | (t2 & 0xf800 | t3 & 0x10000) << 1] << 4
              | lookup[t5 = t5 >> 6 | (t2 & 0x1f00) << 5 | (t3 & 0x2000) >> 1]) << 16
            | (lookup[t4 = t4 >> 6 | (t2 & 0xf8) << 9 | (t3 & 0x4000) << 3] << 4
              | lookup[t5 = t5 >> 6 | (t2 & 0x1f) << 13 | t3 & 0x1000]) << 8
            | lookup[t4 = t4 >> 6 | (t1 & 0xf8000000) >> 15 | (t3 & 0x40000) >> 1] << 4
              | lookup[t5 = t5 >> 6 | (t1 & 0x1f000000 | t3 & 0x800000) >> 11];
        cptr->cells.twostate.live1 = t1 =
            (lookup[t4 = t4 >> 6 | (t1 & 0xf80000 | t3 & 0x1000000) >> 7] << 4
              | lookup[t5 = t5 >> 6 | (t1 & 0x1f0000) >> 3 | (t3 & 0x200000) >> 9]) << 24
            | (lookup[t4 = t4 >> 6 | (t1 & 0xf800 | (t3 = cptr->cells.twostate.edges2) & 0x10000) << 1] << 4
              | lookup[t5 = (t5 | t3 & 0x40000) >> 6 | (t1 & 0x1f00) << 5]) << 16
            | (lookup[t4 = t4 >> 6 | (t1 & 0xf8) << 9 | t3 & 0x20000] << 4
              | lookup[t5 = t5 >> 6 | (t1 & 0x1f) << 13 | t3 & 0x1000]) << 8
            | lookup[t4 = t4 >> 6 | (t3 & 0x3f0) << 8] << 4
              | lookup[t5 = t5 >> 6 | (t3 & 0x3f) << 12];
        cptr->cells.twostate.edges1 = cptr->cells.twostate.edges2 = 0;
        /*
         * Box-aging and statistics gathering.
         */
        gather_statistics();
        cptr = cptr->next;
    }
    lock_guard<mutex> lk(mx1);
    context->cellcount[1] += cellcount;
    context->chgcount += chgcount;
}

static void evolve2(pattern *context) {
    register u32bits t1, t2, t3, t4, t5, q = 0, t = 0;
    register tile *cptr;
    tile *cptrup, *cptrdn, *cptrlf, *cptrrt, *thread_ptr_end[threads], *thread_ptr_begin[threads];

    live_total = 0;
    for (unsigned i = 1; i < threads; ++i)
        thread_ptr_end[i] = thread_ptr_begin[i] = 0;
    for (cptr = context->tiles; cptr; cptr = cptr->next) {
        q++;
        if (cptr->cells.twostate.live1 | cptr->cells.twostate.live2)
            xadjacency_links(twostate, cptr->cells.twostate.live1, cptr->cells.twostate.live2);
        if (context->tilecount < q*threads) thread_ptr_end[t] = cptr, thread_ptr_begin[++t] = cptr, q = 0;
    }
    thread_ptr_end[t] = 0;
    thread_ptr_begin[0] = context->tiles;
    /*
    * Perform actual evolution.
    */
    fix_plain(cptrup);
    for (unsigned i = 1; i < threads; ++i)
        thread_ref[i - 1] = thread(evolve2_1, context, thread_ptr_begin[i], thread_ptr_end[i]);
    evolve2_1(context, thread_ptr_begin[0], thread_ptr_end[0]);
    for (unsigned i = 1; i < threads; ++i)
        thread_ref[i - 1].join();
    if (live_total == 0 || context->chgcount == 0)
        state = STOP;
}

static const u32bits invmask = 0xffffffff;

static void evolve2b0(pattern *context) {
    register u32bits t1, t2, t3, t4, t5;
    u32bits *tmpptr, live_total = 0;
    register tile *cptr;
    tile *cptrup, *cptrdn, *cptrlf, *cptrrt;

    cptr = context->tiles;
    if (context->cellmass) {
      int live8st = !!(live & 0x100);
      tile *cptrul, *cptrur, *cptrdl, *cptrdr;

      for (; cptr; cptr = cptr->next) {
        if ((cptr->cells.twostate.live1 | cptr->cells.twostate.live2) == 0)
            continue;
        maketile(context, cptr->x, cptr->y - BOXSIZE);
        maketile(context, cptr->x, cptr->y + BOXSIZE);
        maketile(context, cptr->x - BOXSIZE, cptr->y);
        maketile(context, cptr->x - BOXSIZE, cptr->y - BOXSIZE);
        maketile(context, cptr->x - BOXSIZE, cptr->y + BOXSIZE);
        maketile(context, cptr->x + BOXSIZE, cptr->y);
        maketile(context, cptr->x + BOXSIZE, cptr->y - BOXSIZE);
        maketile(context, cptr->x + BOXSIZE, cptr->y + BOXSIZE);
      }
      for (cptr = context->tiles; cptr; cptr = cptr->next) {
        cptr->cells.twostate.live1 ^= invmask;
        cptr->cells.twostate.live2 ^= invmask;
        cptrup = fetchtile(context, cptr->x, cptr->y - BOXSIZE);
        cptrdn = fetchtile(context, cptr->x, cptr->y + BOXSIZE);
        cptrlf = fetchtile(context, cptr->x - BOXSIZE, cptr->y);
        cptrrt = fetchtile(context, cptr->x + BOXSIZE, cptr->y);
        cptrul = fetchtile(context, cptr->x - BOXSIZE, cptr->y - BOXSIZE);
        cptrur = fetchtile(context, cptr->x + BOXSIZE, cptr->y - BOXSIZE);
        cptrdl = fetchtile(context, cptr->x - BOXSIZE, cptr->y + BOXSIZE);
        cptrdr = fetchtile(context, cptr->x + BOXSIZE, cptr->y + BOXSIZE);
        if (!cptrup) cptr->cells.twostate.edges2 += 0x1fe;
        if (!cptrdn) cptr->cells.twostate.edges1 += 0x1fe;
        if (!cptrlf) {
           cptr->cells.twostate.edges1 += 0xa0b400;
           cptr->cells.twostate.edges2 += 0x41000;
        }
        if (!cptrrt) {
           cptr->cells.twostate.edges1 += 0x1074800;
           cptr->cells.twostate.edges2 += 0x30000;
        }
        if (!cptrul) cptr->cells.twostate.edges2 += 1;
        if (!cptrur) cptr->cells.twostate.edges2 += 0x200;
        if (!cptrdl) cptr->cells.twostate.edges1 += 1;
        if (!cptrdr) cptr->cells.twostate.edges1 += 0x200;
        if ((cptr->cells.twostate.live1 | cptr->cells.twostate.live2) == 0)
            continue;

        if (t1 = cptr->cells.twostate.live1 &0xff)
            if (cptrup) cptrup->cells.twostate.edges1 += t1 << 1;

        if (t1 = (cptr->cells.twostate.live2 & 0xff000000) >> 24)
            if (cptrdn) cptrdn->cells.twostate.edges2 += t1 << 1;

        t1 = cptr->cells.twostate.live1;
        t2 = cptr->cells.twostate.live2;
        if (t1&0x1010101 && cptrlf) {
            if (t1 & 0x1)
                cptrlf->cells.twostate.edges2 += 0x20000;
            if (t1 & 0x100)
                cptrlf->cells.twostate.edges2 += 0x10000;
            if (t1 & 0x10000)
                cptrlf->cells.twostate.edges1 += 0x1000000;
            if (t1 & 0x1000000)
                cptrlf->cells.twostate.edges1 += 0x40000;
        }
        if (t1&0x1 && cptrul) cptrul->cells.twostate.edges1 += 0x200;

        if (t2&0x1010101 && cptrlf) {
            if (t2 & 0x1)
                cptrlf->cells.twostate.edges1 += 0x4000;
            if (t2 & 0x100)
                cptrlf->cells.twostate.edges1 += 0x10000;
            if (t2 & 0x10000)
                cptrlf->cells.twostate.edges1 += 0x20000;
            if (t2 & 0x1000000)
                cptrlf->cells.twostate.edges1 += 0x800;
        }
        if (t2&0x1000000 && cptrdl) cptrdl->cells.twostate.edges2 += 0x200;

        if (t1&0x80808080 && cptrrt) {
            if (t1 & 0x80)
                cptrrt->cells.twostate.edges2 += 0x1000;
            if (t1 & 0x8000)
                cptrrt->cells.twostate.edges2 += 0x40000;
            if (t1 & 0x800000)
                cptrrt->cells.twostate.edges1 += 0x200000;
            if (t1 & 0x80000000)
                cptrrt->cells.twostate.edges1 += 0x800000;
        }
        if (t1&0x80 && cptrur) cptrur->cells.twostate.edges1 += 0x1;

        if (t2&0x80808080 && cptrrt) {
            if (t2 & 0x80)
                cptrrt->cells.twostate.edges1 += 0x1000;
            if (t2 & 0x8000)
                cptrrt->cells.twostate.edges1 += 0x2000;
            if (t2 & 0x800000)
                cptrrt->cells.twostate.edges1 += 0x8000;
            if (t2 & 0x80000000)
                cptrrt->cells.twostate.edges1 += 0x400;
        }
        if (t2&0x80000000 && cptrdr) cptrdr->cells.twostate.edges2 += 0x1;

        cptr->up = cptrup;
        cptr->dn = cptrdn;
        cptr->lf = cptrlf;
        cptr->rt = cptrrt;
      }
      /*
      * Perform actual evolution.
      */
      fix_plain(cptrup);
      context->cellmass = live8st;
      for (cptr = context->tiles; cptr; cptr = cptr->next) {
        cptr->cells.twostate.olive1 = (t1 = cptr->cells.twostate.live1) ^ invmask;
        cptr->cells.twostate.olive2 = (t2 = cptr->cells.twostate.live2) ^ invmask;
        cptr->cells.twostate.live2 = 
           ((lookup[t4 = (t3 = cptr->cells.twostate.edges1) >> 4 & 0x3f
              | (t2 & 0xf8000000) >> 21 | t3 & 0x20800
              | (t2 & 0xf80000) >> 7] << 4
           | lookup[t5 = t3 & 0x3f
              | (t2 & 0x1f000000) >> 17 | (t3 & 0x400) >> 4
              | (t2 & 0x1f0000 | t3 & 0x8000) >> 3]) << 24
           | (lookup[t4 = t4 >> 6 | (t2 & 0xf800 | t3 & 0x10000) << 1] << 4
              | lookup[t5 = t5 >> 6 | (t2 & 0x1f00) << 5 | (t3 & 0x2000) >> 1]) << 16
           | (lookup[t4 = t4 >> 6 | (t2 & 0xf8) << 9 | (t3 & 0x4000) << 3] << 4
              | lookup[t5 = t5 >> 6 | (t2 & 0x1f) << 13 | t3 & 0x1000]) << 8
           | lookup[t4 = t4 >> 6 | (t1 & 0xf8000000) >> 15 | (t3 & 0x40000) >> 1] << 4
              | lookup[t5 = t5 >> 6 | (t1 & 0x1f000000 | t3 & 0x800000) >> 11]) ^ (invmask*live8st);
        
        cptr->cells.twostate.live1 = t1 =
           ((lookup[t4 = t4 >> 6 | (t1 & 0xf80000 | t3 & 0x1000000) >> 7] << 4
              | lookup[t5 = t5 >> 6 | (t1 & 0x1f0000) >> 3 | (t3 & 0x200000) >> 9]) << 24
           | (lookup[t4 = t4 >> 6 | (t1 & 0xf800 | (t3 = cptr->cells.twostate.edges2) & 0x10000) << 1] << 4
              | lookup[t5 = (t5 | t3 & 0x40000) >> 6 | (t1 & 0x1f00) << 5]) << 16
           | (lookup[t4 = t4 >> 6 | (t1 & 0xf8) << 9 | t3 & 0x20000] << 4
              | lookup[t5 = t5 >> 6 | (t1 & 0x1f) << 13 | t3 & 0x1000]) << 8
           | lookup[t4 = t4 >> 6 | (t3 & 0x3f0) << 8] << 4
              | lookup[t5 = t5 >> 6 | (t3 & 0x3f) << 12]) ^ invmask*live8st;
        cptr->cells.twostate.edges1 = cptr->cells.twostate.edges2 = 0;

        /*
         * Box-aging and statistics gathering.
         */
        t2 = cptr->cells.twostate.live2;
        gather_statistics();
      }
    }
    else {
      for (; cptr; cptr = cptr->next) {
        if ((cptr->cells.twostate.live1 | cptr->cells.twostate.live2) == 0)
            continue;
        xadjacency_links(twostate, cptr->cells.twostate.live1, cptr->cells.twostate.live2); /* macro */
      }

      /*
      * Perform actual evolution.
      */
      fix_plain(cptrup);
      context->cellmass = 1;
      for (cptr = context->tiles; cptr; cptr = cptr->next) {
        cptr->cells.twostate.olive1 = t1 = cptr->cells.twostate.live1;
        cptr->cells.twostate.olive2 = t2 = cptr->cells.twostate.live2;
        cptr->cells.twostate.live2 = 
           ((lookup[t4 = (t3 = cptr->cells.twostate.edges1) >> 4 & 0x3f
              | (t2 & 0xf8000000) >> 21 | t3 & 0x20800
              | (t2 & 0xf80000) >> 7] << 4
           | lookup[t5 = t3 & 0x3f
              | (t2 & 0x1f000000) >> 17 | (t3 & 0x400) >> 4
              | (t2 & 0x1f0000 | t3 & 0x8000) >> 3]) << 24
           | (lookup[t4 = t4 >> 6 | (t2 & 0xf800 | t3 & 0x10000) << 1] << 4
              | lookup[t5 = t5 >> 6 | (t2 & 0x1f00) << 5 | (t3 & 0x2000) >> 1]) << 16
           | (lookup[t4 = t4 >> 6 | (t2 & 0xf8) << 9 | (t3 & 0x4000) << 3] << 4
              | lookup[t5 = t5 >> 6 | (t2 & 0x1f) << 13 | t3 & 0x1000]) << 8
           | lookup[t4 = t4 >> 6 | (t1 & 0xf8000000) >> 15 | (t3 & 0x40000) >> 1] << 4
              | lookup[t5 = t5 >> 6 | (t1 & 0x1f000000 | t3 & 0x800000) >> 11]) ^ invmask;

        cptr->cells.twostate.live1 = t1 = 
           ((lookup[t4 = t4 >> 6 | (t1 & 0xf80000 | t3 & 0x1000000) >> 7] << 4
              | lookup[t5 = t5 >> 6 | (t1 & 0x1f0000) >> 3 | (t3 & 0x200000) >> 9]) << 24
           | (lookup[t4 = t4 >> 6 | (t1 & 0xf800 | (t3 = cptr->cells.twostate.edges2) & 0x10000) << 1] << 4
              | lookup[t5 = (t5 | t3 & 0x40000) >> 6 | (t1 & 0x1f00) << 5]) << 16
           | (lookup[t4 = t4 >> 6 | (t1 & 0xf8) << 9 | t3 & 0x20000] << 4
              | lookup[t5 = t5 >> 6 | (t1 & 0x1f) << 13 | t3 & 0x1000]) << 8
           | lookup[t4 = t4 >> 6 | (t3 & 0x3f0) << 8] << 4
              | lookup[t5 = t5 >> 6 | (t3 & 0x3f) << 12]) ^ invmask;
        cptr->cells.twostate.edges1 = cptr->cells.twostate.edges2 = 0;
        /*
         * Box-aging and statistics gathering.
         */
        t2 = cptr->cells.twostate.live2;
        gather_statistics();
      }
    }
    if (live_total == 0 || context->chgcount == 0)
        state = STOP;
}

#define get_live12() {\
    if (maxstates <= 4) {\
        t1 = cptr->cells.gstate.live[0][0] | cptr->cells.gstate.live[0][1] << 2 | cptr->cells.gstate.live[1][0] << 4 | cptr->cells.gstate.live[1][1] << 6;\
        t2 = cptr->cells.gstate.live[2][0] | cptr->cells.gstate.live[2][1] << 2 | cptr->cells.gstate.live[3][0] << 4 | cptr->cells.gstate.live[3][1] << 6;\
        live1 = tab6[t1&0xffff] | tab6[t1 >> 16] << 2\
            | tab6[t2&0xffff] << 16 | tab6[t2 >> 16] << 18;\
        t1 = cptr->cells.gstate.live[4][0] | cptr->cells.gstate.live[4][1] << 2 | cptr->cells.gstate.live[5][0] << 4 | cptr->cells.gstate.live[5][1] << 6;\
        t2 = cptr->cells.gstate.live[6][0] | cptr->cells.gstate.live[6][1] << 2 | cptr->cells.gstate.live[7][0] << 4 | cptr->cells.gstate.live[7][1] << 6;\
        live2 = tab6[t1&0xffff] | tab6[t1 >> 16] << 2\
            | tab6[t2&0xffff] << 16 | tab6[t2 >> 16] << 18;\
    }\
    else if (maxstates <= 16) {\
        t1 = cptr->cells.gstate.live[0][0] | cptr->cells.gstate.live[0][1] << 4;\
        t2 = cptr->cells.gstate.live[1][0] | cptr->cells.gstate.live[1][1] << 4;\
        t3 = cptr->cells.gstate.live[2][0] | cptr->cells.gstate.live[2][1] << 4;\
        live1 = cptr->cells.gstate.live[3][0] | cptr->cells.gstate.live[3][1] << 4;\
        live1 = tab4[t1&0xffff] | tab4[t1 >> 16] << 2\
            | tab4[t2&0xffff] << 8 | tab4[t2 >> 16] << 10\
            | tab4[t3&0xffff] << 16 | tab4[t3 >> 16] << 18\
            | tab4[live1&0xffff] << 24 | tab4[live1 >> 16] << 26;\
        t1 = cptr->cells.gstate.live[4][0] | cptr->cells.gstate.live[4][1] << 4;\
        t2 = cptr->cells.gstate.live[5][0] | cptr->cells.gstate.live[5][1] << 4;\
        t3 = cptr->cells.gstate.live[6][0] | cptr->cells.gstate.live[6][1] << 4;\
        live2 = cptr->cells.gstate.live[7][0] | cptr->cells.gstate.live[7][1] << 4;\
        live2 = tab4[t1&0xffff] | tab4[t1 >> 16] << 2\
            | tab4[t2&0xffff] << 8 | tab4[t2 >> 16] << 10\
            | tab4[t3&0xffff] << 16 | tab4[t3 >> 16] << 18\
            | tab4[live2&0xffff] << 24 | tab4[live2 >> 16] << 26;\
    }\
    else {\
       live1 = tab4[cptr->cells.gstate.slive[0][0]]\
            | tab4[cptr->cells.gstate.slive[0][1]] << 2\
            | tab4[cptr->cells.gstate.slive[0][2]] << 4\
            | tab4[cptr->cells.gstate.slive[0][3]] << 6\
            | tab4[cptr->cells.gstate.slive[1][0]] << 8\
            | tab4[cptr->cells.gstate.slive[1][1]] << 10\
            | tab4[cptr->cells.gstate.slive[1][2]] << 12\
            | tab4[cptr->cells.gstate.slive[1][3]] << 14\
            | tab4[cptr->cells.gstate.slive[2][0]] << 16\
            | tab4[cptr->cells.gstate.slive[2][1]] << 18\
            | tab4[cptr->cells.gstate.slive[2][2]] << 20\
            | tab4[cptr->cells.gstate.slive[2][3]] << 22\
            | tab4[cptr->cells.gstate.slive[3][0]] << 24\
            | tab4[cptr->cells.gstate.slive[3][1]] << 26\
            | tab4[cptr->cells.gstate.slive[3][2]] << 28\
            | tab4[cptr->cells.gstate.slive[3][3]] << 30;\
       live2 = tab4[cptr->cells.gstate.slive[4][0]]\
            | tab4[cptr->cells.gstate.slive[4][1]] << 2\
            | tab4[cptr->cells.gstate.slive[4][2]] << 4\
            | tab4[cptr->cells.gstate.slive[4][3]] << 6\
            | tab4[cptr->cells.gstate.slive[5][0]] << 8\
            | tab4[cptr->cells.gstate.slive[5][1]] << 10\
            | tab4[cptr->cells.gstate.slive[5][2]] << 12\
            | tab4[cptr->cells.gstate.slive[5][3]] << 14\
            | tab4[cptr->cells.gstate.slive[6][0]] << 16\
            | tab4[cptr->cells.gstate.slive[6][1]] << 18\
            | tab4[cptr->cells.gstate.slive[6][2]] << 20\
            | tab4[cptr->cells.gstate.slive[6][3]] << 22\
            | tab4[cptr->cells.gstate.slive[7][0]] << 24\
            | tab4[cptr->cells.gstate.slive[7][1]] << 26\
            | tab4[cptr->cells.gstate.slive[7][2]] << 28\
            | tab4[cptr->cells.gstate.slive[7][3]] << 30;\
    }\
}

static void evolve3(pattern *context) {
   register u32bits t1, t2, t3, live1, live2;
   u32bits *tmpptr, live_total = 0;
   register tile *cptr;
   tile *cptrup, *cptrdn, *cptrlf, *cptrrt;
   union {
      u32bits olive[BOXSIZE][2];
      cell_t ocell[BOXSIZE][BOXSIZE];
   } ocell;

   for (cptr = context->tiles; cptr; cptr = cptr->next) {
      if (cptr->dead) continue;
      get_live12();
      if ((live1 | live2) == 0) continue;
      adjacency_links(gstate, live1, live2); /* macro */
   }
/*
 * Perform actual evolution.
 */
   fix_plain(cptrup);
   for (cptr = context->tiles; cptr; cptr = cptr->next) {
      memcpy(ocell.ocell, cptr->cells.gstate.cell, 64);
      tmpptr = cptr->cells.gstate.on;
      live2 = live1 = 0;
      for (t1 = 0; t1 < BOXSIZE; t1++) {
         t3 = *tmpptr;
         if (maxstates <= 4) {
            live1 |= cptr->cells.gstate.live[t1][0] = t2 =
               lookup4[(t3 << 8 & 0xffff00) | (cptr->cells.gstate.cell[t1][3] << 6)
                  | (cptr->cells.gstate.cell[t1][2] << 4) | (cptr->cells.gstate.cell[t1][1] << 2)
                  | cptr->cells.gstate.cell[t1][0]];
            live1 |= cptr->cells.gstate.live[t1][1] = t3 =
               lookup4[(t3 >> 8 & 0xffff00) | (cptr->cells.gstate.cell[t1][7] << 6)
                  | (cptr->cells.gstate.cell[t1][6] << 4) | (cptr->cells.gstate.cell[t1][5] << 2)
                  | cptr->cells.gstate.cell[t1][4]];
         }
         else if (maxstates <= 16) {
            live1 |= cptr->cells.gstate.live[t1][0] = t2 =
               lookup2[((t3 & 0xff) << 8) | (cptr->cells.gstate.cell[t1][1] << 4)
                       | cptr->cells.gstate.cell[t1][0]]
                    | lookup2[(t3 & 0xff00) | (cptr->cells.gstate.cell[t1][3] << 4)
                       | cptr->cells.gstate.cell[t1][2]] << 16;
            live1 |= cptr->cells.gstate.live[t1][1] = t3 =
               lookup2[((t3 & 0xff0000) >> 8) | (cptr->cells.gstate.cell[t1][5] << 4)
                       | cptr->cells.gstate.cell[t1][4]]
                    | lookup2[((t3 & 0xff000000) >> 16) | (cptr->cells.gstate.cell[t1][7] << 4)
                       | cptr->cells.gstate.cell[t1][6]] << 16;
         }
         else {
            live1 |= cptr->cells.gstate.live[t1][0] = t2 =
                      lookup[((t3 & 0xf) << 8) | cptr->cells.gstate.cell[t1][0]]
                    | lookup[((t3 & 0xf0) << 4) | cptr->cells.gstate.cell[t1][1]] << 8
                    | lookup[(t3 & 0xf00) | cptr->cells.gstate.cell[t1][2]] << 16
                    | lookup[((t3 & 0xf000) >> 4) | cptr->cells.gstate.cell[t1][3]] << 24;
            live1 |= cptr->cells.gstate.live[t1][1] = t3 =
                      lookup[((t3 & 0xf0000) >> 8) | cptr->cells.gstate.cell[t1][4]]
                    | lookup[((t3 & 0xf00000) >> 12) | cptr->cells.gstate.cell[t1][5]] << 8
                    | lookup[((t3 & 0xf000000) >> 16) | cptr->cells.gstate.cell[t1][6]] << 16
                    | lookup[((t3 & 0xf0000000) >> 20) | cptr->cells.gstate.cell[t1][7]] << 24;
         }
         live2 = live2 || t2 != ocell.olive[t1][0] || t3 != ocell.olive[t1][1];
         *tmpptr++ = 0;
      }
/*
 * Box-aging and statistics gathering.
 */
      if (live2) {
         if (truehistory && context == &active) add2history(cptr);
         if (dispchanges) {
            for (t1 = 0; t1 < BOXSIZE; t1++)
               for (t2 = 0; t2 < BOXSIZE; t2++)
                  if (ocell.ocell[t2][t1] != cptr->cells.gstate.cell[t2][t1])
                     context->chgcount++;
         }
         else
            context->chgcount++;
      }
      if (live1) {
         cptr->dead = 0;
         if (oscillators) osc_values(context, cptr);
         live_total++;
         if (dispboxes)
            for (t1 = 0; t1 < BOXSIZE; t1++)
               for (t2 = 0; t2 < BOXSIZE; t2++)
                  if (t3 = cptr->cells.gstate.cell[t2][t1])
                     context->cellcount[t3]++;
      }
      else
         cptr->dead++;
   }
   if (live_total == 0 || context->chgcount == 0)
      state = STOP;
}

static u8bits tr1[7][7] = {
     {1, 0, 1, 255, 255, 255, 255}, {1, 2, 1, 255, 255, 255, 255}, {255, 2, 1, 255, 255, 255, 255},
     {3, 4, 255, 4, 3, 255, 255}, {255, 4, 3, 4, 3, 255, 255}, {5, 4, 255, 255, 5, 4, 255},
     {6, 6, 6, 6, 6, 6, 6}},
   tr2[7][7] = {
     {255, 1, 0, 255, 255, 255, 255}, {2, 1, 255, 255, 255, 255, 255}, {255, 1, 2, 255, 255, 255, 255},
     {4, 3, 255, 255, 255, 255, 255}, {255, 3, 4, 255, 255, 255, 255}, {4, 5, 255, 255, 255, 255, 255},
     {6, 6, 6, 6, 6, 6, 6}};

static void evolve3b0(pattern *context) {
    register u32bits t1, t2, t3, live1, live2;
    u32bits *tmpptr, live_total = 0;
    register tile *cptr;
    tile *cptrup, *cptrdn, *cptrlf, *cptrrt;
    union {
      u32bits olive[BOXSIZE][2];
      cell_t  ocell[BOXSIZE][BOXSIZE];
    } ocell;

    cptr = context->tiles;
    if (context->cellmass) {
      int live8st = !!(live & 0x100);
      tile *cptrul, *cptrur, *cptrdl, *cptrdr;

      for (; cptr; cptr = cptr->next) {
        get_live12();
        if ((live1 | live2) == 0) continue;
        maketile(context, cptr->x, cptr->y - BOXSIZE);
        maketile(context, cptr->x, cptr->y + BOXSIZE);
        maketile(context, cptr->x - BOXSIZE, cptr->y);
        maketile(context, cptr->x - BOXSIZE, cptr->y - BOXSIZE);
        maketile(context, cptr->x - BOXSIZE, cptr->y + BOXSIZE);
        maketile(context, cptr->x + BOXSIZE, cptr->y);
        maketile(context, cptr->x + BOXSIZE, cptr->y - BOXSIZE);
        maketile(context, cptr->x + BOXSIZE, cptr->y + BOXSIZE);
      }
      for (cptr = context->tiles; cptr; cptr = cptr->next) {
        memcpy(ocell.ocell, cptr->cells.gstate.cell, 64);
        for (t1 = 0; t1 < BOXSIZE; t1++)
           for (t2 = 0; t2 < 2; t2++)
              cptr->cells.gstate.live[t1][t2] = cptr->cells.gstate.live[t1][t2]&0x1010101 ^ 0x1010101;
        get_live12();
        for (t1 = 0; t1 < BOXSIZE; t1++)
           for (t2 = 0; t2 < 2; t2++)
              cptr->cells.gstate.live[t1][t2] += ocell.olive[t1][t2] << 4;
        cptrup = fetchtile(context, cptr->x, cptr->y - BOXSIZE);
        cptrdn = fetchtile(context, cptr->x, cptr->y + BOXSIZE);
        cptrlf = fetchtile(context, cptr->x - BOXSIZE, cptr->y);
        cptrrt = fetchtile(context, cptr->x + BOXSIZE, cptr->y);
        cptrul = fetchtile(context, cptr->x - BOXSIZE, cptr->y - BOXSIZE);
        cptrur = fetchtile(context, cptr->x + BOXSIZE, cptr->y - BOXSIZE);
        cptrdl = fetchtile(context, cptr->x - BOXSIZE, cptr->y + BOXSIZE);
        cptrdr = fetchtile(context, cptr->x + BOXSIZE, cptr->y + BOXSIZE);
        if (!cptrup) cptr->cells.gstate.on[0] += 0x23333332;
        if (!cptrdn) cptr->cells.gstate.on[7] += 0x23333332;
        if (!cptrlf) {
           cptr->cells.gstate.on[0] += 2;
           cptr->cells.gstate.on[1] += 3;
           cptr->cells.gstate.on[2] += 3;
           cptr->cells.gstate.on[3] += 3;
           cptr->cells.gstate.on[4] += 3;
           cptr->cells.gstate.on[5] += 3;
           cptr->cells.gstate.on[6] += 3;
           cptr->cells.gstate.on[7] += 2;
        }
        if (!cptrrt) {
           cptr->cells.gstate.on[0] += 0x20000000;
           cptr->cells.gstate.on[1] += 0x30000000;
           cptr->cells.gstate.on[2] += 0x30000000;
           cptr->cells.gstate.on[3] += 0x30000000;
           cptr->cells.gstate.on[4] += 0x30000000;
           cptr->cells.gstate.on[5] += 0x30000000;
           cptr->cells.gstate.on[6] += 0x30000000;
           cptr->cells.gstate.on[7] += 0x20000000;
        }
        if (!cptrul) cptr->cells.gstate.on[0] += 1;
        if (!cptrur) cptr->cells.gstate.on[0] += 0x10000000;
        if (!cptrdl) cptr->cells.gstate.on[7] += 1;
        if (!cptrdr) cptr->cells.gstate.on[7] += 0x10000000;
        if ((live1 | live2) == 0)
            continue;

        t1 = live1 &0xff;
        if (t1) {
            t2 = tab1[t1];
            if (cptrup) cptrup->cells.gstate.on[7] += t2;
            cptr->cells.gstate.on[1] += t2;
            cptr->cells.gstate.on[0] += tab2[t1];
        }

        t1 = (live2 & 0xff000000) >> 24;
        if (t1) {
            t2 = tab1[t1];
            if (cptrdn) cptrdn->cells.gstate.on[0] += t2;
            cptr->cells.gstate.on[6] += t2;
            cptr->cells.gstate.on[7] += tab2[t1];
        }

        t1 = live1;
        t2 = live2;
        if (t1&0x1010101 && cptrlf) {
            if (t1 & 0x1) {
                cptrlf->cells.gstate.on[0] += 0x10000000;
                cptrlf->cells.gstate.on[1] += 0x10000000;
            }
            if (t1 & 0x100) {
                cptrlf->cells.gstate.on[0] += 0x10000000;
                cptrlf->cells.gstate.on[1] += 0x10000000;
                cptrlf->cells.gstate.on[2] += 0x10000000;
            }
            if (t1 & 0x10000) {
                cptrlf->cells.gstate.on[1] += 0x10000000;
                cptrlf->cells.gstate.on[2] += 0x10000000;
                cptrlf->cells.gstate.on[3] += 0x10000000;
            }
            if (t1 & 0x1000000) {
                cptrlf->cells.gstate.on[2] += 0x10000000;
                cptrlf->cells.gstate.on[3] += 0x10000000;
                cptrlf->cells.gstate.on[4] += 0x10000000;
            }
        }
        if (t1&0x1 && cptrul) cptrul->cells.gstate.on[7] += 0x10000000;

        if (t2&0x1010101 && cptrlf) {
            if (t2 & 0x1) {
                cptrlf->cells.gstate.on[3] += 0x10000000;
                cptrlf->cells.gstate.on[4] += 0x10000000;
                cptrlf->cells.gstate.on[5] += 0x10000000;
            }
            if (t2 & 0x100) {
                cptrlf->cells.gstate.on[4] += 0x10000000;
                cptrlf->cells.gstate.on[5] += 0x10000000;
                cptrlf->cells.gstate.on[6] += 0x10000000;
            }
            if (t2 & 0x10000) {
                cptrlf->cells.gstate.on[5] += 0x10000000;
                cptrlf->cells.gstate.on[6] += 0x10000000;
                cptrlf->cells.gstate.on[7] += 0x10000000;
            }
            if (t2 & 0x1000000) {
                cptrlf->cells.gstate.on[6] += 0x10000000;
                cptrlf->cells.gstate.on[7] += 0x10000000;
            }
        }
        if (t2&0x1000000 && cptrdl) cptrdl->cells.gstate.on[0] += 0x10000000;

        if (t1&0x80808080 && cptrrt) {
            if (t1 & 0x80) {
                cptrrt->cells.gstate.on[0] += 0x1;
                cptrrt->cells.gstate.on[1] += 0x1;
            }
            if (t1 & 0x8000) {
                cptrrt->cells.gstate.on[0] += 0x1;
                cptrrt->cells.gstate.on[1] += 0x1;
                cptrrt->cells.gstate.on[2] += 0x1;
            }
            if (t1 & 0x800000) {
                cptrrt->cells.gstate.on[1] += 0x1;
                cptrrt->cells.gstate.on[2] += 0x1;
                cptrrt->cells.gstate.on[3] += 0x1;
            }
            if (t1 & 0x80000000) {
                cptrrt->cells.gstate.on[2] += 0x1;
                cptrrt->cells.gstate.on[3] += 0x1;
                cptrrt->cells.gstate.on[4] += 0x1;
            }
        }
        if (t1&0x80 && cptrur) cptrur->cells.gstate.on[7] += 0x1;

        if (t2&0x80808080 && cptrrt) {
            if (t2 & 0x80) {
                cptrrt->cells.gstate.on[3] += 0x1;
                cptrrt->cells.gstate.on[4] += 0x1;
                cptrrt->cells.gstate.on[5] += 0x1;
            }
            if (t2 & 0x8000) {
                cptrrt->cells.gstate.on[4] += 0x1;
                cptrrt->cells.gstate.on[5] += 0x1;
                cptrrt->cells.gstate.on[6] += 0x1;
            }
            if (t2 & 0x800000) {
                cptrrt->cells.gstate.on[5] += 0x1;
                cptrrt->cells.gstate.on[6] += 0x1;
                cptrrt->cells.gstate.on[7] += 0x1;
            }
            if (t2 & 0x80000000) {
                cptrrt->cells.gstate.on[6] += 0x1;
                cptrrt->cells.gstate.on[7] += 0x1;
            }
        }
        if (t2&0x80000000 && cptrdr) cptrdr->cells.gstate.on[0] += 0x1;

        t1 = (live1 & 0xff00) >> 8;
        t2 = (live1 & 0xff0000) >> 16;
        if (t1) {
            t3 = tab1[t1];
            cptr->cells.gstate.on[1] += tab2[t1];
            cptr->cells.gstate.on[0] += t3;
            cptr->cells.gstate.on[2] += t3;
        }

        t1 = (live1 & 0xff000000) >> 24;
        if (t2) {
            t3 = tab1[t2];
            cptr->cells.gstate.on[2] += tab2[t2];
            cptr->cells.gstate.on[1] += t3;
            cptr->cells.gstate.on[3] += t3;
        }

        t2 = (live2 & 0xff);
        if (t1) {
            t3 = tab1[t1];
            cptr->cells.gstate.on[3] += tab2[t1];
            cptr->cells.gstate.on[2] += t3;
            cptr->cells.gstate.on[4] += t3;
        }

        t1 = (live2 & 0xff00) >> 8;
        if (t2) {
            t3 = tab1[t2];
            cptr->cells.gstate.on[4] += tab2[t2];
            cptr->cells.gstate.on[3] += t3;
            cptr->cells.gstate.on[5] += t3;
        }

        t2 = (live2 & 0xff0000) >> 16;
        if (t1) {
            t3 = tab1[t1];
            cptr->cells.gstate.on[5] += tab2[t1];
            cptr->cells.gstate.on[4] += t3;
            cptr->cells.gstate.on[6] += t3;
        }
        if (t2) {
            t3 = tab1[t2];
            cptr->cells.gstate.on[6] += tab2[t2];
            cptr->cells.gstate.on[5] += t3;
            cptr->cells.gstate.on[7] += t3;
        }
        cptr->up = cptrup;
        cptr->dn = cptrdn;
        cptr->lf = cptrlf;
        cptr->rt = cptrrt;
      }
      /*
      * Perform actual evolution.
      */
      fix_plain(cptrup);
      context->cellmass = live8st;
      for (cptr = context->tiles; cptr; cptr = cptr->next) {
        for (t1 = 0; t1 < BOXSIZE; t1++)
           for (t2 = 0; t2 < 2; t2++) {
              ocell.olive[t1][t2] = cptr->cells.gstate.live[t1][t2] >> 4 & 0xf0f0f0f;
              cptr->cells.gstate.live[t1][t2] &= 0xf0f0f0f;
           }
        tmpptr = cptr->cells.gstate.on;
        for (t1 = 0; t1 < BOXSIZE; t1++) {
           t3 = *tmpptr;
           cptr->cells.gstate.live[t1][0] =
                  lookup2[((t3 & 0xff) << 8) | (cptr->cells.gstate.cell[t1][1] << 4)
                       | cptr->cells.gstate.cell[t1][0]]
                    | lookup2[(t3 & 0xff00) | (cptr->cells.gstate.cell[t1][3] << 4)
                       | cptr->cells.gstate.cell[t1][2]] << 16;
           cptr->cells.gstate.live[t1][1] =
                  lookup2[((t3 & 0xff0000) >> 8) | (cptr->cells.gstate.cell[t1][5] << 4)
                       | cptr->cells.gstate.cell[t1][4]]
                    | lookup2[((t3 & 0xff000000) >> 16) | (cptr->cells.gstate.cell[t1][7] << 4)
                       | cptr->cells.gstate.cell[t1][6]] << 16;
           *tmpptr++ = 0;
        }
        if (live8st)
          for (t1 = 0; t1 < BOXSIZE; t1++)
             for (t2 = 0; t2 < BOXSIZE; t2++)
                cptr->cells.gstate.cell[t1][t2]
                   = tr1[ocell.ocell[t1][t2]][cptr->cells.gstate.cell[t1][t2]];
        else
          for (t1 = 0; t1 < BOXSIZE; t1++)
             for (t2 = 0; t2 < BOXSIZE; t2++)
                cptr->cells.gstate.cell[t1][t2]
                   = tr2[ocell.ocell[t1][t2]][cptr->cells.gstate.cell[t1][t2]];
        live2 = live1 = 0;
        for (t1 = 0; t1 < BOXSIZE; t1++)
           for (t2 = 0; t2 < 2; t2++) {
              live1 = live1 || cptr->cells.gstate.live[t1][t2];
              live2 = live2 || cptr->cells.gstate.live[t1][t2] != ocell.olive[t1][t2];
           }
        /*
         * Box-aging and statistics gathering.
         */
        if (live2)
           if (dispchanges) {
             for (t1 = 0; t1 < BOXSIZE; t1++)
                for (t2 = 0; t2 < BOXSIZE; t2++)
                   if (ocell.ocell[t2][t1] != cptr->cells.gstate.cell[t2][t1])
                     context->chgcount++;
           }
           else
             context->chgcount++;
        if (live1) {
          if (oscillators) osc_values(context, cptr);
          cptr->dead = 0;
          live_total++;
          if (dispboxes)
             for (t1 = 0; t1 < BOXSIZE; t1++)
                for (t2 = 0; t2 < BOXSIZE; t2++)
                   if (t3 = cptr->cells.gstate.cell[t2][t1])
                     context->cellcount[t3]++;
        }
        else
            cptr->dead++;
      }
    }
    else {
      for (; cptr; cptr = cptr->next) {
        t1 = cptr->cells.gstate.live[0][0] | cptr->cells.gstate.live[0][1] << 4;
        t2 = cptr->cells.gstate.live[1][0] | cptr->cells.gstate.live[1][1] << 4;
        t3 = cptr->cells.gstate.live[2][0] | cptr->cells.gstate.live[2][1] << 4;
        live1 = cptr->cells.gstate.live[3][0] | cptr->cells.gstate.live[3][1] << 4;
        live1 = tab4[t1&0xffff] | tab4[t1 >> 16] << 2
            | tab4[t2&0xffff] << 8 | tab4[t2 >> 16] << 10
            | tab4[t3&0xffff] << 16 | tab4[t3 >> 16] << 18
            | tab4[live1&0xffff] << 24 | tab4[live1 >> 16] << 26;
        t1 = cptr->cells.gstate.live[4][0] | cptr->cells.gstate.live[4][1] << 4;
        t2 = cptr->cells.gstate.live[5][0] | cptr->cells.gstate.live[5][1] << 4;
        t3 = cptr->cells.gstate.live[6][0] | cptr->cells.gstate.live[6][1] << 4;
        live2 = cptr->cells.gstate.live[7][0] | cptr->cells.gstate.live[7][1] << 4;
        live2 = tab4[t1&0xffff] | tab4[t1 >> 16] << 2
            | tab4[t2&0xffff] << 8 | tab4[t2 >> 16] << 10
            | tab4[t3&0xffff] << 16 | tab4[t3 >> 16] << 18
            | tab4[live2&0xffff] << 24 | tab4[live2 >> 16] << 26;
        if ((live1 | live2) == 0)
            continue;
        adjacency_links(gstate, live1, live2); /* macro */
      }

      /*
      * Perform actual evolution.
      */
      fix_plain(cptrup);
      context->cellmass = 1;
      for (cptr = context->tiles; cptr; cptr = cptr->next) {
        memcpy(ocell.ocell, cptr->cells.gstate.cell, 64);
        tmpptr = cptr->cells.gstate.on;
        for (t1 = 0; t1 < BOXSIZE; t1++) {
           t3 = *tmpptr;
           cptr->cells.gstate.live[t1][0] =
                  lookup2[((t3 & 0xff) << 8) | (cptr->cells.gstate.cell[t1][1] << 4)
                       | cptr->cells.gstate.cell[t1][0]]
                    | lookup2[(t3 & 0xff00) | (cptr->cells.gstate.cell[t1][3] << 4)
                       | cptr->cells.gstate.cell[t1][2]] << 16;
           cptr->cells.gstate.live[t1][1] =
                  lookup2[((t3 & 0xff0000) >> 8) | (cptr->cells.gstate.cell[t1][5] << 4)
                       | cptr->cells.gstate.cell[t1][4]]
                    | lookup2[((t3 & 0xff000000) >> 16) | (cptr->cells.gstate.cell[t1][7] << 4)
                       | cptr->cells.gstate.cell[t1][6]] << 16;
           *tmpptr++ = 0;
        }
        for (t1 = 0; t1 < BOXSIZE; t1++)
           for (t2 = 0; t2 < BOXSIZE; t2++)
              cptr->cells.gstate.cell[t1][t2]
                 = tr1[ocell.ocell[t1][t2]][cptr->cells.gstate.cell[t1][t2]];
        live2 = live1 = 0;
        for (t1 = 0; t1 < BOXSIZE; t1++)
           for (t2 = 0; t2 < 2; t2++) {
              live1 = live1 || cptr->cells.gstate.live[t1][t2];
              live2 = live2 || cptr->cells.gstate.live[t1][t2] != ocell.olive[t1][t2];
           }
        /*
         * Box-aging and statistics gathering.
         */
        if (live2)
           if (dispchanges) {
             for (t1 = 0; t1 < BOXSIZE; t1++)
                for (t2 = 0; t2 < BOXSIZE; t2++)
                   if (ocell.ocell[t2][t1] != cptr->cells.gstate.cell[t2][t1])
                     context->chgcount++;
           }
           else
             context->chgcount++;
        if (live1) {
          cptr->dead = 0;
          if (oscillators) osc_values(context, cptr);
          live_total++;
          if (dispboxes)
             for (t1 = 0; t1 < BOXSIZE; t1++)
                for (t2 = 0; t2 < BOXSIZE; t2++)
                   if (t3 = cptr->cells.gstate.cell[t2][t1])
                     context->cellcount[t3]++;
        }
        else
            cptr->dead++;
      }
    }
    if (live_total == 0 || context->chgcount == 0)
        state = STOP;
}

static void setinipattern() {
   tile *ptr;
   int dx, dy;

   FOR_CELLS(&active, ptr, dx, dy)
      if (getcell(&ptr->cells, dx, dy) == 1)
         setcell(&ptr->cells, dx, dy, 5);
}

int file_rules(const char* ptr) { /* get and accept 2-state/generated/named/pd rules from file's #T line */
   const char *optr = ptr;
   char *p, b[10], s[10];
   int hm = historymode;
   unsigned long cc;
   historymode = 0;
   if (palettemod) DefaultPalette();
   palettemod = 0;
   if (!strcasecmp(ptr, "wireworld")) {
      set_active_rules(ptr, 1);
      born = 6;
      alloc_states(GEN_DRIVEN, 4);
      gentab3();
      genatab4();
   }
   else if (!strcmp(ptr, "lifehistory") || !strcasecmp(ptr, "life+")) {
      born = 8;
      live = 12;
      goto l3;
   }
   else if (!strncasecmp(ptr, "brian", 5)) {
      born = 4;
      live = 0;
      alloc_states(GEN_DRIVEN, 3);
      goto l2;
   }
   else if (!strcasecmp(ptr, "life")) {
      born = 8;
      live = 12;
      goto l1;
   }
   else if (strchr(ptr, '$')) {
      set_active_rules(ptr, 1);
      if (sscanf(ptr, "%f$%f", &payoffs[DEFECT][COOPERATE],
                                   &payoffs[COOPERATE][DEFECT]) != 2) return 1;
      alloc_states(PAYOFF_DRIVEN, 5); /* must alloc 5 states to get pseudocolors */
      sprintf(active_rules, "%.2f$%.2f",
      payoffs[DEFECT][COOPERATE], payoffs[COOPERATE][DEFECT]);
      set_active_rules(active_rules, 1);
   }
   else if (sscanf(ptr, " %*[sS]%[0-9] %*[bB]%[0-9] ", s, b) == 2
                     || sscanf(ptr, " %*[sS]%[0-9] / %*[bB]%[0-9] ", s, b) == 2
                     || sscanf(ptr, " %*[bB]%[0-9] / %*[sS]%[0-9] ", b, s) == 2
                    || sscanf(ptr, " %*[bB]%[0-9] %*[sS]%[0-9] ", b, s) == 2) {
      p = s;
      born = live = 0;
      while (*p >= '0' && *p <= '9')
         live |= 1 << *p++ - '0';
      p = b;
      while (*p >= '0' && *p <= '9')
         born |= 1 << *p++ - '0';
      sprintf((char*)ptr, "B%s/S%s", b, s);
      goto l1;
   }
   else if (strchr(ptr, '/')) {
      born = live = 0;
      while (*ptr >= '0' && *ptr <= '9')
         live |= 1 << *ptr++ - '0';
      if (*ptr == '/') {
         ptr++;
         while (*ptr >= '0' && *ptr <= '9')
            born |= 1 << *ptr++ - '0';
      }
      if (optr == ptr) return 1;
      if (*ptr == '/') {
         unsigned agelimit;
         if (sscanf(++ptr, "%u", &agelimit) != 1 || agelimit > MAXSTATES
                                             || agelimit < 3 || agelimit > 256)
            return 1;
         if (born & 1) return 1; /* B0 is not supported */
         set_active_rules(optr, 1);
         alloc_states(GEN_DRIVEN, agelimit);
         if (born == 4 && live == 0 && maxstates == 3)
l2:         set_active_rules("brian's brain", 1);
         gentab2();
         genatab4();
      }
      else if (*ptr == '+') {
l3:
         cc = active.generations;
         alloc_states(GEN_DRIVEN, 7);
         active.generations = cc;
         if (born == 8 && live == 12)
            set_active_rules("life+", 1);
         else
            set_active_rules(optr, 1);
         historymode = 1;
         setinipattern();
         gentab4();
         genatab5();
         HistoryPalette();
         palettemod = 1;
      }
      else {
l1:
         if (hm) cc = active.generations;
         alloc_states(VALENCE_DRIVEN, 2);
         if (hm) active.generations = cc;
         if (born == 8 && live == 12)
            set_active_rules("life", 1);
         else if (born == 4 && live == 0)
            set_active_rules("seeds", 1);
         else
            set_active_rules(optr, 1);
         if (active.cellmass) active.cellmass = born&1;
         gentab();
      }
   }
   else
      return 10;
   showrules();
   set_paintcolor();
   return *pathtorules = 0;
}

void newrules(char *nrs) {
/* prompt for and accept 2-state, 3-component, or named rules */
    int i, k;
    char *ptr = inpbuf, *p, ptrbuf[RULESNAMELEN];
    if (nrs != 0 && nrs[0] != 0) {
       strcpy(ptrbuf, nrs);
       goto L1;
    }
    strcpy(ptr, "Rules: ");
    ptr = ptr + 7;
    if (ev_mode == VALENCE_DRIVEN) {
	k = live;
	for (i = 0; i < 9; i++) {
	    if (k&1)
		*ptr++ = i + '0';
	    k >>= 1;
	}
	*ptr++ = '/';
	k = born;
	for (i = 0; i < 9; i++) {
	    if (k&1)
		*ptr++ = i + '0';
	    k >>= 1;
	}
    }
    else {
        strcpy(ptr, active_rules);
        ptr += strlen(ptr);
    }
    strcpy(ptr, "   New Rules: ");
    minbuflen = strlen(inpbuf);
    DoExpose(inputw);
    getxstring();
    if (*strcpy(ptrbuf, inpbuf + minbuflen) == 0)
       return;
L1:
    if (p = strrchr(ptrbuf, ':')) *p = 0;
    if (file_rules(ptrbuf)) {
       sprintf(inpbuf, "Can't set rules %s", ptrbuf);
       announce_and_wait(RED_EVER);
       if (*pathtorules)
         use_rules(pathtorules, 0);
       else
         file_rules(active_rules);
    }
    else if (p)
       if (set_topology(p + 1) > 0) {
              sprintf(inpbuf, "Can't set topology %s", p + 1);
              announce_and_wait(RED_EVER);
       }
       else
       {
              center();
              if (bounding_box(&active))
                   make_tentative(active.xmin, active.ymin, active.xmax, active.ymax);
              redraw_lifew();
       }
}


/* now the n-state case... */
/*
 * These macros unroll a bunch of loops inside the tile-evolution function.
 * They aren't parametrized for BOXSIZE, but have the double benefit of
 * eliminating loop overhead and allowing the optimizer to fold constant
 * array references. On a machine with character-addressable memory the
 * results ought to run like a banshee.
 */
/* first, macros to test whether a tile edge is changed */
#define V_CHNG(h) (cptr->cells.pstate.ocell[h][1] != cptr->cells.pstate.cell[h][1] \
  | cptr->cells.pstate.ocell[h][2] != cptr->cells.pstate.cell[h][2] \
  | cptr->cells.pstate.ocell[h][3] != cptr->cells.pstate.cell[h][3] \
  | cptr->cells.pstate.ocell[h][4] != cptr->cells.pstate.cell[h][4] \
  | cptr->cells.pstate.ocell[h][5] != cptr->cells.pstate.cell[h][5] \
  | cptr->cells.pstate.ocell[h][6] != cptr->cells.pstate.cell[h][6])
/*#define V_CHNG(h) (memcmp(cptr->cells.pstate.ocell[h]+1, cptr->cells.pstate.cell[h]+1, 6))*/
#define H_CHNG(r) (cptr->cells.pstate.ocell[1][r] != cptr->cells.pstate.cell[1][r] \
  | cptr->cells.pstate.ocell[2][r] != cptr->cells.pstate.cell[2][r] \
  | cptr->cells.pstate.ocell[3][r] != cptr->cells.pstate.cell[3][r] \
  | cptr->cells.pstate.ocell[4][r] != cptr->cells.pstate.cell[4][r] \
  | cptr->cells.pstate.ocell[5][r] != cptr->cells.pstate.cell[5][r] \
  | cptr->cells.pstate.ocell[6][r] != cptr->cells.pstate.cell[6][r])
#define X_CHNG(r,h) (cptr->cells.pstate.ocell[r][h] != cptr->cells.pstate.cell[r][h])

/*
 * Offsets of current tile in hold area.  The idea here is that we need to copy
 * enough of the surrounding tiles into the hold area to do all resolution.
 * For table-driven functions one edge row suffices; for payoff-driven
 * functions, two edge rows/columns are needed (because we need to compute the
 * payoffs of the inner edge cells).
 */
#define HX	2
#define HY	2

/* macros to grab a row or column out of a tile and put it in the hold area */
#define GET_COL(tcell, tr, fcell, fr) { \
	tcell[HY+0][tr] = fcell[0][fr]; \
	tcell[HY+1][tr] = fcell[1][fr]; \
	tcell[HY+2][tr] = fcell[2][fr]; \
	tcell[HY+3][tr] = fcell[3][fr]; \
	tcell[HY+4][tr] = fcell[4][fr]; \
	tcell[HY+5][tr] = fcell[5][fr]; \
	tcell[HY+6][tr] = fcell[6][fr]; \
	tcell[HY+7][tr] = fcell[7][fr]; \
}
#define GET_ROW(tcell, tc, fcell, fr) { \
	tcell[tc][HX+0] = fcell[fr][0]; \
	tcell[tc][HX+1] = fcell[fr][1]; \
	tcell[tc][HX+2] = fcell[fr][2]; \
	tcell[tc][HX+3] = fcell[fr][3]; \
	tcell[tc][HX+4] = fcell[fr][4]; \
	tcell[tc][HX+5] = fcell[fr][5]; \
	tcell[tc][HX+6] = fcell[fr][6]; \
	tcell[tc][HX+7] = fcell[fr][7]; \
}

/* fill the hold area from a tile */
#define GET_INTERIOR(to, from) { \
      GET_COL(to, HX+0, from, 0); \
      GET_COL(to, HX+1, from, 1); \
      GET_COL(to, HX+2, from, 2); \
      GET_COL(to, HX+3, from, 3); \
      GET_COL(to, HX+4, from, 4); \
      GET_COL(to, HX+5, from, 5); \
      GET_COL(to, HX+6, from, 6); \
      GET_COL(to, HX+7, from, 7); \
}

float compute_payoff(int me, int you) { //static float compute_payoff(int me, int you) {
/* compute payoff function for Prisoner's Dilemma simulations */
   if (you == 0 || me == 0)
      return 0;
   else
      return payoffs[me - 1][you - 1];
}

static cell_t	hold[HY + BOXSIZE + HY][HX + BOXSIZE + HX];

static void get_corner(int dy, int dx, tile *cp, int sy, int sx) {
/* fetch a tile corner into the hold area */
   hold[dy  ][dx  ] = cp->cells.pstate.ocell[sy  ][sx  ];
   hold[dy+1][dx  ] = cp->cells.pstate.ocell[sy+1][sx  ];
   hold[dy  ][dx+1] = cp->cells.pstate.ocell[sy  ][sx+1];
   hold[dy+1][dx+1] = cp->cells.pstate.ocell[sy+1][sx+1];
}

static cell_t bad_state_handler8(tile *cptr, int yc, int xc, cell_t u,
                 cell_t ur, cell_t r, cell_t dr, cell_t d, cell_t dl, cell_t l,
                                                    cell_t ul, cell_t oldval) {
   cell_t newval;
   FILE *fp;
   drawbox(cptr->x + xc, cptr->y + yc, cptr->x + xc + 2, cptr->y + yc + 2,
                                                                   WHITE_EVER);
   state = STOP;
   newval = patch_transition8(oldval, u, ur, r, dr, d, dl, l, ul);
   drawcell(RXPOS(cptr->x + xc + 1), RYPOS(cptr->y + yc + 1), newval);
   erasebox(cptr->x + xc, cptr->y + yc, cptr->x + xc + 2, cptr->y + yc + 2);
   if (active_rules[0] && (fp = fopen(PATCH_LOG, "a"))) {
      fprintf(fp, "%c%c%c%c%c%c%c%c%c%c", itos(oldval), itos(u), itos(ur),
                       itos(r), itos(dr), itos(d), itos(dl), itos(l), itos(ul),
                                                                 itos(newval));
      stamp("\t#", fp);
      fclose(fp);
   }
   return newval;
}

static void evolvep(pattern *context) { /* slower table-driven evolve for n-state spaces */
    register tile	*cptr, *ncptr;
    int			nzcount, live_total = 0;
    float		payoff[HY + BOXSIZE + HY][HX + BOXSIZE + HX];
    /* first, save old state and extend active region as necessary */
    for (cptr = context->tiles; cptr; cptr = cptr->next) {
	if (cptr->dead) goto cpbox; /* dead cells don't grow */
        if (H_CHNG(0))
	    maketile(context, cptr->x - BOXSIZE, cptr->y);
	if (H_CHNG(BOXSIZE - 1))
	    maketile(context, cptr->x + BOXSIZE, cptr->y);
	if (V_CHNG(0))
	    maketile(context, cptr->x, cptr->y - BOXSIZE);
	if (V_CHNG(BOXSIZE - 1))
	    maketile(context, cptr->x, cptr->y + BOXSIZE);
        if (X_CHNG(0,0)) {
            maketile(context, cptr->x - BOXSIZE, cptr->y);
            maketile(context, cptr->x, cptr->y - BOXSIZE);
            maketile(context, cptr->x - BOXSIZE, cptr->y - BOXSIZE);
        }
        if (X_CHNG(BOXSIZE - 1,0)) {
            maketile(context, cptr->x - BOXSIZE, cptr->y);
            maketile(context, cptr->x, cptr->y + BOXSIZE);
            maketile(context, cptr->x - BOXSIZE, cptr->y + BOXSIZE);
        }
        if (X_CHNG(0,BOXSIZE - 1)) {
            maketile(context, cptr->x, cptr->y - BOXSIZE);
            maketile(context, cptr->x + BOXSIZE, cptr->y);
            maketile(context, cptr->x + BOXSIZE, cptr->y - BOXSIZE);
        }
        if (X_CHNG(BOXSIZE - 1,BOXSIZE - 1)) {
            maketile(context, cptr->x + BOXSIZE, cptr->y);
            maketile(context, cptr->x, cptr->y + BOXSIZE);
            maketile(context, cptr->x + BOXSIZE, cptr->y + BOXSIZE);
        }
	/* we may need to grab the diagonally-adjacent tiles too */
cpbox:
        memcpy(&cptr->cells.pstate.ocell,
                      &cptr->cells.pstate.cell,
                      sizeof(cell_t[BOXSIZE][BOXSIZE]));
    }
    /* evolution in action */
    for (cptr = context->tiles; cptr; cptr = cptr->next) {
        register int i, j;
	memset(hold, '\0', sizeof(hold));
	GET_INTERIOR(hold, cptr->cells.pstate.ocell);
/*	    drawbox(cptr->x, cptr->y, 
		cptr->x + BOXSIZE - 1, cptr->y + BOXSIZE - 1, WHITE_EVER);
*/
	/* fill in left edge cells if needed */
	if (ncptr = fetchtile(context, cptr->x - BOXSIZE, cptr->y)) {
	    GET_COL(hold, 1, ncptr->cells.pstate.ocell, BOXSIZE - 1);
            GET_COL(hold, 0, ncptr->cells.pstate.ocell, BOXSIZE - 2);
	}
	/* fill in right edge cells if needed */
	if (ncptr = fetchtile(context, cptr->x + BOXSIZE, cptr->y)) {
	    GET_COL(hold, BOXSIZE + 2, ncptr->cells.pstate.ocell, 0);
	    GET_COL(hold, BOXSIZE + 3, ncptr->cells.pstate.ocell, 1);
	}
	/* fill in top edge cells if needed */
	if (ncptr = fetchtile(context, cptr->x, cptr->y - BOXSIZE)) {
	    GET_ROW(hold, 1, ncptr->cells.pstate.ocell, BOXSIZE - 1);
	    GET_ROW(hold, 0, ncptr->cells.pstate.ocell, BOXSIZE - 2);
	}
	/* fill in bottom edge cells if needed */
	if (ncptr = fetchtile(context, cptr->x, cptr->y + BOXSIZE)) {
	    GET_ROW(hold, BOXSIZE + 2, ncptr->cells.pstate.ocell, 0);
	    GET_ROW(hold, BOXSIZE + 3, ncptr->cells.pstate.ocell, 1);
	}
 	if (ncptr = fetchtile(context, cptr->x - BOXSIZE, cptr->y - BOXSIZE))
		get_corner(0, 0,                 ncptr, BOXSIZE - 2, BOXSIZE - 2);
 	if (ncptr = fetchtile(context, cptr->x + BOXSIZE, cptr->y - BOXSIZE))
		get_corner(0, BOXSIZE + 2,         ncptr, BOXSIZE - 2, 0);
 	if (ncptr = fetchtile(context, cptr->x + BOXSIZE, cptr->y + BOXSIZE))
		get_corner(BOXSIZE + 2, BOXSIZE + 2, ncptr, 0, 0);
 	if (ncptr = fetchtile(context, cptr->x - BOXSIZE, cptr->y + BOXSIZE))
		get_corner(BOXSIZE + 2, 0,         ncptr, 0, BOXSIZE - 2);
        if (ev_mode == TAB8_DRIVEN)
                goto NOPAYOFF;
	    /* flatten away the transition pseudo-states */
	for (i = 0; i <= BOXSIZE + 3; i++)
		for (j = 0; j <= BOXSIZE + 3; j++)
		    if (hold[i][j] > 2)
			hold[i][j] = 5 - hold[i][j];
	for (i = 1; i <= BOXSIZE + 2; i++)
		for (j = 1; j <= BOXSIZE + 2; j++) {
		    int ostate = hold[i][j];
		    payoff[i][j] = compute_payoff(ostate, hold[i - 1][j - 1]);
		    payoff[i][j] += compute_payoff(ostate, hold[i - 1][j]);
		    payoff[i][j] += compute_payoff(ostate, hold[i - 1][j + 1]);
		    payoff[i][j] += compute_payoff(ostate, hold[i][j - 1]);
		    payoff[i][j] += compute_payoff(ostate, hold[i][j + 1]);
		    payoff[i][j] += compute_payoff(ostate, hold[i + 1][j - 1]);
		    payoff[i][j] += compute_payoff(ostate, hold[i + 1][j]);
		    payoff[i][j] += compute_payoff(ostate, hold[i + 1][j + 1]);
		    payoff[i][j] += compute_payoff(ostate, ostate); /*a self-bargain?*/
		}
NOPAYOFF:
	/* apply the transition function */
	nzcount = 0;
	for (i = HY; i < BOXSIZE + HY; i++)
	    for (j = HX; j < BOXSIZE + HX; j++) {
		register int newval;
		static offsets neighbors[] =
		    {
			{-1, -1}, {-1,  0}, {-1,  1},
                        { 0, -1}, { 0,  0}, { 0,  1},
			{ 1, -1}, { 1,  0}, { 1,  1}
		    };
		float highest, maxpayoff[MAXSTATES];
		unsigned int k;
		/* compute the maximum payoff of each strategy */
                if (ev_mode == TAB8_DRIVEN) {
                    newval = btrans[hold[i][j]][hold[i - 1][j]][hold[i - 1][j + 1]]
                        [hold[i][j + 1]][hold[i + 1][j + 1]][hold[i + 1][j]]
                        [hold[i + 1][j - 1]][hold[i][j - 1]][hold[i - 1][j - 1]];
                    if (newval == BADSTATE)
                        if (passive < 0)
                            make_transition8(0, newval = hold[i][j], hold[i - 1][j],
                                hold[i - 1][j + 1], hold[i][j + 1], hold[i + 1][j + 1],
                                hold[i + 1][j], hold[i + 1][j - 1], hold[i][j - 1],
                                hold[i - 1][j - 1], newval);
                        else
                            newval = bad_state_handler8(cptr, i - HY, j - HX, hold[i - 1][j],
                                hold[i - 1][j + 1], hold[i][j + 1], hold[i + 1][j + 1],
                                hold[i + 1][j], hold[i + 1][j - 1], hold[i][j - 1],
                                hold[i - 1][j - 1], hold[i][j]);
                    goto CONT;
                }
		memset(maxpayoff, '\0', sizeof(maxpayoff));
		for (k = 0; k < sizeof(neighbors)/sizeof(offsets); k++) {
		    float np = payoff[i + neighbors[k].yi][j + neighbors[k].xi];
		    newval = hold[i + neighbors[k].yi][j + neighbors[k].xi];
		    if (np > maxpayoff[newval])
			    maxpayoff[newval] = np;
		}
		/*
		* Pick the strategy with the highest payoff.
		* Break ties towards lowest-numbered strategy.
		*/
		if (newval = hold[i][j]) {
		    highest = 0;
		    for (k = 0; k < maxstates; k++)
			if (maxpayoff[k] > highest)
		           highest = maxpayoff[newval = k];
		}
		/* this creates the transition pseudo-states */
                if (highest > payoff[i][j] && newval != hold[i][j])
		    newval = hold[i][j] + 2;
CONT:
		if (newval != cptr->cells.pstate.cell[i - HY][j - HX]) {
		    context->chgcount++;
                    /* OK, assign the new cell state */
                    cptr->cells.pstate.cell[i - HY][j - HX] = newval;
                }
		if (newval) {
		    if (dispboxes)
			context->cellcount[newval]++;
		    nzcount++;
		}
	    }
        if (nzcount == 0)
            cptr->dead++;
        else
        {
            if (truehistory && context == &active) add2history(cptr);
            cptr->dead = 0;
            if (oscillators) osc_values(context, cptr);
            live_total++;
        }
    }
    fix_plain(ncptr);
    if (context->chgcount == 0 || live_total == 0)
	state = STOP;
}

static cell_t bad_state_handler(tile *cptr, int yc, int xc, cell_t u, cell_t r, cell_t d, cell_t l, cell_t oldval) {
   cell_t newval;
   FILE *fp;
   drawbox(cptr->x + xc, cptr->y + yc, cptr->x + xc + 2, cptr->y + yc + 2,
                                                                   WHITE_EVER);
   state = STOP;
   newval = patch_transition(oldval, u, r, d, l);
   drawcell(RXPOS(cptr->x + xc + 1), RYPOS(cptr->y + yc + 1), newval);
   erasebox(cptr->x + xc, cptr->y + yc, cptr->x + xc + 2, cptr->y + yc + 2);
   if (active_rules[0] && (fp = fopen(PATCH_LOG, "a"))) {
      fprintf(fp, "%c%c%c%c%c%c", itos(oldval),
                             itos(u), itos(r), itos(d), itos(l), itos(newval));
      stamp("\t#", fp);
      fclose(fp);
   }
   return newval;
}

#define new_state(yc, xc, u, r, d, l) {\
   newval = ztrans[oldval = ocell[yc][xc]][u][r][d][l];\
   if (newval == BADSTATE)\
      if (passive < 0)\
         make_transition(0, newval = oldval, u, r, d, l, oldval);\
      else\
         newval = bad_state_handler(cptr, yc, xc, u, r, d, l, oldval);\
   if (newval != oldval) {\
       context->chgcount++;\
       chgcount = 1;\
       cptr->cells.nstate.cell[yc][xc] = newval;\
   }\
   if (newval) {\
       if (dispboxes)\
           context->cellcount[newval]++;\
       nzcount = 1;\
   }}

static void evolven(pattern *context) {
    register u32bits t1, t2;
    int live_total = 0;
    register tile *cptr;
    tile *cptrup, *cptrdn, *cptrlf, *cptrrt;
    static cell_t ocell[BOXSIZE][BOXSIZE];
    for (cptr = context->tiles; cptr; cptr = cptr->next) {
        cptr->nochgcp = cptr->nochg;
        cptr->nochg = 0;
        if (cptr->dead) continue;
        cptrup = cptr->up;
        cptrdn = cptr->dn;
        cptrlf = cptr->lf;
        cptrrt = cptr->rt;
        if ((t1 = cptr->cells.nstate.live[0][0]) || cptr->cells.nstate.live[0][1]) {
            if (cptrup == NULL) {
                cptrup = maketile(context, cptr->x, cptr->y - BOXSIZE);
                cptrup->dn = cptr;
            }
            cptrup->cells.nstate.l[2] = t1;
            cptrup->cells.nstate.l[3] = cptr->cells.nstate.live[0][1];
        }
        if ((t1 = cptr->cells.nstate.live[7][0]) || cptr->cells.nstate.live[7][1]) {
            if (cptrdn == NULL) {
                cptrdn = maketile(context, cptr->x, cptr->y + BOXSIZE);
                cptrdn->up = cptr;
            }
            cptrdn->cells.nstate.l[0] = t1;
            cptrdn->cells.nstate.l[1] = cptr->cells.nstate.live[7][1];
        }
        if ((t1 = cptr->cells.nstate.cell[0][0]) || cptr->cells.nstate.cell[1][0]
              || cptr->cells.nstate.cell[2][0] || cptr->cells.nstate.cell[3][0]
              || cptr->cells.nstate.cell[4][0] || cptr->cells.nstate.cell[5][0]
              || cptr->cells.nstate.cell[6][0] || cptr->cells.nstate.cell[7][0]) {
            if (cptrlf == NULL) {
                cptrlf = maketile(context, cptr->x - BOXSIZE, cptr->y);
                cptrlf->rt = cptr;
            }
            cptrlf->cells.nstate.c[3][0] = t1;
            cptrlf->cells.nstate.c[3][1] = cptr->cells.nstate.cell[1][0];
            cptrlf->cells.nstate.c[3][2] = cptr->cells.nstate.cell[2][0];
            cptrlf->cells.nstate.c[3][3] = cptr->cells.nstate.cell[3][0];
            cptrlf->cells.nstate.c[3][4] = cptr->cells.nstate.cell[4][0];
            cptrlf->cells.nstate.c[3][5] = cptr->cells.nstate.cell[5][0];
            cptrlf->cells.nstate.c[3][6] = cptr->cells.nstate.cell[6][0];
            cptrlf->cells.nstate.c[3][7] = cptr->cells.nstate.cell[7][0];
        }
        if ((t1 = cptr->cells.nstate.cell[0][7]) || cptr->cells.nstate.cell[1][7]
              || cptr->cells.nstate.cell[2][7] || cptr->cells.nstate.cell[3][7]
              || cptr->cells.nstate.cell[4][7] || cptr->cells.nstate.cell[5][7]
              || cptr->cells.nstate.cell[6][7] || cptr->cells.nstate.cell[7][7]) {
            if (cptrrt == NULL) {
                cptrrt = maketile(context, cptr->x + BOXSIZE, cptr->y);
                cptrrt->lf = cptr;
            }
            cptrrt->cells.nstate.c[2][0] = t1;
            cptrrt->cells.nstate.c[2][1] = cptr->cells.nstate.cell[1][7];
            cptrrt->cells.nstate.c[2][2] = cptr->cells.nstate.cell[2][7];
            cptrrt->cells.nstate.c[2][3] = cptr->cells.nstate.cell[3][7];
            cptrrt->cells.nstate.c[2][4] = cptr->cells.nstate.cell[4][7];
            cptrrt->cells.nstate.c[2][5] = cptr->cells.nstate.cell[5][7];
            cptrrt->cells.nstate.c[2][6] = cptr->cells.nstate.cell[6][7];
            cptrrt->cells.nstate.c[2][7] = cptr->cells.nstate.cell[7][7];
        }
        cptr->up = cptrup;
        cptr->dn = cptrdn;
        cptr->lf = cptrlf;
        cptr->rt = cptrrt;
    }
    /*
    * Perform actual evolution.
    */
    fix_plain(cptrup);
    for (cptr = context->tiles; cptr; cptr = cptr->next) {
        register int newval, nzcount = 0, chgcount = 0, oldval;
        if (cptr->nochgcp == 5) {
           if (dispboxes)
              for (t2 = 0; t2 < BOXSIZE; ++t2)
                 for (t1 = 0; t1 < BOXSIZE; ++t1) {
                    if (newval = cptr->cells.nstate.cell[t2][t1]) {
                       context->cellcount[newval]++;
                       nzcount = 1;
                    }
                 }
           else
               nzcount = cptr->cells.nstate.live[0][0] || cptr->cells.nstate.live[0][1]
                 || cptr->cells.nstate.live[1][0] || cptr->cells.nstate.live[1][1]
                 || cptr->cells.nstate.live[2][0] || cptr->cells.nstate.live[2][1]
                 || cptr->cells.nstate.live[3][0] || cptr->cells.nstate.live[3][1]
                 || cptr->cells.nstate.live[4][0] || cptr->cells.nstate.live[4][1]
                 || cptr->cells.nstate.live[5][0] || cptr->cells.nstate.live[5][1]
                 || cptr->cells.nstate.live[6][0] || cptr->cells.nstate.live[6][1]
                 || cptr->cells.nstate.live[7][0] || cptr->cells.nstate.live[7][1];
           goto calc_nochg;
        }
        memcpy(ocell, cptr->cells.nstate.cell, sizeof(cell_t[BOXSIZE][BOXSIZE]));
        new_state(0, 0, cptr->cells.nstate.c[0][0], ocell[0][1], ocell[1][0], cptr->cells.nstate.c[2][0]);//ul
        new_state(0, 7, cptr->cells.nstate.c[0][7], cptr->cells.nstate.c[3][0], ocell[1][7], ocell[0][6]);//ur
        new_state(7, 0, ocell[6][0], ocell[7][1], cptr->cells.nstate.c[1][0], cptr->cells.nstate.c[2][7]);//ll
        new_state(7, 7, ocell[6][7], cptr->cells.nstate.c[3][7], cptr->cells.nstate.c[1][7], ocell[7][6]);//lr
        for (t2 = 1; t2 <= 6; ++t2) {
           new_state(0, t2, cptr->cells.nstate.c[0][t2], ocell[0][t2 + 1], ocell[1][t2], ocell[0][t2 - 1]);//u
           new_state(7, t2, ocell[6][t2], ocell[7][t2 + 1], cptr->cells.nstate.c[1][t2], ocell[7][t2 - 1]);//d
           new_state(t2, 0, ocell[t2 - 1][0], ocell[t2][1], ocell[t2 + 1][0], cptr->cells.nstate.c[2][t2]);//l
           new_state(t2, 7, ocell[t2 - 1][7], cptr->cells.nstate.c[3][t2], ocell[t2 + 1][7], ocell[t2][6]);//r
        }
        for (t2 = 1; t2 <= 6; ++t2)
          for (t1 = 1; t1 <= 6; ++t1)
             new_state(t2, t1, ocell[t2 - 1][t1], ocell[t2][t1 + 1], ocell[t2 + 1][t1], ocell[t2][t1 - 1]);
        for (t2 = 0; t2 < 8; ++t2)
             cptr->cells.nstate.l[t2] = 0;
        /*
         * Box-aging and statistics gathering.
         */
        if (chgcount == 0) {
calc_nochg:
             cptr->nochg++;
             if (cptr->lf || (cptr->lf = fetchtile(context, cptr->x - BOXSIZE, cptr->y)))
               cptr->lf->nochg++;
             else
               cptr->nochg++;
             if (cptr->rt || (cptr->rt = fetchtile(context, cptr->x + BOXSIZE, cptr->y)))
               cptr->rt->nochg++;
             else
               cptr->nochg++;
             if (cptr->up || (cptr->up = fetchtile(context, cptr->x, cptr->y - BOXSIZE)))
               cptr->up->nochg++;
             else
               cptr->nochg++;
             if (cptr->dn || (cptr->dn = fetchtile(context, cptr->x, cptr->y + BOXSIZE)))
               cptr->dn->nochg++;
             else
               cptr->nochg++;
        }
        else
            if (truehistory && context == &active) add2history(cptr);
        if (nzcount == 0)
             cptr->dead++;
        else
        {
             cptr->dead = 0;
             live_total++;
             if (oscillators) osc_values(context, cptr);
        }
    }
    if (live_total == 0 || context->chgcount == 0)
        state = STOP;
}

/*
 * Parsing machinery for the transition-rule compiler begins here.
 *
 * !!!*** Nifty hack alert **** Nifty hack alert **** Nifty hack alert ****!!!
 *
 * An implementation problem with the state-quintuple format is that we need
 * to apply the `code-generator' function to all glob patterns implied by an
 * RE of the form
 *
 *  <ss><ss><ss><ss><ss>
 *
 * where <ss> can be a digit or a digit set bracketed by [, ]; but there
 * could be up to MAXSTATES to the 5th power of these, which is too many to
 * generate into a scratch array or file and then crunch through.
 *
 * Read on, and be enlightened...
 *						Eric S. Raymond
 *						esr@snark.thyrsus.com
 */
#define E_NOCLOSE	(char*) "Missing ] character"
#define E_BADCHAR	(char*) "Invalid character"
#define E_WRONGNUM	(char*) "Wrong number of states"

static char *parse_recurse(char *buf, int ecount) {
    static cell_t sofar[8 + 2];
    int nhsize = 6;
    if (ev_mode == TAB8_DRIVEN)
          nhsize = 10;
    while (buf[0] == ' ' || buf[0] == '\t')
	buf++;
    if (ecount > nhsize)
	return E_WRONGNUM;
    else if (is_state(buf[0])) {
	/* 
	 * Sequential cons of next state onto the tip of the branch
	 * passed on.
	 */
	sofar[ecount] = stoi(*buf);
	return parse_recurse(buf + 1, ecount + 1);
    }
    else if (buf[0] == '[') {
	char *ep = strchr(buf, ']');
	if (ep == 0)
	    return E_NOCLOSE;
	/*
	 * Wild-carding time. We iterate through the set; for each member,
	 * we start a recurse from just past the *end* of the set.
	 */
	while (is_state(*++buf)) {
	    char *err;
	    sofar[ecount] = stoi(*buf);
	    if ((err = parse_recurse(ep + 1, ecount + 1)) == E_NOCLOSE
			|| err == E_BADCHAR || err == E_WRONGNUM)
		return err;
	}
	return 0;
    }
    else if (buf[0] == ']')
	return 0;
    else if (buf[0] != '\n' && buf[0] != '\0' && buf[0] != '\r')
	return E_BADCHAR;
    else if (ecount < nhsize)
	return E_WRONGNUM;
    else
    {
        if (ev_mode != TAB8_DRIVEN)
	  make_transition(0, sofar[0], sofar[1], sofar[2], sofar[3], sofar[4],
                                                                     sofar[5]);
        else
          make_transition8(0, sofar[0], sofar[1], sofar[2], sofar[3], sofar[4],
                             sofar[5], sofar[6], sofar[7], sofar[8], sofar[9]);
	return 0;
    }
}

/*
 * Here is the actual transition function implementation. Note that this
 * will eat up obscene amounts of memory for large maxstates values;
 * 32K for 3, 1M for 4, 32M for 5 and so on. However, it has the merit of
 * being simple and very fast.
 */

void make_transition(int force, int s, int a, int b, int c, int d, int t) {
/* add a transition to the search tree */
   int oldval = ztrans[s][a][b][c][d];
   /* enforce rotational symmetry */
   if (oldval == BADSTATE || passive >= 0 || force) {
      ztrans[s][a][b][c][d] = t;
      if (!nosymm) {
         ztrans[s][b][c][d][a] 
            = ztrans[s][c][d][a][b] 
            = ztrans[s][d][a][b][c] = t;
         if (rotate4reflect)
           ztrans[s][d][c][b][a]
              = ztrans[s][c][b][a][d]
              = ztrans[s][b][a][d][c]
              = ztrans[s][a][d][c][b] = t;
      }
   }
}

void make_transition8(int force, int s, int a, int b, int c, int d, int e, int f, int g, int h, int t) {
   int oldval = btrans[s][a][b][c][d][e][f][g][h];
   /* enforce rotational symmetry */
   if (oldval == BADSTATE || passive >= 0 || force) {
      btrans[s][a][b][c][d][e][f][g][h] = t;
      if (!nosymm) {
         btrans[s][c][d][e][f][g][h][a][b]
            = btrans[s][e][f][g][h][a][b][c][d]
            = btrans[s][g][h][a][b][c][d][e][f] = t;
         if (rotate4reflect)
            btrans[s][a][h][g][f][e][d][c][b]
               = btrans[s][g][f][e][d][c][b][a][h]
               = btrans[s][e][d][c][b][a][h][g][f]
               = btrans[s][c][b][a][h][g][f][e][d] = t;
         else
            btrans[s][b][c][d][e][f][g][h][a]
               = btrans[s][d][e][f][g][h][a][b][c]
               = btrans[s][f][g][h][a][b][c][d][e]
               = btrans[s][h][a][b][c][d][e][f][g] = t;
      }
   }
}

char *parse_rule(char *buf) {  /* parse a transition rule */
    char *err  = parse_recurse(buf, 0);
    if (err == E_NOCLOSE
		|| err == E_WRONGNUM || err == E_BADCHAR)
	return err;
    else
	return 0;
}

char *readrules(const char *file) {
/* read a transition-rule set out of a file */
    static char		buf[1024], buf2[80];
    FILE		*fp;
    unsigned		n, s, c, r, line = 0, new_ev_mode = TABLE_DRIVEN;
    register unsigned   i, j, k, l, m;
    if ((fp = fopen(file, "r")) == 0)
	return (char*)SYSERR;
    passive = -1;
    if (palettemod) DefaultPalette();
    palettemod = historymode = nosymm = rotate4reflect = 0;
    while (fgets(buf, sizeof(buf), fp)) {
	char	*cp, *errmsg;
	line++;
	if (cp = strchr(buf, '#')) {
	    *cp = '\n';
	    *++cp = '\0';
	}
	if (*buf == '\n' || *buf == '\0')
	    continue;
	else if (!strncmp(buf, "nosymm", 6))
	    nosymm = 1;
        else if (!strncmp(buf, "rotate4reflect", 14))
            rotate4reflect = 1;
        else if (!strncasecmp(buf, "Moore", 5))
            new_ev_mode = TAB8_DRIVEN;
        else if (!strncmp(buf, "debug", 5));
	else if (!strncasecmp(buf, "states ", 7)) {
            unsigned long memreq;
	    if ((n = atoi(buf + 7)) > NMAX)
		return (char*) "Too many states";
            alloc_states(new_ev_mode, n);
            memreq = n*n*n*n*n;
            if (ev_mode == TAB8_DRIVEN) {
                memreq *= n*n*n*n;
                if (n > 12) return (char*) "Moore neiborhood supports only up to 12 states";
            }
            if (dm_given < memreq) {
               dm_given = 0;
               free(trans);
               if ((trans = (cell_t *) malloc(memreq)) == 0)
                  return (char*) "No enough free RAM for so many states";
               dm_given = memreq;
            }
            memset(trans, BADSTATE, memreq);
	}
	else if (!strncmp(buf, "passive ", 8)) {
            if ((passive = atoi(buf + 8)) >= maxstates)
               return (char*) "Too many states for passive";
	    for (i = 0; i <= passive; i++)
		for (j = 0; j <= passive; j++)
		    for (k = 0; k <= passive; k++)
			for (l = 0; l <= passive; l++)
			    for (m = 0; m <= passive; m++)
                                if (ev_mode != TAB8_DRIVEN)
				    make_transition(0, i, j, k, l, m, i);
                                else {
                                   int j1, k1, l1, m1;
                                   for (j1 = 0; j1 <= passive; j1++)
                                     for (k1 = 0; k1 <= passive; k1++)
                                       for (l1 = 0; l1 <= passive; l1++)
                                         for (m1 = 0; m1 <= passive; m1++)
                                            make_transition8(0, i, j, k, l, m, j1, k1, l1, m1, i);
                                }
	}
	else if (sscanf(buf, "%u(%u*%u)%u", &s, &n, &c, &r) == 4 && ev_mode == TABLE_DRIVEN) {
	    if (c == 0) {
		for (i = 0; i < maxstates; i++)
		    if (i != n)
			for (j = 0; j < maxstates; j++)
			    if (j != n)
				for (k = 0; k < maxstates; k++)
				    if (k != n)
					for (l = 0; l < maxstates; l++)
					    if (l != n)
						make_transition(0, s, i, j, k, l, r);
	    }
	    else if (c == 1) {
		for (i = 0; i < maxstates; i++)
		    if (i != n)
			for (j = 0; j < maxstates; j++)
			    if (j != n)
				for (k = 0; k < maxstates; k++)
				    if (k != n)
					make_transition(0, s, n, i, j, k, r);
	    }
	    else if (c == 2) {
		for (i = 0; i < maxstates; i++)
		    if (i != n)
			for (j = 0; j < maxstates; j++)
			    if (j != n) {
				make_transition(0, s, n, n, i, j, r);
				make_transition(0, s, n, i, n, j, r);
			    }
	    }
	    else /* (c == 3) */
	    {
		for (i = 0; i < maxstates; i++)
		    if (i != n)
			make_transition(0, s, n, n, n, i, r);
	    }
	}
	else if (errmsg = parse_recurse(buf, 0)) {
	    sprintf(buf2, "%s, line %d", errmsg, line);
	    fclose(fp);
	    return buf2;
	}
    }
    fclose(fp);
    return 0;
}

/* now, pull it all together */

void generate(pattern *context) {
   register tile *cptr, *nextptr;
   static unsigned t, tc1 = 3, tc2 = 4000000;
   static struct timeval prev, cur;
   memset(context->cellcount, 0, maxstates*sizeof(cellcount_t));
   context->chgcount = 0;
   if (oscillators) context->osc1 = context->osc2 = context->osc3 = context->osc4 = context->osc5 = 0;
   if (ev_mode == VALENCE_DRIVEN)
      if (born & 1)
         evolve2b0(context);
      else
         evolve2(context);
   else if (ev_mode == GEN_DRIVEN)
      if (born & 1)
         evolve3b0(context);
      else
         evolve3(context);
   else if (ev_mode == TABLE_DRIVEN)
      evolven(context);
   else
      evolvep(context);

    /* clean up tiles that have gone blank */
   for (cptr = context->tiles; cptr; cptr = nextptr) {
      nextptr = cptr->next;
      if (cptr->dead > DEADTIME)
         killtile(context, cptr);
   }
   if ((++context->generations&tc1) == 0 && context == &active) {
      gettimeofday(&cur, 0);
      t = (cur.tv_sec - prev.tv_sec)*1000000 + cur.tv_usec - prev.tv_usec;
      if (t == 0)
        tc2 += 4000000;
      else {
        if ((speed = tc2/t) < 20)
           speed += (tc2%t >= t/2);
        prev = cur;
        tc2 = 4000000;
      }
   }
   if (oscillators && context == &active) 
         if (active.chgcount == oscillator_check.chgcount && active.osc1 == oscillator_check.osc1
         && active.osc2 == oscillator_check.osc2 && active.osc3 == oscillator_check.osc3
         && active.osc4 == oscillator_check.osc4 && active.osc5 == oscillator_check.osc5)
      if ((!memcmp(oscillator_check.cellcount + 1, active.cellcount + 1, maxstates - 1))
            && comparepatterns(&active, &oscillator_check)) {
         state = STOP;
         redraw_lifew();
         sprintf(inpbuf, "Period-%d oscillator is found", active.generations - oscillator_check.generations);
         announce_and_wait(GREEN_EVER);
         oscillators = 0;
         dispboxes = save_dispboxes;
         dispchanges = save_dispchanges;
         announce_and_delay("Oscillator check mode is off");
      }
   if (runcounter && --runcounter == 0) {
      state = STOP;
      strcpy(inpbuf, "Time up.");
      announce_and_wait(GREEN_EVER);
      displaystats();
   }
}

