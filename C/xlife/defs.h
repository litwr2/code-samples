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
A lot of modifications were added at 2001, 2011-14 by Vladimir Lidovski vol.litwr@gmail.com
(C) This version of XLife may be used under the same conditions as mentioned above
$Id: defs.h 363 2017-11-07 19:16:47Z litwr $
*/

#ifdef __GNUCC__
; /* bogus semi-colon to appease the GNU god */
#endif /* __GNUCC__ */
#include "bignumber.h"
#include <stdio.h>
#include <X11/Xos.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>
#include <X11/keysymdef.h>
#include <string.h>
#include "common.h"
#include <unistd.h>
#include <sys/time.h>
#ifdef BTREE
#include <map>
#include <utility>
using namespace std;
#endif
#ifdef UNORDEREDMAP
#include <unordered_map>
#include <utility>
using namespace std;
#endif
#ifndef MICROSOFT
#include <sys/sysinfo.h>
#endif

/*#define SMALLFONT*/
#ifndef SMALLFONT
#define NORMALFONT "9x15"
#define FONTHEIGHT 15
#define FONTWIDTH 9
#else
#define NORMALFONT "8x13"
#define FONTHEIGHT 13
#define FONTWIDTH 8
#endif

#ifndef VFREQ
#define VFREQ 0
#endif

#define MAXSCALE 6         /* 1:64 */
#define MINSCALE -22       /* 2^22:1 */
#define MINSCALEHASH -30

#define BORDERWIDTH 2

#define INPUTXOFF 2
#define INPUTH 20
#define INPUTYOFF FONTHEIGHT
#define INPUTLEN 125
#define INPUTTEXTLEN (INPUTLEN - INPUTFROMLEN)

#define COORDXOFF 2
#define COORDYOFF FONTHEIGHT
#define COORDW 200

#define STATEOFF 4

/* mouse coordinates to life-universe coordinates */
#define SCALE(x) shr((x), scale)
#define RSCALE(x) shl((int)(x), scale)
#define RSCALEBIG(x) shl((x), scale)//for big coords
#define XPOS(x) ((u32bits)(SCALE(x) + xpos))
#define YPOS(y) ((u32bits)(SCALE(y) + ypos))
#define XPOSBIG(x) ((SCALE(x) + bigXPos))
#define YPOSBIG(y) ((SCALE(y) + bigYPos))

/* life-universe coordinates to mouse coordinates */
#define RXPOS(x) RSCALE((x) - xpos)
#define RYPOS(y) RSCALE((y) - ypos)
#define RXPOSBIG(x) RSCALEBIG(abs((x) - bigXPos))//for big coords
#define RYPOSBIG(y) RSCALEBIG(abs((y) - bigYPos))

/* mouse coordinates to cell coords (relative to top left of screen) */
#define MOUSE2CELL(n)	((n) & shl(0xffffffff, scale))

/* this magic number is roughly halfway through the range of unsigned long */
/* it must be multiple of 256 for the scaling */
/* it must be multiple of 8 for the torus mode */
#define STARTX 2147483648U
#define STARTY 2147483648U

/* the program's run states */
#define STOP 	0x1	/* single-stepping */
#define HIDE	0x2	/* evolving, no per-generation display */
#define RUN	0x4	/* evolving with per-generation display */
#define HASHRUN	0x8

/* time-delays for generation evolution */
#define DELAY_FAST 0
#define DELAY_MED 250
#define DELAY_SLOW 500
#define DELAY_INCREMENT 10

#define DEADTIME	10	/* nuke a tile when it's been dead this long */

#define PATCH_LOG	"new.transitions"

#define LOADEXT		".l"

#define TRUE	1
#define FALSE	0

/*
 * If you don't want n-state capability, define this to 1 in the Makefile
 * to save some storage and reduce the program size.
 */
#define STATEBITS 8
#define MAXSTATES (1<<STATEBITS)
#define BADSTATE 0xff	/* out-of-band cell-state value */

int is_state(char);   /* is char a valid state indicator? */
int stoi(char);       /* state character to int */
char itos(int);       /* int to state character */

#if STATEBITS <= 8
typedef u8bits 	cell_t;		/* allows 256 states per cell */
#elif STATEBITS <= 16
typedef u16bits	cell_t;			/* allows 65536 states per cell */
#else
typedef u32bits	cell_t;			/* are you serious? */
#endif

