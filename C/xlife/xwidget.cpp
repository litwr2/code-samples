/*
 * XLife Copyright 2011-13 Vladimir Lidovski vol.litwr@gmail.com
 * $Id: xwidget.cpp 336 2014-04-09 07:04:12Z litwr $
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

#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <ctype.h>
#include "defs.h"
#include "file.h"
#include "tile.h"
#include "xwidget.h"
#include "colors.h"

#define MAXLWENTRIES 10000
#define MAXLWELEN 61

#define LWACTIVEITEMS 12
#define LW_UPDIR 0
#define LW_LIBRARY 1
#define LW_WORKDIR 2
#define LW_HOME 3
#define LW_SCROLL_DN 4
#define LW_SCROLL_UP 5
#define LW_1ST 6
#define LW_MIDDLE 7
#define LW_LAST 8
#define LW_LOADALL 9
#define LW_FN 10
#define LW_CANCEL 11
#define LW_1ST_LINE 12
#define LW_OPT_SF_A 20
#define LW_OPT_SF_D 21
#define LW_OPT_SF_I 22
#define LW_OPT_SF_M 23
#define LW_OPT_SF_P 24
#define LW_OPT_SF_R 25
#define LW_OPT_SF_S 26
#define LW_OPT_LF_D 30
#define LW_OPT_LF_T 31
#define LW_OPT_SS_C 35
#define LW_OPT_SS_I 36

static const char* const widst_msg[] = {"LOAD PATTERN", "LOAD PALETTE",
   "LOAD RULES", "SAVE FILE", "LOAD FILE PART", "SAVE SCRIPT"};
static char widget_fn[PATNAMESIZ];
static struct LWF {
   char fn[MAXLWELEN], dir;
} lwfiles[MAXLWENTRIES];
static int cp, plwfiles, tlwfiles, qlwfiles, save_plwfiles, hrs,
   flashp, widget_st, save_cp;
static char curfn[PATNAMESIZ];
static char ix[] = {'A', 'D', 'I', 'M', 'P', 'R', 'S'};

static int cmplwe(const void *e1, const void *e2) {
   if (((struct LWF*)e1)->dir != ((struct LWF*)e2)->dir)
      return ((struct LWF*)e1)->dir < ((struct LWF*)e2)->dir;
   return strcmp(((struct LWF*)e1)->fn, ((struct LWF*)e2)->fn);
}

static int loadwidgetfiles(char *dir) {
   char fullpath[PATNAMESIZ], *p;
   int  filescount = 0;
   DIR *dp;
   struct dirent *entry;
   struct stat sb;
   plwfiles = 0;
   if (dp = opendir(dir)) {
      while (entry = readdir(dp)) {
         if (*entry->d_name == '.') continue;
         makefullpath(fullpath, entry->d_name, &dir);
         if (stat(fullpath, &sb)) continue;
         p = entry->d_name + strlen(entry->d_name) - 4;
         if (sb.st_mode&S_IFREG && sb.st_mode&S_IRUSR &&
                                           ((widget_st == WIDST_LOAD_PATTERN &&
                         (!strcasecmp(p + 2, ".l") || !strcasecmp(p, ".rle") ||
                            !strcasecmp(p, ".lif") || !strcasecmp(p, ".mcl") ||
                                                 !strcasecmp(p - 1, ".life") ||
                                              !strcasecmp(p - 2, ".cells"))) ||
                                                   ((widget_st == WIDST_SAVE ||
                widget_st == WIDST_SAVE_SCRIPT) && !strcasecmp(p + 2, ".l")) ||
                  (widget_st == WIDST_LOAD_RULE && !strcasecmp(p + 2, ".r")) ||
                                            (widget_st == WIDST_LOAD_PALETTE &&
                                             !strcasecmp(p - 3, ".colors"))) ||
                                    sb.st_mode&S_IFDIR && sb.st_mode&S_IRUSR) {
            lwfiles[filescount].dir = !(sb.st_mode&S_IFREG);
            strncpy(lwfiles[filescount].fn, entry->d_name, MAXLWELEN);
            lwfiles[filescount++].fn[MAXLWELEN - 1] = 0;
         }
         if (filescount >= MAXLWENTRIES) break;
      }
      closedir(dp);
      qsort(lwfiles, filescount, sizeof(struct LWF), cmplwe);
   }
   return filescount;
}

static void drawbg(int p, GC gc) {
   DoExpose(inputw);
   if (p == LW_UPDIR) {
      XFillRectangle(disp, loadw, gc, 0, hrs*FONTHEIGHT + 3,
                                             5*LOADW_WIDTH/23 - 1, FONTHEIGHT);
      XDrawString(disp, loadw, ntextgc, 2, (hrs + 1)*FONTHEIGHT - 1,
                                                                   "UpDir", 5);
   }
   else if (p == LW_LIBRARY) {
      XFillRectangle(disp, loadw, gc, 5*LOADW_WIDTH/23 + 1, hrs*FONTHEIGHT + 3,
                                             7*LOADW_WIDTH/23 - 1, FONTHEIGHT);
      XDrawString(disp, loadw, ntextgc, 2 + 5*LOADW_WIDTH/23,
                                       (hrs + 1)*FONTHEIGHT - 1, "Library", 7);
   }
   else if (p == LW_WORKDIR) {
      XFillRectangle(disp, loadw, gc,
           12*LOADW_WIDTH/23 + 1, hrs*FONTHEIGHT + 3, 7*LOADW_WIDTH/23 - 1,
                                                                   FONTHEIGHT);
      XDrawString(disp, loadw, ntextgc, 2 + 12*LOADW_WIDTH/23,
                                       (hrs + 1)*FONTHEIGHT - 1, "WorkDir", 7);
   }
   else if (p == LW_HOME) {
      XFillRectangle(disp, loadw, gc, 19*LOADW_WIDTH/23 + 1,
                             hrs*FONTHEIGHT + 3, 4*LOADW_WIDTH/23, FONTHEIGHT);
      XDrawString(disp, loadw, ntextgc, 2 + 19*LOADW_WIDTH/23,
                                          (hrs + 1)*FONTHEIGHT - 1, "Home", 4);
   }
   else if (p == LW_SCROLL_DN) {
      XFillRectangle(disp, loadw, gc, 0, (hrs + 1)*FONTHEIGHT + 3,
                                                 4*LOADW_WIDTH/21, FONTHEIGHT);
      XDrawString(disp, loadw, ntextgc, 2, (hrs + 2)*FONTHEIGHT - 1, "PgDn",
                                                                            4);
   }
   else if (p == LW_SCROLL_UP) {
      XFillRectangle(disp, loadw, gc, 4*LOADW_WIDTH/21,
                       (hrs + 1)*FONTHEIGHT + 3, 4*LOADW_WIDTH/21, FONTHEIGHT);
      XDrawString(disp, loadw, ntextgc, 2 + 4*LOADW_WIDTH/21,
                                          (hrs + 2)*FONTHEIGHT - 1, "PgUp", 4);
   }
   else if (p == LW_1ST) {
      XFillRectangle(disp, loadw, gc, 8*LOADW_WIDTH/21 + 1,
                      (hrs + 1)*FONTHEIGHT + 3, LOADW_WIDTH/7 - 1, FONTHEIGHT);
      XDrawString(disp, loadw, ntextgc, 2 + 8*LOADW_WIDTH/21,
                                           (hrs + 2)*FONTHEIGHT - 1, "1st", 3);
   }
   else if (p == LW_MIDDLE) {
      XFillRectangle(disp, loadw, gc, 11*LOADW_WIDTH/21 + 1,
                    (hrs + 1)*FONTHEIGHT + 3, 2*LOADW_WIDTH/7 - 1, FONTHEIGHT);
      XDrawString(disp, loadw, ntextgc, 2 + 11*LOADW_WIDTH/21,
                                        (hrs + 2)*FONTHEIGHT - 1, "Middle", 6);
   }
   else if (p == LW_LAST) {
      XFillRectangle(disp, loadw, gc, 17*LOADW_WIDTH/21 + 1,
                   (hrs + 1)*FONTHEIGHT + 3, 4*LOADW_WIDTH/21 - 1, FONTHEIGHT);
      XDrawString(disp, loadw, ntextgc, 2 + 17*LOADW_WIDTH/21,
                                          (hrs + 2)*FONTHEIGHT - 1, "Last", 4);
   }
   else if (p == LW_CANCEL) {
      XFillRectangle(disp, loadw, gc, LOADW_WIDTH - FONTWIDTH + 1, 0,
                                                    FONTWIDTH, FONTHEIGHT + 1);
      XDrawLine(disp, loadw, ntextgc, LOADW_WIDTH - FONTWIDTH + 2, 2,
                                              LOADW_WIDTH - 2, FONTHEIGHT - 2);
      XDrawLine(disp, loadw, ntextgc, LOADW_WIDTH - 2, 2,
                                  LOADW_WIDTH - FONTWIDTH + 2, FONTHEIGHT - 2);
   }
   else if (p == LW_LOADALL) {
      if (widget_st == WIDST_LOAD_FILE_PART) {
         XFillRectangle(disp, loadw, gc, 0, LOADW_HEIGHT - 3*FONTHEIGHT - 2,
                                                      LOADW_WIDTH, FONTHEIGHT);
         XDrawString(disp, loadw, ntextgc, LOADW_WIDTH/2 - 4*FONTWIDTH,
                               LOADW_HEIGHT - 2*FONTHEIGHT - 5, "Load All", 8);
      }
   }
   else if (p == LW_FN) {
      char s[PATNAMESIZ], *p = s;
      XFillRectangle(disp, loadw, gc, 0, LOADW_HEIGHT - 2*FONTHEIGHT - 1,
                                                      LOADW_WIDTH, FONTHEIGHT);
      strcpy(s, widget_fn);
      if (strlen(s)*FONTWIDTH > LOADW_WIDTH - 4)
          p += strlen(s) - (LOADW_WIDTH - 4)/FONTWIDTH;
      flashp = strlen(p);
      strcat(p, "_");
      XDrawString(disp, loadw, ntextgc, 2, LOADW_HEIGHT - FONTHEIGHT - 5, p,
                                                                    strlen(p));
      if (gc == itextgc)
         if (strlen(widget_fn) >= LOADW_WIDTH/FONTWIDTH) {
            XSetWindowBackground(disp, inputw, cellcolor[GREEN_EVER].pixel);
            XClearWindow(disp, inputw);
            XSetWindowBackground(disp, inputw, fcolor);
            XDrawString(disp, inputw, ntextgc, 2, FONTHEIGHT, widget_fn,
                                                            strlen(widget_fn));
         }
   }
   else if (p >= 20 && p < 40) {
      XSetWindowBackground(disp, inputw, cellcolor[GREEN_EVER].pixel);
      XClearWindow(disp, inputw);
      XSetWindowBackground(disp, inputw, fcolor);
      switch (widget_st) {
      case WIDST_LOAD_PATTERN:
      case WIDST_LOAD_FILE_PART:
         XFillRectangle(disp, loadw, gc, p == LW_OPT_LF_D? 0: 2*LOADW_WIDTH/5,
                                                     LOADW_HEIGHT - FONTHEIGHT,
                           (p == LW_OPT_LF_D? 2: 3)*LOADW_WIDTH/5, FONTHEIGHT);
         XDrawString(disp, loadw, ntextgc, LOADW_WIDTH/5 - FONTWIDTH*3,
                                                LOADW_HEIGHT - 2, "Direct", 6);
         XDrawString(disp, loadw, ntextgc, 7*LOADW_WIDTH/10 - FONTWIDTH*9/2,
                                             LOADW_HEIGHT - 2, "Tentative", 9);
         XDrawRectangle(disp, loadw, ntextgc, LOADW_WIDTH/5 - FONTWIDTH*4 - 1,
                                        LOADW_HEIGHT - FONTHEIGHT/2 - 4, 7, 7);
         XDrawRectangle(disp, loadw, ntextgc,
                                         7*LOADW_WIDTH/10 - FONTWIDTH*11/2 - 1,
                                        LOADW_HEIGHT - FONTHEIGHT/2 - 4, 7, 7);
         if (aloadmode == (p == LW_OPT_LF_D))
            XFillRectangle(disp, loadw, ntextgc, p == LW_OPT_LF_D?
                                               LOADW_WIDTH/5 - FONTWIDTH*4 + 1:
                                         7*LOADW_WIDTH/10 - FONTWIDTH*11/2 + 1,
                                        LOADW_HEIGHT - FONTHEIGHT/2 - 2, 4, 4);
         XDrawString(disp, inputw, ntextgc, 2, FONTHEIGHT,
                                                       "Select load mode", 16);
         break;
      case WIDST_SAVE:
         XFillRectangle(disp, loadw, gc, (p - 20)*LOADW_WIDTH/7,
                         LOADW_HEIGHT - FONTHEIGHT, LOADW_WIDTH/7, FONTHEIGHT);
         XDrawString(disp, loadw, ntextgc,
                                          (p - 19)*LOADW_WIDTH/7 - 2*FONTWIDTH,
                                             LOADW_HEIGHT - 2, &ix[p - 20], 1);
         XDrawRectangle(disp, loadw, ntextgc,
                                      (p - 19)*LOADW_WIDTH/7 - 3*FONTWIDTH - 1,
                                        LOADW_HEIGHT - FONTHEIGHT/2 - 4, 7, 7);
         if (saveformat == ix[p - 20])
            XFillRectangle(disp, loadw, ntextgc,
                                      (p - 19)*LOADW_WIDTH/7 - 3*FONTWIDTH + 1,
                                        LOADW_HEIGHT - FONTHEIGHT/2 - 2, 4, 4);
         XDrawString(disp, inputw, ntextgc, 2, FONTHEIGHT,
                                                 "Select savefile format", 22);
         break;
      case WIDST_SAVE_SCRIPT:
         XFillRectangle(disp, loadw, gc, p == LW_OPT_SS_C? 0: LOADW_WIDTH/2,
                         LOADW_HEIGHT - FONTHEIGHT, LOADW_WIDTH/2, FONTHEIGHT);
         XDrawString(disp, loadw, ntextgc, (LOADW_WIDTH - 16*FONTWIDTH)/3,
                                             LOADW_HEIGHT - 2, "Collected", 9);
         XDrawString(disp, loadw, ntextgc, (2*LOADW_WIDTH - 2*FONTWIDTH)/3,
                                                 LOADW_HEIGHT - 2, "Plain", 5);
         XDrawRectangle(disp, loadw, ntextgc,
                                            (LOADW_WIDTH - 19*FONTWIDTH)/3 - 1,
                                        LOADW_HEIGHT - FONTHEIGHT/2 - 4, 7, 7);
         XDrawRectangle(disp, loadw, ntextgc,
                                           (2*LOADW_WIDTH - 5*FONTWIDTH)/3 - 1,
                                        LOADW_HEIGHT - FONTHEIGHT/2 - 4, 7, 7);
         if ((scriptformat == 'C') == (p == LW_OPT_SS_C))
            XFillRectangle(disp, loadw, ntextgc, p == LW_OPT_SS_C?
                                            (LOADW_WIDTH - 19*FONTWIDTH)/3 + 1:
                                           (2*LOADW_WIDTH - 5*FONTWIDTH)/3 + 1,
                                        LOADW_HEIGHT - FONTHEIGHT/2 - 2, 4, 4);
         XDrawString(disp, inputw, ntextgc, 2, FONTHEIGHT,
                                               "Select scriptfile format", 24);
      }
   }
   else if (p >= 100) {
      XFillRectangle(disp, loadw, gc, 0, (p + hrs - 98)*FONTHEIGHT + 4,
                                                      LOADW_WIDTH, FONTHEIGHT);
      XDrawString(disp, loadw, ntextgc, 2, (p + hrs - 97)*FONTHEIGHT,
                                                lwfiles[plwfiles + p - 100].fn,
                                       strlen(lwfiles[plwfiles + p - 100].fn));
      if (gc == itextgc)
         if (strlen(lwfiles[plwfiles + p - 100].fn) > LOADW_WIDTH/FONTWIDTH) {
            XSetWindowBackground(disp, inputw, cellcolor[GREEN_EVER].pixel);
            XClearWindow(disp, inputw);
            XSetWindowBackground(disp, inputw, fcolor);
            XDrawString(disp, inputw, ntextgc, 2, FONTHEIGHT,
                                                lwfiles[plwfiles + p - 100].fn,
                                       strlen(lwfiles[plwfiles + p - 100].fn));
         }
   }
}

int loadwidgetcoord(unsigned x, unsigned y) {
   if (y >= hrs*FONTHEIGHT + 3 && y < (hrs + 1)*FONTHEIGHT + 3)
      if (x < 5*LOADW_WIDTH/23)
         return LW_UPDIR;
      else if (x < 12*LOADW_WIDTH/23)
         return LW_LIBRARY;
      else if (x < 19*LOADW_WIDTH/23)
         return LW_WORKDIR;
      else
         return LW_HOME;
   else if (y >= (hrs + 1)*FONTHEIGHT + 3 && y < (hrs + 2)*FONTHEIGHT + 3)
      if (x < 4*LOADW_WIDTH/21)
         return LW_SCROLL_DN;
      else if (x < 8*LOADW_WIDTH/21)
         return LW_SCROLL_UP;
      else if (x < 11*LOADW_WIDTH/21)
         return LW_1ST;
      else if (x < 17*LOADW_WIDTH/21)
         return LW_MIDDLE;
      else
         return LW_LAST;
   else if (y <= FONTHEIGHT)
      if (x > LOADW_WIDTH - FONTWIDTH)
         return LW_CANCEL;
      else
         return LW_1ST_LINE;
   else if (y <= LOADW_HEIGHT - 2*FONTHEIGHT - 2 &&
                                          y >= LOADW_HEIGHT - 3*FONTHEIGHT - 2)
      return LW_LOADALL;
   else if (y <= LOADW_HEIGHT - FONTHEIGHT - 1 &&
                                          y >= LOADW_HEIGHT - 2*FONTHEIGHT - 1)
      return LW_FN;
   else if (y < LOADW_HEIGHT && y >= LOADW_HEIGHT - 2*FONTHEIGHT - 1)
      switch (widget_st) {
      int i;
      case WIDST_LOAD_PATTERN:
      case WIDST_LOAD_FILE_PART:
         if (x <= LOADW_WIDTH/3)
            return LW_OPT_LF_D;
         return LW_OPT_LF_T;
      case WIDST_SAVE:
         for (i = 0; i < 7; i++)
            if (x < (i + 1)*LOADW_WIDTH/7 && x >= i*LOADW_WIDTH/7)
               return 20 + i;
      case WIDST_SAVE_SCRIPT:
         if (x <= LOADW_WIDTH/2)
            return LW_OPT_SS_C;
         return LW_OPT_SS_I;
      }
   else if (y <= LOADW_HEIGHT - 3*FONTHEIGHT - 3 &&
                                                 y >= (hrs + 2)*FONTHEIGHT + 3)
      return (y - (hrs + 2)*FONTHEIGHT - 3)/FONTHEIGHT + 100;
   return -1;
}

void updateloadwidget(unsigned x, unsigned y) {
   int ocp = cp;
   cp = loadwidgetcoord(x, y);
   if (cp >= 100 + qlwfiles || cp >= 100 + tlwfiles) cp = ocp;
   if (ocp != cp) {
      drawbg(ocp, cellgc[LOADW_BG]);
      drawbg(cp, itextgc);
   }
}

void updateloadwidget2(int ncp) {
   int ocp = cp;
   cp = ncp;
   if (cp >= 100 + qlwfiles || cp >= 100 + tlwfiles) cp = ocp;
   if (ocp != cp) {
      drawbg(ocp, cellgc[LOADW_BG]);
      drawbg(cp, itextgc);
   }
}

static int lwcheckblock(char *fn) {
   FILE *fi;
   int n = 0;
   char s[BUFSIZ], *p;
   if (strcasecmp(fn + strlen(fn) - 2, ".l") || (fi = fopen(fn, "r")) == 0)
      return 0;
   strcpy(curfn, fn);
   while (!feof(fi)) {
      fgets(s, BUFSIZ, fi);
      if (strncmp("#B", s, 2))
         continue;
      p = s + 3;
      while (strchr(" \t:", *p)) p++;
      while (strrchr(" \t\n\r", p[strlen(p) - 1])) p[strlen(p) - 1] = 0;
      if (p) {
         strcpy(lwfiles[n].fn, p);
         lwfiles[n++].dir = 0;
      }
   }
   fclose(fi);
   if (n) {
      qlwfiles = n;
      save_plwfiles = plwfiles;
      save_cp = cp;
      cp = LW_LOADALL;
      plwfiles = 0;
      widget_st = WIDST_LOAD_FILE_PART;
      drawloadwidget();
      return 1;
   }
   return 0;
}

static void lwloadfile(char *fn) {
   stashed[0] = 0;
   strcpy(saved_rules, active_rules);
   if (!loadfile_req(fn))
      return;
   if (rules_changed())
      free_loadscript();
   sprintf(inpbuf, "%d lines of comments", numcomments);
   DoExpose(inputw);
}

static char* lwprepfn(char *fn) {
   static char fullfn[PATNAMESIZ];
   sprintf(fullfn, "%s/%s", curdir, fn);
   return fullfn;
}

static int overwrite_save(char *fn, int (*sf)(char*)) {
   strcpy(inpbuf, "This file exists. Overwrite it?");
LOOP:
   wait_activity(RED_EVER);
   if (event.type == KeyPress) {
      if (XLookupString(&event.xkey, keybuf, 16, &ks, 0) == 0 ||
                                                  strchr("YyNn", *keybuf) == 0)
         goto LOOP;
      if (strchr("Yy", *keybuf) && (*sf)(fn))
         return 1;
   }
   displaystats();
   return 0;
}

static int file_exists(char *fn) {
   FILE *fp;
   if (fp = fopen(add_opt_ext(fn, LOADEXT), "r")) {
      fclose(fp);
      return 1;
   }
   return 0;
}

void setcurdir(const char *dir) {
   char savedir[PATNAMESIZ];
   if (widget_st == WIDST_LOAD_FILE_PART)
      widget_st = WIDST_LOAD_PATTERN;
   strcpy(savedir, curdir);
   if (chdir(dir) == 0)
      if (!getcwd(curdir, PATNAMESIZ)) {
         strcpy(curdir, savedir);
         chdir(curdir);
      }
   qlwfiles = loadwidgetfiles(curdir);
}

int widget_exec(int i) {
   char *p;
   if (i == LW_CANCEL)
      return 1;
   else if (i == LW_UPDIR) {
      if (widget_st == WIDST_LOAD_FILE_PART) {
         setcurdir(".");
         plwfiles = save_plwfiles;
      }
      else
         setcurdir("..");
      drawloadwidget();
   }
   else if (i == LW_WORKDIR) {
      setcurdir(inidir);
      drawloadwidget();
   }
   else if (i == LW_HOME) {
      setcurdir(checktilda("~"));
      drawloadwidget();
   }
   else if (i == LW_LIBRARY) {
      setcurdir(LIFEDIR);
      drawloadwidget();
   }
   else if (i == LW_SCROLL_UP) {
      if (plwfiles > 0) {
         plwfiles = max(0, plwfiles - tlwfiles);
         drawloadwidget();
      }
      else if (cp > 100)
         updateloadwidget2(100);
   }
   else if (i == LW_SCROLL_DN) {
      if (plwfiles < qlwfiles - tlwfiles) {
         plwfiles = min(plwfiles + tlwfiles, qlwfiles - tlwfiles);
         drawloadwidget();
      }
      else if (cp >= 100)
         updateloadwidget2(qlwfiles + 99 - plwfiles);
   }
   else if (i == LW_LAST) {
      int t = plwfiles;
      plwfiles = max(0, qlwfiles - tlwfiles);
      if (t == plwfiles) {
         if (qlwfiles - t > 0 && cp >= 100)
            updateloadwidget2(qlwfiles + 99 - t);
      }
      else
         drawloadwidget();
   }
   else if (i == LW_MIDDLE) {
      plwfiles = max(0, (qlwfiles - tlwfiles)/2);
      drawloadwidget();
   }
   else if (i == LW_1ST) {
      if (plwfiles) {
         plwfiles = 0;
         drawloadwidget();
      }
      else if (cp >= 100)
         updateloadwidget2(100);
   }
   else if (i == LW_LOADALL && widget_st == WIDST_LOAD_FILE_PART) {
      lwloadfile(lwprepfn(curfn));
      cp = save_cp;
      return 1;
   }
   else if (i == LW_FN) {
      switch (widget_st) {
         case WIDST_LOAD_PATTERN:
            if (*widget_fn == 0)
               break;
            if (p = checktilda(widget_fn))
               lwloadfile(p);
            return 1;
         case WIDST_LOAD_RULE:
            if (p = checktilda(widget_fn)) {
               loadrules(p);
               if (rules_changed())
                  free_loadscript();
            }
            return 1;
         case WIDST_LOAD_PALETTE:
            if (p = checktilda(widget_fn))
               loadcolors(p);
            return 1;
         case WIDST_SAVE_SCRIPT:
            if (*widget_fn) {
               if ((p = checktilda(widget_fn)) == 0)
                  return 1;
               if (file_exists(p)) {
                  if (overwrite_save(p, saveloadscript))
                     return 1;
               }
               else
                  return saveloadscript(p);
            }
         case WIDST_SAVE:
            if (*widget_fn) {
               if ((p = checktilda(widget_fn)) == 0)
                  return 1;
               if (file_exists(p)) {
                  if (overwrite_save(p, savefile))
                     return 1;
               }
               else
                  return savefile(p);
            }
      }
   }
   else if (i >= 20 && i < 40) {
      if (i >= LW_OPT_SF_A && i <= LW_OPT_SF_S) {
         static char t[] = "ADIMPRS";
         int k = strchr(t, saveformat) - t;
         saveformat = ix[i - LW_OPT_SF_A];
         drawbg(LW_OPT_SF_A + k, cellgc[LOADW_BG]);
      }
      else if (i >= LW_OPT_SS_C && i <= LW_OPT_SS_I) {
         char c = scriptformat;
         scriptformat = i == LW_OPT_SS_C? 'C': 'I';
         if (c == 'C')
            drawbg(LW_OPT_SS_C, cellgc[LOADW_BG]);
         else
            drawbg(LW_OPT_SS_I, cellgc[LOADW_BG]);
      }
      else if (i >= LW_OPT_LF_D && i <= LW_OPT_LF_T) {
         int k = aloadmode;
         aloadmode = i == LW_OPT_LF_D;
         if (k != aloadmode)
            if (k)
               drawbg(LW_OPT_LF_D, cellgc[LOADW_BG]);
            else
               drawbg(LW_OPT_LF_T, cellgc[LOADW_BG]);
      }
      if (i != cp)
         drawbg(cp, cellgc[LOADW_BG]);
      drawbg(cp = i, itextgc);
   }
   else if (i >= 100 && i < qlwfiles + 100) {
      p = lwfiles[plwfiles + i - 100].fn;
      if (lwfiles[plwfiles + i - 100].dir) {
         setcurdir(p);
         drawloadwidget();
      }
      else if (widget_st == WIDST_LOAD_FILE_PART) {
         char s[PATNAMESIZ];
         strcpy(s, curfn);
         strcat(s, ":");
         strcat(s, p);
         lwloadfile(lwprepfn(s));
         cp = save_cp;
         return 1;
      }
      else if (widget_st == WIDST_LOAD_RULE) {
         loadrules(lwprepfn(p));
         if (rules_changed())
            free_loadscript();
         return 1;
      }
      else if (widget_st == WIDST_LOAD_PALETTE) {
         loadcolors(lwprepfn(p));
         return 1;
      }
      else if (widget_st == WIDST_SAVE) {
         if (!strcmp(widget_fn, p))
            if (file_exists(p)) {
               if (overwrite_save(p, savefile))
                  return 1;
            }
            else
               return savefile(p);
         strcpy(widget_fn, p);
         drawbg(LW_FN, cellgc[LOADW_BG]);
      }
      else if (widget_st == WIDST_SAVE_SCRIPT) {
         if (!strcmp(widget_fn, p))
            if (file_exists(p)) {
               if (overwrite_save(p, saveloadscript))
                  return 1;
            }
            else
               return saveloadscript(p);
         strcpy(widget_fn, p);
         drawbg(LW_FN, cellgc[LOADW_BG]);
      }
      else if (!lwcheckblock(p)) {
         lwloadfile(lwprepfn(p));
         return 1;
      }
   }
   return 0;
}

void drawloadwidget(void) {
   int i;
   char s[PATNAMESIZ], curfullpath[PATNAMESIZ], *p = curdir, *q = curdir;
   hrs = 2;
   XClearWindow(disp, loadw);
   if (widget_st == WIDST_LOAD_FILE_PART) {
      sprintf(curfullpath, "%s/%s", q, curfn);
      p = q = curfullpath;
   }
   while ((i = strlen(p)) > LOADW_WIDTH/FONTWIDTH) {
      strncpy(s, p,  LOADW_WIDTH/FONTWIDTH);
      XDrawString(disp, loadw, ntextgc, 2, hrs++*FONTHEIGHT - 1, s,
                                                        LOADW_WIDTH/FONTWIDTH);
      p += LOADW_WIDTH/FONTWIDTH;
   }
   i = (LOADW_WIDTH - i*FONTWIDTH)/2;
   XDrawString(disp, loadw, ntextgc, i, hrs*FONTHEIGHT - 1, p, strlen(p));
   tlwfiles = (LOADW_HEIGHT - (5 + hrs)*FONTHEIGHT - 6)/FONTHEIGHT;
   for (i = 0; i < tlwfiles + 100; i++)
      if (i != cp && i < 100 + qlwfiles && (i <= LWACTIVEITEMS || i >= 100))
         drawbg(i, cellgc[LOADW_BG]);
   if (cp >= 100 + qlwfiles)
      cp = 0;
   XDrawLine(disp, loadw, ntextgc, LOADW_WIDTH - FONTWIDTH, 0,
                                          LOADW_WIDTH - FONTWIDTH, FONTHEIGHT);
   i = (LOADW_WIDTH - (1 + strlen(p = 
                                   (char*) widst_msg[widget_st]))*FONTWIDTH)/2;
   XDrawString(disp, loadw, ntextgc, i, FONTHEIGHT - 3, p, strlen(p));
   XDrawLine(disp, loadw, ntextgc, 0, FONTHEIGHT + 1, LOADW_WIDTH,
                                                               FONTHEIGHT + 1);
   XDrawLine(disp, loadw, ntextgc, 0, hrs*FONTHEIGHT + 2, LOADW_WIDTH,
                                                           hrs*FONTHEIGHT + 2);
   XDrawLine(disp, loadw, ntextgc, 0, (2 + hrs)*FONTHEIGHT + 3, LOADW_WIDTH,
                                                     (2 + hrs)*FONTHEIGHT + 3);
   XDrawLine(disp, loadw, ntextgc, 0, LOADW_HEIGHT - 3*FONTHEIGHT - 3,
                                 LOADW_WIDTH, LOADW_HEIGHT - 3*FONTHEIGHT - 3);
   XDrawLine(disp, loadw, ntextgc, 0, LOADW_HEIGHT - 2*FONTHEIGHT - 2,
                                 LOADW_WIDTH, LOADW_HEIGHT - 2*FONTHEIGHT - 2);
   XDrawLine(disp, loadw, ntextgc, 0, LOADW_HEIGHT - FONTHEIGHT - 1,
                                   LOADW_WIDTH, LOADW_HEIGHT - FONTHEIGHT - 1);
   switch (widget_st) {
   case WIDST_LOAD_PATTERN:
   case WIDST_LOAD_FILE_PART:
      drawbg(LW_OPT_LF_D, cellgc[LOADW_BG]);
      drawbg(LW_OPT_LF_T, cellgc[LOADW_BG]);
      break;
   case WIDST_SAVE:
      for (i = 0; i < 7; i++)
         drawbg(LW_OPT_SF_A + i, cellgc[LOADW_BG]);
      break;
   case WIDST_SAVE_SCRIPT:
      drawbg(LW_OPT_SS_C, cellgc[LOADW_BG]);
      drawbg(LW_OPT_SS_I, cellgc[LOADW_BG]);
   }
   drawbg(cp, itextgc);
}

static int next_option() {
   if (*widget_fn && cp != LW_FN)
      return LW_FN;
   if (widget_st == WIDST_LOAD_PATTERN || widget_st == WIDST_LOAD_FILE_PART)
      return 30;
   if (widget_st == WIDST_SAVE)
      return 20;
   if (widget_st == WIDST_SAVE_SCRIPT)
      return 35;
   return LW_UPDIR;
}

static int last_option() {
   if (widget_st == WIDST_LOAD_PATTERN || widget_st == WIDST_LOAD_FILE_PART)
      return 31;
   if (widget_st == WIDST_SAVE)
      return 26;
   if (widget_st == WIDST_SAVE_SCRIPT)
      return 36;
   return LW_HOME;
}

void widget_main(int st) {
   unsigned t = st != widget_st, tcp;
   displaystats();
   *widget_fn = 0;
   widget_st = st;
   if (t)
      setcurdir(".");
   drawloadwidget();
   XRaiseWindow(disp, loadw);
   XLowerWindow(disp, rulew);
   XLowerWindow(disp, coordw);
   for (;;) {
      if (XCheckMaskEvent(disp, KeyPressMask | ButtonPressMask
                       | PointerMotionMask | ExposureMask | StructureNotifyMask
          | Button1MotionMask | Button3MotionMask | ButtonReleaseMask, &event))
/* handle other kinds of events if no key is pressed  */
         switch(event.type) {
         case MotionNotify:
            if (event.xmotion.window == loadw)
               updateloadwidget(event.xmotion.x, event.xmotion.y);
            break;
         case ButtonPress:
	    if (event.xmotion.window != loadw)
	       break;
            switch (event.xbutton.button) {
               case 4:
                  widget_exec(LW_SCROLL_UP);
                  break;
               case 5:
                  widget_exec(LW_SCROLL_DN);
                  break;
               default:
                  if (widget_exec(loadwidgetcoord(event.xbutton.x,
                                                             event.xbutton.y)))
                     goto quit;
            }
            break;
         case ConfigureNotify:
            DoResize();
            break;
         case Expose:
            DoExpose(event.xexpose.window);
            break;
         case KeyPress:
            XLookupString(&event.xkey, keybuf, 16, &ks, 0);
            switch (ks) {
               case XK_Return:
               case XK_Linefeed:
                  if (widget_exec(cp))
                     goto quit;
                  break;
               case XK_Escape:
                  goto quit;
               case XK_Page_Up:
               case XK_KP_Page_Up:
                  if (event.xkey.state & ShiftMask)
                     widget_exec(LW_UPDIR);
                  else
                     widget_exec(LW_SCROLL_UP);
                  break;
               case XK_Page_Down:
               case XK_KP_Page_Down:
                  widget_exec(LW_SCROLL_DN);
                  break;
               case XK_Home:
               case XK_KP_Home:
                  widget_exec(LW_1ST);
                  break;
               case XK_End:
               case XK_KP_End:
                  widget_exec(LW_LAST);
                  break;
               case XK_Up:
               case XK_KP_Up:
                  if (cp < 100)
                     updateloadwidget2(99 + min(tlwfiles, qlwfiles));
                  else if (cp > 100)
                     updateloadwidget2(cp - 1);
                  else if (plwfiles > 0) {
                     plwfiles--;
                     drawloadwidget();
                  }
                  break;
               case XK_Down:
               case XK_KP_Down:
                  if (cp < 100)
                     updateloadwidget2(100);
                  else if (cp + 1 < qlwfiles + 100 && cp + 1 < tlwfiles + 100)
                     updateloadwidget2(cp + 1);
                  else if (plwfiles < qlwfiles - tlwfiles) {
                     plwfiles++;
                     drawloadwidget();
                  }
                  break;
               case XK_KP_Right:
               case XK_Right:
               case XK_Tab:
                  tcp = cp;
                  if (cp < 3 || cp >= 20 && cp < 26 || cp == 30 || cp == 35)
                     tcp++;
                  else if (cp >= 3 && cp <= 8)
                     if (widget_st == WIDST_LOAD_FILE_PART)
                        tcp = LW_LOADALL;
                     else
                        tcp = next_option();
                  else if (cp >= 100)
                     if (widget_st == WIDST_LOAD_FILE_PART)
                        tcp = LW_LOADALL;
                     else
                        tcp = next_option();
                  else if (cp == LW_FN || cp == LW_LOADALL)
                     tcp = next_option();
                  else
                     tcp = 0;
                  updateloadwidget2(tcp);
                  break;
               case XK_KP_Left:
               case XK_Left:
                  tcp = cp;
                  if (cp > 0 && cp < 4 || cp > 20 && cp <= 26 || cp == 31 ||
                                                                      cp == 36)
                     tcp--;
                  else if (cp == LW_FN || cp == 20 || cp == 30 || cp == 35)
                     if (*widget_fn && cp != LW_FN)
                        tcp = LW_FN;
                     else if (widget_st == WIDST_LOAD_FILE_PART)
                        tcp = LW_LOADALL;
                     else
                        tcp = LW_HOME;
                  else if (cp == 0)
                     tcp = last_option();
                  else
                     tcp = LW_HOME;
                  updateloadwidget2(tcp);
                  break;
               case XK_BackSpace:
               case XK_Delete:
                  if (strlen(widget_fn) > 0)
                     widget_fn[strlen(widget_fn) - 1] = 0;
                  goto show_FN;
               default:
                  if (!isprint(*keybuf))
                     break;
                  if (strlen(widget_fn) < PATNAMESIZ - 10)
                     strcat(widget_fn, keybuf);
show_FN:          if (cp == LW_FN)
                     drawbg(cp, itextgc);
                  else if (strlen(keybuf) > 0)
                     updateloadwidget2(LW_FN);
            }
         }
      cursorshow(loadw, flashp*FONTWIDTH + 2, LOADW_HEIGHT - FONTHEIGHT - 5,
      cp == LW_FN ? itextgc : cellgc[LOADW_BG]);
   }
quit:
   if (cp < 100) cp = 0;
   DoExpose(inputw);
   XLowerWindow(disp, loadw);
   XRaiseWindow(disp, rulew);
   XRaiseWindow(disp, coordw);
}
