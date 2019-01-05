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
 * $Id: tile.h 362 2017-02-08 16:29:02Z litwr $
 */

#define XMASK 0xfffffff8
#define YMASK 0xfffffff8

/* redisplay() modes */
#define MESH 1
#define EXPOSE 2

extern pattern active, tentative, oscillator_check;       /* the two+1 pattern layers */

#ifdef HASHARRAY
void hashstat(pattern*);
#endif

void adjhash(pattern*);
pattern *analyze(pattern*);
cellcount_t bounding_box(pattern*);
cellcount_t cellcnt(pattern*);
void center(void);
void changesize(const int);
int chgcell(pattern*, coord_t, coord_t, cell_t);
void clear_all(void);
void clear_pattern(pattern*);
void current_tile(void);
void display_move(int, int, int);
void displaystats(void);
tile *fetchtile(pattern*, coord_t, coord_t);
void griddisplay(int);
void initcells(pattern*);
void killtile(pattern*, tile*);
int lookcell(pattern*, coord_t, coord_t);
tile *maketile(pattern*, coord_t, coord_t);
void median(void);
int onscreen(coord_t, coord_t);
void redisplay(int);
char* rlestr(int);
void saveall(FILE*, char);
void savesimple(pattern*, int, int, FILE*);
void setscale(const int);
void stamp(char*, FILE*);