#define PATNAMESIZ 256 /* It's an arbitrary limit, but probably enough */

typedef struct lq {
    unsigned long loadtime;
    char patname[PATNAMESIZ];
    int relpath;
    coord_t hotx, hoty;
    int rxx, rxy, ryx, ryy;
    struct lq  *next;
} LoadReq;

typedef struct {int xi; int yi;} offsets;

/* definitions for geometric transformations */ 

/* Get around "%" bogosity, and compute mod *correctly*. */
#define mod(m,n) (((m)>=0)?((m)%(n)):((n)-1-(-(m)+(n)-1)%(n)))

#define max(x, y)	((x) > (y) ? (x) : (y))
#define min(x, y)	((x) > (y) ? (y) : (x))
#define abs(x)	  	((x) > 0 ? (x) : (-(x)))
#define sgn(x)          (((x) > 0) ? 1 : ((x) < 0) ? -1 : 0)
#define shr(x, n)	((n) < 0 ? ((x) << -(n)) : ((x) >> (n)))
#define shl(x, n)	((n) < 0 ? ((x) >> -(n)) : ((x) << (n)))

/* compute coordinate transform for rotations */
/* yes, macros are equivalent, but this makes the meaning clearer (maybe) */
#define tx(x,y,rxx,rxy) ((rxx)*(x)+(rxy)*(y))
#define ty(x,y,ryx,ryy) ((ryx)*(x)+(ryy)*(y))
#define Tx(x, y) (tx(x,y,txx,txy)+loadx)
#define Ty(x, y) (ty(x,y,tyx,tyy)+loady)
#define TX(x, y) (Tx(x,y)-xpos)
#define TY(x, y) (Ty(x,y)-ypos)

//I really don't like these names
#define TxBIG(x, y) (tx(x - tentativeX,y - tentativeY,txx,txy) + tentativeX)
#define TyBIG(x, y) (ty(x - tentativeX,y - tentativeY,tyx,tyy) + tentativeY)
#define TXBIG(x, y) (TxBIG(x,y)-bigXPos)
#define TYBIG(x, y) (TyBIG(x,y)-bigYPos)

#ifdef SVR4
#define srandom(n)	srand48(n)
#define random()	lrand48()
#endif

/* tile.c */  /* don't change BOXSIZE! */
#define BOXSIZE         8

union cellbox {
    struct twostate_t {
        u32bits live1, live2, olive1, olive2;
        u32bits edges1, edges2; /* only 36 of 64 bits are used */
/* edges1:  1..8: ground base;
            0: ground left
            9: ground right
           10: 1st left
           11: 1st right 
           12: 4th lt
           13: 3rd lt
           14: 4th rt
           15: 2nd lt
           16: 3rd rt
           17: 2nd rt
           18: 5th rt
           21: 6th lt 
           23: 5th lt
           24: 6th rt
   edges2:  1..8: ceiling base
            0: ceiling lt
            9: ceiling rt
           12: 8th lt
           16: 7th rt
           17: 8th rt
           18: 7th lt
left: 0, 10, 15, 13, 12, 23, 21; 18, 12, 0
right: 9, 11, 17, 16, 14, 18, 24; 16, 17, 9
*/
    } twostate; /* 24 bytes */
    struct nstate_t {
       union {
           u32bits live[BOXSIZE][2];
           cell_t  cell[BOXSIZE][BOXSIZE];
       };
       union {
           u32bits l[8]; //0,1 - up; 2,3 - down
           cell_t  c[4][BOXSIZE]; //0 - up, 1 - down, 2 - left, 3 - right
       };
    } nstate;   /* 64+32=96 bytes (assuming char-sized cells) */
    struct pstate_t {
       union {
           u32bits live[BOXSIZE][2];
           cell_t  cell[BOXSIZE][BOXSIZE];
        };
        cell_t  ocell[BOXSIZE][BOXSIZE];
        union {
           u32bits l[8]; //0,1 - up; 2,3 - down
           cell_t  c[4][BOXSIZE]; //0 - up, 1 - down, 2 - left, 3 - right
        };
    } pstate;   /* 128+32=160 bytes (assuming char-sized cells) */
    struct gstate_t {
        union {
           u32bits live[BOXSIZE][2];
           u16bits slive[BOXSIZE][4];
           cell_t  cell[BOXSIZE][BOXSIZE];
        };
        u32bits on[BOXSIZE];
    } gstate;   /* 64+32=96 bytes (assuming char-sized cells) */
};

