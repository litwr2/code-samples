/*
 * XLife Copyright 2013 Vladimir Lidovski vol.litwr@gmail.com
 * $Id: history.h 363 2017-11-07 19:16:47Z litwr $
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

#define HISTCOLOR1 267
#define HISTCOLOR2 266

typedef struct {
   u32bits live1, live2;
} historybox;

#ifdef HASHARRAY
typedef struct historytile_t {
    coord_t x, y;
    struct historytile_t *next, *hnext;
    historybox cells;
} historytile;

typedef struct {
    historytile         *tiles;                 /* head of the cell list */
    cellcount_t         tilecount;              /* count of boxes */
    historytile         *hashlist[HASHSIZE];    /* hash list */
    unsigned            hxm, hym;               /* hash coeff */
    int                 hxs;
} historypattern;
void fixhistoryhash(void);
void historyhashstat(int);
#elif defined(BTREE) || defined(UNORDEREDMAP)
typedef struct historytile_t {
    coord_t x, y;
    struct historytile_t *next;
    historybox cells;
} historytile;

typedef struct {
    historytile         *tiles;                 /* head of the cell list */
    cellcount_t         tilecount;              /* count of boxes */
#ifdef BTREE
    map<pair<u32bits,u32bits>, historytile*>  tilemap;
#else
    unordered_map<pair<u32bits,u32bits>, historytile*>  tilemap;
#endif
} historypattern;
#endif

extern historypattern history[];
extern int histpaint[];

void add2history(tile*);
void clearhistorytiles(void);
void displayhistorybox(u32bits, u32bits, historytile*, int);
historytile *fetchhistorytile(historypattern*, coord_t, coord_t);
historytile *makehistorytile(historypattern*, coord_t, coord_t);
void pattern2history(pattern*, int);