/*
 * A `pattern' is a linked list of tiles with its own context globals.
 * The universe may contain multiple patterns; they are overlayed on
 * the screen, but can be manipulated independently and evolve independently.
 */
#ifdef HASHARRAY
typedef struct tile_t {
    coord_t x, y;
    short dead, nochg, nochgcp;
    struct tile_t *up, *dn, *lf, *rt, *fore, *next, *hfore, *hnext;
/* MUST be last in the structure or our dynamic allocation will fail */
    cellbox cells;
} tile;
#ifndef HASHBITS
#define HASHBITS 20
#endif
#define HASHSIZE        (1 << HASHBITS)
//#define HASH(x,y,HXM,HXS,HYM) (shl((((x)%65521)&HXM),HXS) + ((((y)%65537) >> 3)&HYM))
#define HASH(x,y,HXM,HXS,HYM) (shl(((x)&HXM),HXS) + (((y) >> 3)&HYM))
struct pattern {
    tile                *tiles;                 /* head of the cell list */
    cellcount_t         tilecount;              /* count of boxes */
    cellcount_t         chgcount;               /* count of changes */
    unsigned long       generations;            /* context generation number */
    u32bits             osc1, osc2, osc3, osc4, osc5;       /* for oscillator check */
    //cellcount_t         population;                /* cellcount total */
    cellcount_t         cellcount[MAXSTATES];   /* count of cells */
    tile                *hashlist[HASHSIZE];    /* hash list */
    unsigned            hxm, hym;               /* hash coeff */
    int                 hxs;
    coord_t             xmax, xmin, ymax, ymin;
    unsigned            cellmass;
    char                *pattern_name;
};
#elif defined(BTREE) || defined(UNORDEREDMAP)
#ifdef UNORDEREDMAP
    namespace std {
        template<> struct hash<pair<u32bits, u32bits>> {
            size_t operator()(const pair<u32bits, u32bits>& n) const {
	        return hash<unsigned long long>()(((unsigned long long)n.first << 32) | n.second);
            }
        };
    }
#endif
typedef struct tile_t {
    coord_t x, y;
    short dead, nochg, nochgcp;
    struct tile_t *up, *dn, *lf, *rt, *fore, *next;
/* MUST be last in the structure or our dynamic allocation will fail */
    cellbox cells;
} tile;
struct pattern {
    tile                *tiles;                 /* head of the cell list */
    cellcount_t         tilecount;              /* count of boxes */
    cellcount_t         chgcount;               /* count of changes */
    unsigned long       generations;            /* context generation number */
    u32bits             osc1, osc2, osc3, osc4, osc5;       /* for oscillator check */
    //cellcount_t         population;                /* cellcount total */
    cellcount_t         cellcount[MAXSTATES];   /* count of cells */
#ifdef BTREE
    map<pair<u32bits,u32bits>, tile*>         tilemap;
#else
    unordered_map<pair<u32bits,u32bits>, tile*>         tilemap;
#endif
    coord_t             xmax, xmin, ymax, ymin;
    unsigned            cellmass;
    char                *pattern_name;
};
#endif
/* file_misc.c */
void name_file(void);
void comment(void);

/* gentab.c */
void gentab(void), gentab2(void), gentab3(void), gentab4(void), genatab4(void), genatab5(void);

/* help.c */
void help(void);
void pseudocolor_msg(void);
void redraw_help(void);
void viewvars(void);
void redraw_viewvars(void);
void view_comments(void);
int redraw_comments(void);
void view_slashinfo(void);
void redraw_slashinfo(void);

/* key.c */
void getxstring(void);
void cursorshow(Window, int, int, GC);
void set_transition(void);
void test_transition(void);
void announce_and_wait(int);
cell_t patch_transition(cell_t, cell_t, cell_t, cell_t, cell_t);
cell_t patch_transition8(cell_t, cell_t, cell_t, cell_t, cell_t, cell_t, cell_t, cell_t, cell_t);
void wait_activity(int);

/* main.c */
void DoKeyIn(char*);
int DoKeySymIn(KeySym);
void Motion(void), DoResize(void), DoExpose(Window);
void Button(void), Release(void);
void ResizeLW(int);
void alloc_states(unsigned, unsigned);
void redraw_lifew(void);
void fixpatterntopology(void);
void make_delay(long, long);
int rules_changed(void);
void set_active_rules(const char*, int);
extern coord_t savex, savey;
void eventHandler();////////////////////////
void redrawModeWindow();
void toggle_hash();
extern unsigned insideHashAlgorithm;
extern int multiColor;//for hashlife N-state colors mode
extern int currentHashMode;//1 - "classical" hash mode (fast growing step), 2 - Golly mode (less memory consumption), 3 - constant step
extern int shortText;//1 for short text mode ('gens' instead of 'generations' etc)
extern int convst;

/* tentative.c */
void boxpattern(int);
void change_tentative_color(void);
int comparepatterns(pattern*, pattern*);
void copy_tentative(void);
void copypattern(pattern*, pattern*);
void flipload(void);
void make_tentative(coord_t, coord_t, coord_t, coord_t);
void moveload(void);
void prep_tentative(void);
void turnload(void);

/* tile.c declarations are in tile.h */

/* utils.c */
void announce(const char*);
void announce_and_delay(const char*);
void benchmark(void);
void fatal(const char*);
void drawcell(coord_t, coord_t, cell_t);
void drawpivot(void);
void drawbox(coord_t, coord_t, coord_t, coord_t, int);
void erasebox(coord_t, coord_t, coord_t, coord_t);
unsigned long mstime(void);
void showcoord(const int);
void showrules(void);
void randomize(int, int, coord_t, coord_t);
void settimeout(unsigned long);
void color_button(int, int, int);
void setcolor(int, unsigned long, unsigned long);
void entercolor(unsigned long, unsigned long);
void set_paintcolor(void);
void showstates(void);
void stamp(const char*, FILE *);

/* UNIX interface */
#define SYSERR strerror(errno)

/* X I/O state information */
extern Display *disp;
extern Window rootw, mainw, lifew, helpw, inputw, coordw, rulew, statew, loadw, modew;
extern int screen;
extern unsigned long fcolor, bcolor;
extern XEvent event;
extern XFontStruct *nfont, *cfont;
extern GC ntextgc, itextgc, cellgc[], xorgc, invgc;
extern KeySym ks;

#define INPBUFLEN 255
extern char inpbuf[];
extern int minbuflen;
extern int numcomments;
#define MAXCOMMENTS 512
#define MAXCOMMENTLINELEN 192
extern char comments[][MAXCOMMENTLINELEN];
extern char keybuf[16];
extern u32bits lookup4[];
extern u16bits *lookup2, tab6[];
extern u8bits *lookup, tab4[];
extern char fname[];
extern char colorfile[];

extern unsigned maxstates, statescols;
extern coord_t xpos, ypos;       /* coords of upper left corner of viewport */
extern coord_t xorigin, yorigin; /* virtual origin */
extern coord_t lastx, lasty;     /* last coord pair displayed on status line */
extern coord_t loadx, loady;     /* location to load new points */
extern coord_t iloadx, iloady;   /* initial location of load points */
extern int txx, txy, tyx, tyy;   /* transform for tentative points */

extern int dispboxes, dispchanges, dispcoord, dispspeed;
extern int scale;
extern u32bits born;
extern u32bits live;

extern int width, maxwidth, height, inputlength, state, paintcolor;
extern struct timeval timeout;
extern char active_rules[], saved_rules[];
extern char saveformat, scriptformat;

#define MAXCHANGE       8192
#define EXTRACOLORS 14
#define MAXCOLORS (MAXSTATES + EXTRACOLORS)
extern cellcount_t chgpoints[MAXCOLORS], chgrects[MAXCOLORS];
extern XPoint points[MAXCOLORS][MAXCHANGE];
extern XRectangle rects[MAXCOLORS][MAXCHANGE];

/* cell.cpp */
int getcell(cellbox*, const int, const int);
void setcell(cellbox*, const int, const int, const int);
void displaybox(u32bits, u32bits, cellbox*);
void trdisplaybox(u32bits, u32bits, cellbox*);
void displaybox_nofb(u32bits, u32bits, cellbox*);
void trdisplaybox_nofb(u32bits, u32bits, cellbox*);
void trdrawbox(coord_t, coord_t, coord_t, coord_t, int);
extern int pseudopaint[2][2];

/* collect.cpp */
void collect(char*, int);

/* generate.cpp */
void generate(pattern*);
void newrules(char *);
char *readrules(const char*);
char *parse_rule(char*);
void make_transition(int, int, int, int, int, int, int);
void make_transition8(int, int, int, int, int, int, int, int, int, int, int);
char *parse_rule(char*);
int file_rules(const char*);
extern float payoffs[2][2];///////
float compute_payoff(int, int);

/* isave.cpp */
void savestructured(pattern*, FILE*);
extern char matchlibfile[];

#define FOR_CELLS(pp, pt, dx, dy)       for (pt=(pp)->tiles;pt;pt=pt->next) \
                                            if (!pt->dead) \
                                                for (dx=0;dx<BOXSIZE;dx++) \
                                                    for (dy=0;dy<BOXSIZE;dy++)
extern int oldp;

#define MAXDIRS         128
extern char *dirs[];

#define VALENCE_DRIVEN  0       /* based on neighborhood count (live, born), moore, rotate8 */
#define TABLE_DRIVEN    1       /* based on transition table (trans), vonNeumann, rotate4(reflect) or nosymmetries */
#define PAYOFF_DRIVEN   2       /* based on payoff matrix (payoffs) */
#define GEN_DRIVEN      4       /* based on moore, rotate8 */
#define TAB8_DRIVEN     8       /* based on transition table (trans), moore, rotate8 */

extern unsigned delay, hideperiod, randomseed, eo_comments, speed, rulew_len, runcounter;
extern char stashed[], topology, outcome[], topologyinfo[], pathtorules[];
extern int convmode, wireframe, dispmesh, oscillators, tilesize, ev_mode, pivot,
   limits, helpw_mode, pseudocolor, historymode, palettemod, nosymm, passive,
   rotate4reflect, aloadmode, truehistory, save_dispboxes, save_dispchanges,
   hashmode;
extern coord_t x_min_limit, x_max_limit, y_min_limit, y_max_limit;
extern float rnd_density;

#define chk_limits(x,y) (((limits&1) == 0\
                            || (x) >= x_min_limit && (x) < x_max_limit)\
                          && ((limits&2) == 0\
                            || (y) >= y_min_limit && (y) < y_max_limit))

#define RULESNAMELEN 80

extern char curdir[], inidir[];
extern cell_t *trans;
#define ztrans ((cell_t(*)[maxstates][maxstates][maxstates][maxstates])trans)
#define btrans ((cell_t(*)[maxstates][maxstates][maxstates][maxstates][maxstates][maxstates][maxstates][maxstates])trans)
extern XColor cellcolor[], exact;
extern XGCValues xgcv;

#if VFREQ != 0
extern struct timeval prev_vsync;
#endif

/* hashlife.cpp */
extern char strBuf[120];
void initHashlife();//init hashMap
void initRootNode(pattern*, coord_t, coord_t, coord_t, coord_t);//make root node from pattern
void hashGenerate(pattern*, char*);
void hash_fb_update(int*);
void hash2tile(int);
void clearHashLife();////
int changeCellNode(coord_t, coord_t, cell_t, int);
void redraw_hash_slashinfo();
void hashBoundingBox();
void hashMiddleScr();//move current display to center of root node
void gc(int);//garbageCollector
void doSomeHashSteps(char*, int);//command 'G' and 'B'
void setStep(char*);
void setStrBuf();
extern bigNumber currentStep;
extern unsigned long numOfNewNodes;
void saveHashFile(FILE*);
int loadHashFile(FILE*, char*);
void make_tentative_hash(const bigNumber&, const bigNumber&, const bigNumber&, const bigNumber&);
void hash_moveload(const bigNumber&, const bigNumber&);
void hash_confirmload();

//for big scales
extern bigNumber bigXPos, bigYPos, bigDX, bigDY, bigScreenWidth, bigScreenHeight;

#define SETBIGSCREENWIDTH\
	if (scale < 0)\
		bigScreenWidth = bigXPos + (width << (bigNumber)-scale);\
	else\
		bigScreenWidth = bigXPos + (width << (bigNumber)scale);
		
#define SETBIGSCREENHEIGHT\
	if (scale < 0)\
		bigScreenHeight = bigYPos + (height << (bigNumber)-scale);\
	else\
		bigScreenHeight = bigYPos + (height << (bigNumber)scale);
		
void loadrules(char*);
void fatalw(const char*);
extern bigNumber tentativeX, tentativeY, rightX, bottomY;
extern int tentativeRoot;

