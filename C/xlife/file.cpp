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
Enhancements to #I format added by Paul Callahan (callahan@cs.jhu.edu).
New format is backward compatible with old format.
*/

/*
A lot of modifications were added at 2001, 2011-13 by Vladimir Lidovski vol.litwr@gmail.com
(C) This version of XLife may be used under the same conditions as mentioned above
$Id: file.cpp 347 2014-05-05 19:18:35Z qiray $
*/

/*
void fileinit(void)
static char cuttopology(char *rulestring, int addext)
int loadscriptstat(void)  -- help.c
char* loadscriptfn(int m) -- help.c
char *checktilda(const char *stng)
char *add_opt_ext(char *name, const char *ext)
int savefile(char*)
char *seppatdir(char *filename, char *filefield)
static int breakreq()
int do_loadreq(LoadReq *loadqueue, pattern *pp)
void add_loadreq(LoadReq **ploadqueue, unsigned long loadtime, char *patname, int relpath,
                coord_t hotx, coord_t hoty, int rxx, int rxy, int ryx, int ryy)
void parse_patname(char *patname, char *patfield)
void make_comment_line(char *cp)
static int rle_decode_line (pattern *context, char *cp, coord_t hotx, coord_t hoty,
            int xoff, int yoff, int rxx, int rxy, int ryx, int ryy, int *x, int *y)
static int do_loadfile(LoadReq **loadqueue, pattern *context)
static void get_rot_flip(int rxx, int rxy, int ryx, int ryy, int *rot, int *flip)
void free_loadscript(void)
int saveloadscript(char*)
static void add_include_entry(char *patname, long loadtime, coord_t hotx, coord_t hoty,
       int xx, int xy, int yx, int yy)
void confirmload(void)
void genload()
void loadfile_req(char*)
void makefullpath(char *fullpath, char *fn, char **dirp)
char* use_rules(char *rulefile, char **dirs)
int find_file(char *fn, char **dirs)
void loadrules(char*)
char* readcolors(char*)
char* use_colors(char*, char**)
void loadcolors(char*)
*/

#include <unistd.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <dirent.h>
#include <stdlib.h>
#include <errno.h>
#include "defs.h"
#include "file.h"
#include "tile.h"
#include "colors.h"
#include "topology.h"
#include "xwidget.h"

static int do_loadfile(LoadReq**, pattern*);

/* pattern name of tentative population */
static char tentpat[PATNAMESIZ];

/* global header cell for load script */ 
LoadReq *loadscript;

/* matrix for rotations */
static int rotxx[] = {1, 0,-1, 0}; 
static int rotxy[] = {0,-1, 0, 1}; 
static int rotyx[] = {0, 1, 0,-1}; 
static int rotyy[] = {1, 0,-1, 0}; 

void fileinit(void) {
   char *cp, **dirp = dirs;
/* set up initial search path for file loads */
   *dirp++ = (char*) ""; /* reserved for current dirs for rules and colors */
   *dirp++ = (char*) ".";
   *dirp++ = inidir;
   if (cp = getenv("LIFEPATH")) {
      cp = strtok(cp, ":");
      do {
         if (dirp - dirs > MAXDIRS)
            fatal("Too many pattern libraries\n");
         *dirp++ = cp;
      } while (cp = strtok(0, ":"));
   }
   else
      *dirp++ = (char*) LIFEDIR;
/* initialize load script to empty (circular list) */
   loadscript = (LoadReq*)malloc(sizeof(LoadReq));
   loadscript->next = loadscript;
}

static char cuttopology(char *rulestring, int addext) {
   char *p = strrchr(rulestring, ':');
   *topologyinfo = 0;
   if (p
#ifdef MICROSOFT
         && p != rulestring + 1
#endif
                               ) {
      strcpy(topologyinfo, p + 1);
      *p = 0;
   }
   if (addext && !((p = strrchr(rulestring, '.')) && *(p + 1) == 'r'
                                                             && *(p + 2) == 0))
      strcat(rulestring, ".r");
   return *topologyinfo;
}

int loadscriptstat(void) {
   int i = 0;
   LoadReq *ptr = loadscript->next;
   while (ptr != loadscript) {
      ptr = ptr->next;
      i++;
   }
   return i;
}

char* loadscriptfn(int m) {
   static LoadReq *ptr;
   char *p;
   if (m) ptr = loadscript->next;
   ptr = ptr->next;
   p = strrchr(ptr->patname, '/');
   return p ? p + 1 : ptr->patname;
}

char *checktilda(const char *stng) {
   static char full[PATNAMESIZ];
   struct passwd *pw;
   int i;
   if (stng[0] == '~') {
      i = 1;
      while (stng[i] != '/' && stng[i] != '\0')
         full[i - 1] = stng[i++];
      if (i == 1)
         strcpy(full, getpwuid(getuid())->pw_dir);
      else {
         full[i - 1] = '\0';
         if (pw = getpwnam(full))
            strcpy(full, pw->pw_dir);
         else {
            sprintf(inpbuf, "Can't find information about user %s", full);
            announce_and_wait(RED_EVER);
            displaystats();
            return 0;
         }
      }
      strcat(full, stng + i);
   }
   else
      strcpy(full, stng);
   return full;
}

char *add_opt_ext(char *name, const char *ext) {
   static char buf[PATNAMESIZ];
   int le = strlen(ext), ln = strlen(name);
   strcpy(buf, name);
   if (ln <= le || strcmp(name + ln - le, ext))
      strcat(buf, ext);
   return buf;
}

int savefile(char *fn) {
   FILE *savefl;
   if (savefl = fopen(add_opt_ext(fn, LOADEXT), "w"))  {
      saveall(savefl, saveformat);
      fclose(savefl);
      setcurdir("."); /* update widget's file list */
      announce_and_delay("Saved");
      displaystats();
      return 1;
   }
   else {
      strcpy(inpbuf, SYSERR);
      announce_and_wait(RED_EVER);
   }
   return 0;
}

char *seppatdir(char *filename, char *filefield) {
/* Separates path from rest of pattern's name.  */
   char *ffptr, *colptr;
/* Make sure "/" in pattern block name isn't confusing by temporarily
*   eliminating ":" suffix
*/
   if (colptr = strchr(filename, ':')) *colptr = '\0';
   ffptr = strrchr(filename, '/');
   if (ffptr) {
      strcpy(filefield, ffptr + 1);
      *(ffptr + 1) = '\0';
   }
   else
   {
      strcpy(filefield, filename);
      *filename = '\0';
   }
/* restore ":" suffix */
   if (colptr) *colptr = ':';
   return filename;
}

static int breakreq() {
   int breakval=0;
   if (XCheckMaskEvent(disp, KeyPressMask, &event)) {
      XLookupString(&event.xkey, keybuf, 16, &ks, 0);
      breakval = keybuf[0] == 'X';
      keybuf[0] = '\0';
   }
   return breakval;
}

static char* chop_msg(char *cp, int add) {
   int t = strlen(cp) + add + 11 - (width - BORDERWIDTH*2)/FONTWIDTH;
   char *p = strrchr(cp, '/');
   if (t > 0 && p)
      if (p - cp < t)
         return p + 1;
      else if (t < strlen(cp))
         return cp + t;
   return cp;
}

#define divis(x,y) ((x)/(y)*(y)==(x))

int do_loadreq(LoadReq *loadqueue, pattern *pp) {
   int quitload = 0;
   char thispat[PATNAMESIZ], badpat[PATNAMESIZ], *cp;
/*
* This code used to try to strip LIFEDIR off the front of patname
* before copying it to tentpat.  LIFEDIR doesn't exist anymore as such,
* because we handle multiple automata types now.  Now we just copy the
* basename of patname...I hope this is equivalent!
*/
   if (cp = strrchr(loadqueue->patname, '/'))
      ++cp;
   else
      cp = loadqueue->patname;
   strcpy(tentpat, cp);
   initcells(pp);
   badpat[0] = '\0';
   while (loadqueue) {
      while (loadqueue->loadtime > pp->generations) {
         if (divis(loadqueue->loadtime - pp->generations, 100)) {
            sprintf(inpbuf, "Generating: %lu steps left (type X to halt)",
                                        loadqueue->loadtime - pp->generations);
            DoExpose(inputw);
         }
         generate(pp);
         if (quitload = breakreq())
            break;
      }
      strcpy(thispat, loadqueue->patname + loadqueue->relpath);
      sprintf(inpbuf, "Loading %s", thispat);
      DoExpose(inputw);
      if (breakreq())
         break;
      if (!do_loadfile(&loadqueue, pp)) {
         strcpy(badpat, thispat);
         free_loadscript();
         quitload = 1;
      }
      if (quitload)
         break;
   }
   displaystats();
   pp->generations = 0;
/* echo name of last pattern that couldn't be read */
   if (*badpat) {
      strcpy(inpbuf,"Can't load ");
      strcat(inpbuf, chop_msg(badpat, 0));
      announce_and_wait(RED_EVER);
      displaystats();
      return 0;
   }
   return 1;
}

void add_loadreq(LoadReq **ploadqueue, unsigned long loadtime, char *patname,
   int relpath, coord_t hotx, coord_t hoty, int rxx, int rxy, int ryx, int ryy) {
   LoadReq *newreq;

   /* Find first entry where request can go while maintaining time order.
      A heap would have better theoretical time complexity, of course, but
      in practice, the list probably won't get too long.

      After loop, we have a pointer to the pointer that must be modified
      (Could be pointer to list itself). */

   while (*ploadqueue && (*ploadqueue)->loadtime < loadtime)
      ploadqueue = &((*ploadqueue)->next);
/* Create new request block and load it up */
   newreq = (LoadReq*)malloc(sizeof(LoadReq));
   newreq->loadtime = loadtime;
/* Silently truncates long file names--should probably tell user */
   strncpy(newreq->patname, patname, PATNAMESIZ);
   newreq->relpath = relpath;
   newreq->hotx = hotx;
   newreq->hoty = hoty;
   newreq->rxx = rxx;
   newreq->rxy = rxy;
   newreq->ryx = ryx;
   newreq->ryy = ryy;
/* Insert new request in queue */
   newreq->next = *ploadqueue;
   *ploadqueue = newreq;
}

void parse_patname(char *patname, char *patfield) {
/* Breaks "<filename>:<pattern>" into two strings. */
   char *pfptr = strrchr(patname, ':');
   if (pfptr
#ifdef MICROSOFT
             && pfptr - patname != 1
#endif
                                    ) {
      *pfptr = '\0';
      strcpy(patfield, pfptr + 1);
   }
   else
      patfield[0] = '\0';
}

int detect_format(char *s) {
   char *p = strrchr(s, '.');
   if (p == 0 || p[1] == 'l' && p[2] == 0)
      return F_L;
   else if (!strcasecmp(p + 1, "lif"))
      return F_LIF;
   else if (!strcasecmp(p + 1, "mcl"))
      return F_MCL;
   else if (!strcasecmp(p + 1, "rle"))
      return F_RLE;
   else if (!strcasecmp(p + 1, "cells"))
      return F_CELLS;
   return F_L;
}

#define M_ABSOLUTE	0
#define M_RELATIVE	1
#define M_PICTURE	2
#define M_RLE		3
#define M_DELTA		4
#define M_HASH		5

void make_comment_line(char *cp) {
   if (isspace(*cp)) cp++;
   while (isspace(cp[strlen(cp) - 1]))
      cp[strlen(cp) - 1] = 0;
   if (strlen(cp) > 1) {
      strcpy(comments[numcomments], cp);
      if (numcomments < MAXCOMMENTS - 1)
         numcomments++;
   }
}

static int rle_decode_line (pattern *context, char *cp, coord_t hotx,
                            coord_t hoty, int xoff, int yoff, int rxx, int rxy,
                                            int ryx, int ryy, int *x, int *y) {
   register int sum, i, dc;
   while (*cp) {
      dc = sum = 0;
      while (*cp <= '9' && *cp >= '0')
         sum = sum*10 + *cp++ - '0';
      if (!sum) sum++;
      switch (*cp) {
      case '$':
         *y += sum;
         *x = 0;
         break;
      case '.':
      case 'b':
         *x += sum;
         break;
      case 'o':
         for (i = 0; i < sum; i++) {
            chgcell(context, hotx + tx(xoff + *x, yoff + *y, rxx, rxy),
                                 hoty + ty(xoff + *x, yoff + *y, ryx, ryy), 1);
            ++*x;
         }
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
         if (*cp > 'X' || *cp < 'A') return 1;
         for (i = 0; i < sum; i++) {
            chgcell(context, hotx + tx(xoff + *x, yoff + *y, rxx, rxy),
                    hoty + ty(xoff + *x, yoff + *y, ryx, ryy), *cp - '@' + dc);
            ++*x;
         }
      }
      cp++;
   }
   return 0;
}

static void err_rules(void) { /* uses inpbuf as input data */
   announce_and_wait(RED_EVER);
   if (*pathtorules)
      use_rules(pathtorules, 0);
   else
      file_rules(saved_rules);
}

static int do_loadfile(LoadReq **loadqueue, pattern *context) {
   char patname[PATNAMESIZ], patfield[PATNAMESIZ], tmpstring[PATNAMESIZ],
      patfile[PATNAMESIZ], fullpath[PATNAMESIZ], **dirp, *p;
   coord_t hotx, hoty;
   int relpath, rxx, rxy, ryx, ryy, state, loadmode = M_ABSOLUTE, linect = 0,
      x, y, loadformat;
   FILE *loadfl;
   LoadReq *tmpqptr;
   fname[0] = 0;
/* Get load request */
   strcpy(patname, (*loadqueue)->patname);
/*relpath = (*loadqueue)->relpath;*/
   hotx = (*loadqueue)->hotx;
   hoty = (*loadqueue)->hoty;
   rxx = (*loadqueue)->rxx;
   rxy = (*loadqueue)->rxy;
   ryx = (*loadqueue)->ryx;
   ryy = (*loadqueue)->ryy;
/* Delete request from queue */
   tmpqptr = (*loadqueue)->next;
   free(*loadqueue);
   *loadqueue = tmpqptr;
/* separate filename from pattern name */
   parse_patname(patname, patfield);
   if ((loadformat = detect_format(patname)) == F_L)
/* add load extension to filename if needed */
      if ((p = strrchr(patname, '.')) == 0
                                          || p - patname < 2 && *patname == '.'
#ifdef MICROSOFT
                    || patname[1] == ':' && p - patname < 4 && *patname == '.'
#endif
                                                                              )
         strcpy(patname, add_opt_ext(patname, LOADEXT));
/* open file */
   strcpy(patfile, patname);
   if (patfile[0] == '/'
#ifdef MICROSOFT
           || strstr(patfile, ":/") == patfile + 1 || strchr(patfile, '/') == 0
#endif
                                                                )
      strcpy(fullpath, patfile);
   else {
      for (dirp = dirs + 1; *dirp; dirp++) {
         DIR *dp;
         struct dirent *entry;
         if (dp = opendir(*dirp)) {
            while (entry = readdir(dp)) {
               sprintf(fullpath, "%s/%s/%s", *dirp, entry->d_name, patfile);
               if (access(fullpath, R_OK) == 0) {
                  closedir(dp);
                  goto foundit;
               }
            }
            closedir(dp);
         }
         else {
            sprintf(fullpath, "%s/%s", *dirp, patfile);
            if (access(fullpath, R_OK) == 0)
               goto foundit;
         }
      }
      return 0;
   }
foundit:
   if (loadfl = fopen(fullpath, "r")) {
      char buf[BUFSIZ], patid[PATNAMESIZ], *cp;
      int xoff = 0, yoff = 0, lx, ly;
      int endpattern = 0, found = 0;
      strcpy(saved_rules, active_rules);
      if (patfield[0] == '\0')
         outcome[0] = numcomments = 0;
      fseek(loadfl, 0, SEEK_SET);
      if (loadformat == F_L) {
         /* If we are searching for a specific pattern in the file,
           then we skip lines till we find it.  This is a pretty tacky way
           to handle multiple pattern definitions in the same file,
           but files with #I format probably won't get big enough for
           anyone to notice.  Really, we should implement a dictionary to
           save the location of a pattern definition in a file the first time
           we see it.
         */
         if (patfield[0]) {
            while (!found && fgets(buf, BUFSIZ, loadfl)) {
               if (buf[0] == '#' && buf[1] == 'B') {
                  for (cp = buf + 2; isspace(*cp) ||  *cp == ':'; cp++)
                     continue;
                  sscanf(cp, "%s", patid);
                  found = !strcmp(patfield, patid);
               }
            }
            if (!found) {
               fclose(loadfl);
               return 0;
            }
         }
         while (fgets(buf, BUFSIZ, loadfl) && !endpattern) {
            if (buf[0] == '#') {
               char incl[BUFSIZ];
               char pardir[PATNAMESIZ], *errmsg;
               int rotate, flip, relpathoffset;
               unsigned long loadtime;
               relpathoffset = 0;
               incl[0] = '\0';
               switch(buf[1]) {
               case 'A':
                  loadmode = M_ABSOLUTE;
                  break;

               case 'B':
/* Anything between #B and #E is ignored, since it is a pattern definition to
be included later. #B and #E cannot be nested, so we just skip till we pass #E */
                  while (fgets(buf, BUFSIZ, loadfl)
                                          && (buf[0] != '#' || buf[1] != 'E'));
                     break;

               case 'C':
                  make_comment_line(buf + 2);
                  break;

               case 'E':
/* if we hit a "#E" at this point, then it must be the one that closes the
"#B <patfield>" that we are reading (assuming a syntactically correct file) */
                  endpattern = 1;
                  break;

               case 'I':
                  xoff = yoff = rotate = loadtime = 0;
                  flip = 1;
                  sscanf(buf + 2, " %s %d %d %d %d %lu", incl, &xoff, &yoff,
                                                    &rotate, &flip, &loadtime);
/* Silently ignore invalid flips */
                  if (flip != 1 && flip != -1) flip = 1;
/* if included pattern begins with ':' then assume it's in active file */
                  if (incl[0] == ':') {
                     strcpy(tmpstring, patfile);
                     strcat(tmpstring, incl);
                     strcpy(incl, tmpstring);
                  }
/* if relative path given, assume directory of parent */
/* it is ignored because the current directory will be added to pathes list later */
                  /*else
                        if (!strchr("/~", incl[0])) {
                            strcpy(pardir, patfile);
                            seppatdir(pardir, tmpstring);
                            relpathoffset = strlen(pardir);
                            strcat(pardir, incl);
                            strcpy(incl, pardir);
                      }*/
                  loadtime += context->generations;
                  if ((long)loadtime < 0) loadtime = 0;
                  add_loadreq(loadqueue, loadtime, incl, relpathoffset,
                                                  hotx + tx(xoff,yoff,rxx,rxy),
                                                  hoty + ty(xoff,yoff,ryx,ryy),
                           rxx*rotxx[mod(rotate,4)] + rxy*rotyx[mod(rotate,4)],
                    flip*(rxx*rotxy[mod(rotate,4)] + rxy*rotyy[mod(rotate,4)]),
                           ryx*rotxx[mod(rotate,4)] + ryy*rotyx[mod(rotate,4)],
                   flip*(ryx*rotxy[mod(rotate,4)] + ryy*rotyy[mod(rotate,4)]));
                  break;

               case 'K':
                  buf[strlen(buf) - 1] = '\0';
                  if (strlen(buf) > 3)
                     strcpy(outcome, buf + 3);
                  break;

               case 'N':
                  sscanf(buf + 2, " %s", fname);
                  break;

               case 'D':
                  loadmode = M_DELTA;
                  x = y = xoff = yoff = 0;
                  sscanf(buf + 2, " %d %d", &xoff, &yoff);
                  break;

               case 'H':
                  if (hashmode)
                     linect = loadHashFile(loadfl, buf);
                  else {
                     linect = 0;
                     strcpy(inpbuf, "Not in hash mode");
                     announce_and_wait(RED_EVER);
                     displaystats();
                  }
                  fclose(loadfl);
                  //loadmode = M_HASH;
                  return linect; 

               case 'P':
                  loadmode = M_PICTURE;
                  linect = -1;
                  if (oldp) linect++;
                  xoff = yoff = 0;
                  sscanf(buf + 2, " %d %d", &xoff, &yoff);
                  break;

               case 'R':
                  loadmode = M_RELATIVE;
                  xoff = yoff = 0;
                  sscanf(buf + 2, " %d %d", &xoff, &yoff);
                  break;

               case 'M':
                  x = y = xoff = yoff = 0;
                  if (strchr(buf + 2, ','))
                     sscanf(buf + 2, " %*d , %*d %d %d", &xoff, &yoff);
                  else
                     sscanf(buf + 2, " %d %d", &xoff, &yoff);
                  loadmode = M_RLE;
                  break;

               case 'T':
                  for (cp = buf + 2; isspace(*cp); cp++);
                  while (isspace(cp[strlen(cp) - 1]))
                     cp[strlen(cp) - 1] = '\0';
                  x = cuttopology(cp, 0);
                  if (file_rules(cp)) {
                     sprintf(inpbuf, "Can't set '%s' rules", cp);
                     goto badrules;
                  }
                  if (hashmode && rules_changed()) {
                     hash2tile(ev_mode);
                     clearHashLife();
                     bounding_box(&active);
                     initRootNode(&active, active.xmax, active.xmin, active.ymax, active.ymin);
                  }
                  if (x)
                     if (set_topology(topologyinfo) > 0) {
                        sprintf(inpbuf, "Can't set topology %s", topologyinfo);
                        goto badrules;
                     }
                     else
                        fixpatterntopology();
                  break;

               case 'X':
                  for (cp = buf + 2; isspace(*cp); cp++);
                  while (isspace(cp[strlen(cp) - 1]))
                     cp[strlen(cp) - 1] = '\0';
                  if (errmsg = use_colors(cp, dirs)) {
                     sprintf(inpbuf, "Can't load %s: %s",
                                         chop_msg(cp, strlen(errmsg)), errmsg);
                     announce_and_wait(RED_EVER);
                  }
                  else
                     strcpy(colorfile, rfullpath);
                  break;

               case 'U':
                  for (cp = buf + 2; isspace(*cp); cp++);
                  while (isspace(cp[strlen(cp) - 1]))
                     cp[strlen(cp) - 1] = '\0';
                  x = cuttopology(cp, 1);
                  if (strcmp(active_rules, cp)
                                           && (errmsg = use_rules(cp, dirs))) {
                     sprintf(inpbuf, "Can't load %s: %s", cp, errmsg);
badrules:            fclose(loadfl);
                     err_rules();
                     return 0;
                  }
                  if (x)
                     if (set_topology(topologyinfo) > 0) {
                        sprintf(inpbuf, "Can't set topology %s", topologyinfo);
                        goto badrules;
                     }
                     else
                        fixpatterntopology();
               }
            }
            else if (loadmode == M_ABSOLUTE && maxstates > 2
                              && sscanf(buf, "%d %d %d\n", &x, &y, &state) == 3
                                                                      && state)
               chgcell(context, xorigin + tx(x, y, rxx, rxy),
                                          yorigin + ty(x, y, ryx, ryy), state);
            else if (loadmode == M_ABSOLUTE
                                         && sscanf(buf,"%d %d\n", &x, &y) == 2)
               chgcell(context, xorigin + tx(x, y, rxx, rxy),
                                              yorigin + ty(x, y, ryx, ryy), 1);
            else if (loadmode == M_RELATIVE && maxstates > 2
                              && sscanf(buf, "%d %d %d\n", &x, &y, &state) == 3
                                                                      && state)
               chgcell(context, hotx + tx(xoff + x, yoff + y, rxx, rxy),
                                hoty + ty(xoff + x, yoff + y, ryx, ryy), state);
            else if (loadmode == M_RELATIVE
                                         && sscanf(buf,"%d %d\n", &x, &y) == 2)
               chgcell(context, hotx + tx(xoff + x, yoff + y, rxx, rxy),
                                   hoty + ty(xoff + x, yoff + y, ryx, ryy), 1);
            else if (loadmode == M_DELTA && maxstates > 2
                            && sscanf(buf, "%d %d %d\n", &lx, &ly, &state) == 3
                                                                    && state) {
               x += lx;
               y += ly;
               chgcell(context, hotx + tx(xoff + x, yoff + y, rxx, rxy),
                               hoty + ty(xoff + x, yoff + y, ryx, ryy), state);
            }
            else if (loadmode == M_DELTA
                                    && sscanf(buf, "%d %d\n", &lx, &ly) == 2) {
               x += lx;
               y += ly;
               chgcell(context, hotx + tx(xoff + x, yoff + y, rxx, rxy),
                                   hoty + ty(xoff + x, yoff + y, ryx, ryy), 1);
            }
            else if (loadmode == M_RLE)
               rle_decode_line(context, buf, hotx, hoty, xoff, yoff, rxx, rxy,
                                                             ryx, ryy, &x, &y);
            else                /* loadmode == M_PICTURE */
            {
               unsigned int bufoff = 0;
               for(;;) {
                  for (cp = buf; *cp; cp++)
                     if (*cp == '*')
                        chgcell(context, hotx + tx(xoff + bufoff + cp - buf,
                                                      yoff + linect, rxx, rxy),
                                            hoty + ty(xoff + bufoff + cp - buf,
                                                  yoff + linect, ryx, ryy), 1);
                     else if (stoi(*cp) >= 0)
                        chgcell(context, hotx + tx(xoff + bufoff + cp - buf,
                                                      yoff + linect, rxx, rxy),
                                            hoty + ty(xoff + bufoff + cp - buf,
                                          yoff + linect, ryx, ryy), stoi(*cp));
                  if (*(cp - 1) == '\n')
                     break;
                  bufoff += BUFSIZ - 1;
                  if (!fgets(buf, BUFSIZ, loadfl))
                     break;
               }
            }
            linect++;
         }
      }
      else if (loadformat == F_LIF) {
         loadmode = M_ABSOLUTE;
         while (fgets(buf, BUFSIZ, loadfl)) {
            if (buf[0] == '#') {
               switch(buf[1]) {
               case 'D':
                  make_comment_line(buf + 2);
                  break;
               case 'N':
                  file_rules("life");
                  break;
               case 'P':
                  loadmode = M_PICTURE;
                  xoff = yoff = 0;
                  sscanf(buf + 2, " %d %d", &xoff, &yoff);
                  linect = -1;
                  break;
               case 'R':
                  for (cp = buf + 2; isspace(*cp); cp++)
                     continue;
                  cp[strlen(cp) - 1] = '\0';
                  if (file_rules(cp)) {
                     sprintf(inpbuf, "Can't set rules %s", cp);
                     goto badrules;
                  }
               }
            }
            else if (loadmode == M_ABSOLUTE
                                          && sscanf(buf, "%d %d\n",&x,&y) == 2)
               chgcell(context, xorigin + tx(x, y, rxx, rxy),
                                              yorigin + ty(x, y, ryx, ryy), 1);
            else                /* loadmode == M_PICTURE */
            {
               unsigned int bufoff = 0;
               while(1) {
                  for (cp = buf; *cp; cp++)
                     if (*cp == '*')
                        chgcell(context, hotx + tx(xoff + bufoff + cp - buf,
                                                      yoff + linect, rxx, rxy),
                                            hoty + ty(xoff + bufoff + cp - buf,
                                                  yoff + linect, ryx, ryy), 1);
                  if (*(cp - 1) == '\n')
                     break;
                  bufoff += BUFSIZ - 1;
                  if (!fgets(buf, BUFSIZ, loadfl))
                     break;
               }
            }
            linect++;
         }
      }
      else if (loadformat == F_RLE) {
         x = y = xoff = yoff = 0;
         while (fgets(buf, BUFSIZ, loadfl)) {
            if (buf[0] == '#' || endpattern) {
               if (p = strstr(buf, "CXRLE")) {
                  if (cp = strstr(p + 6, "Pos"))
                     sscanf(cp + 3, " = %d , %d", &xoff, &yoff);
                  if (cp = strstr(p + 6, "Gen"))
                     sscanf(cp + 3, " = %ul", &context->generations);
               }
               else
                  make_comment_line(buf + 2 - endpattern);
            }
            else if (sscanf(buf, " x = %d, y = %d, rule = %s ", &lx, &ly, patid) == 3) {
               if (p = strrchr(patid, ':')) {
                  *p = 0;
                  if (set_topology(p + 1) > 0) {
                     sprintf(inpbuf, "Can't set topology %s", p + 1);
                     goto badrules;
                  }
                  else { /* a bit crude but works */
                     if (limits&1)
                        if ((x_max_limit - x_min_limit)/8%2) xoff += 4;
                     if (limits&2)
                        if ((y_max_limit - y_min_limit)/8%2) yoff += 4;
                  }
               }
               strcpy(tmpstring, patid);
               if (cp = strchr(patid, '/'))
                  if (toupper(patid[0]) == 'S') {
                     strcpy(tmpstring, patid + 1);
                     tmpstring[cp - patid - 1] = '/';
                     strcpy(tmpstring + (cp - patid), cp + 2);
                  }
                  else if (toupper(patid[0]) == 'B') {
                     strcpy(tmpstring, cp + 2);
                     strcat(tmpstring, "/");
                     strncat(tmpstring, patid + 1, cp - patid - 1);
                  }
               cp = add_opt_ext(tmpstring, ".r");
               if ((relpath = strcmp(tmpstring, active_rules))
                           && strcmp(active_rules, cp) && file_rules(tmpstring)
                                                      && use_rules(cp, dirs)) {
                  sprintf(inpbuf, "Can't set rules %s", tmpstring);
                  goto badrules;
               }
               else if (relpath && !strchr(tmpstring, '/')) {
                  cp = add_opt_ext(tmpstring, ".colors");
                  use_colors(cp, dirs);
               }
            }
            else if (rle_decode_line(context, buf, hotx, hoty, xoff, yoff, rxx,
                                                        rxy, ryx, ryy, &x, &y))
               endpattern = 2;
         }
      }
      else if (loadformat == F_CELLS) {
         file_rules("life");
         linect = xoff = yoff = 0;
         while (fgets(buf, BUFSIZ, loadfl)) {
            if (buf[0] == '!')
               make_comment_line(buf);
            else
            {
               unsigned int bufoff = 0;
               while(1) {
                  for (cp = buf; *cp; cp++)
                     if (*cp == 'O')
                        chgcell(context, hotx + tx(xoff + bufoff + cp - buf,
                                                      yoff + linect, rxx, rxy),
                                            hoty + ty(xoff + bufoff + cp - buf,
                                                  yoff + linect, ryx, ryy), 1);
                  if (*(cp - 1) == '\n')
                     break;
                  bufoff += BUFSIZ - 1;
                  if (!fgets(buf, BUFSIZ, loadfl))
                     break;
               }
            }
            linect++;
         }
      }
      else if (loadformat == F_MCL) {
         int norules = 1, time;
         char mcl_topology[16] = "";
         x = y = xoff = yoff = 0;
         while (fgets(buf, BUFSIZ, loadfl)) {
            if (!strncmp(buf, "#D", 2))
               make_comment_line(buf + 2);
            else if (sscanf(buf, "#BOARD %s ", tmpstring + 1) == 1) {
               if (*mcl_topology == 0) *tmpstring = 'P';
               if (p = strchr(tmpstring, 'x')) {
                  *p = ',';
                  strcpy(mcl_topology, tmpstring);
               }
            }
            else if (sscanf(buf, "#WRAP %s ", tmpstring) == 1) {
               if (tmpstring[0] == '1') mcl_topology[0] = 'T';
            }
            else if (sscanf(buf, "#SPEED %d ", &time) == 1)
               settimeout(delay = time);
            else if (norules && sscanf(buf, "#GOLLY %s ", tmpstring) == 1) {
               if (!strcasecmp(tmpstring, "wireworld"))
                  norules = file_rules(tmpstring);
            }
            else if (norules && sscanf(buf, "#RULE %s ", patid) == 1) {
               strcpy(tmpstring, patid);
               if (file_rules(tmpstring)
                            && use_rules(add_opt_ext(tmpstring, ".r"), dirs)) {
                  sprintf(inpbuf, "Can't set rules %s", tmpstring);
                  goto badrules;
               }
               else
                  norules = 0;
            }
            else if (!strncmp(buf, "#L ", 3))
               rle_decode_line(context, buf + 3, hotx, hoty, xoff, yoff, rxx,
                                                        rxy, ryx, ryy, &x, &y);
            else
               make_comment_line(buf);
        }
        if (*mcl_topology)
           if (set_topology(mcl_topology) > 0) {
                  sprintf(inpbuf, "Can't set topology %s", tmpstring);
                  goto badrules;
           }
        if (norules) file_rules("life");
      }
      fclose(loadfl);
      return 1;
   }
   else
      return 0;
}

static void get_rot_flip(int rxx, int rxy, int ryx, int ryy, int *rot, int *flip) {
/* compute rotation and flip from given transformation */
/* No fancy mathematics; just search table of rotations */
   for (*rot = 0; *rot < 4; (*rot)++) {
      if (rxx == rotxx[*rot] && ryx == rotyx[*rot]) {
         if (rxy == -rotxy[*rot] && ryy == -rotyy[*rot]) *flip = -1;
         else *flip = 1;
         break;
      }
   }
}

void free_loadscript(void) {
/* reinitialize load script to one self-connected cell */
   LoadReq *nptr,*ptr;
   for (ptr = loadscript->next; ptr != loadscript;) {
      nptr = ptr->next;
      free(ptr);
      ptr = nptr;
   }
   loadscript->next = loadscript;
}

int saveloadscript(char *fn) {
   FILE *savefl;
   char outbuf[PATNAMESIZ];
   int rot = 0, flip = 0;
   unsigned long mingener;
   LoadReq *ptr;
   strcpy(outbuf, add_opt_ext(fn, LOADEXT));
   if (savefl = fopen(outbuf, "w")) {
      if (scriptformat != 'C')
         fprintf(savefl, "##XLife\n");
/* write load script */
/* first, find minimum generation at which a pattern is
to be included so include times can be normalized */
      mingener = active.generations;
      for (ptr = loadscript->next; ptr != loadscript;) {
         ptr = ptr->next;
         if (mingener > ptr->loadtime) mingener = ptr->loadtime;
      }
/* now output file */
      for (ptr = loadscript->next; ptr != loadscript;) {
         ptr = ptr->next;
         get_rot_flip(ptr->rxx, ptr->rxy, ptr->ryx, ptr->ryy, &rot, &flip);
         fprintf(savefl, "#I %s %d %d %d %d %lu\n", ptr->patname,
                                          ptr->hotx - savex, ptr->hoty - savey,
                                          rot, flip, ptr->loadtime - mingener);
      }
      fclose(savefl);
      free_loadscript();
      if (scriptformat == 'C')
         collect(outbuf, 1);
      announce_and_delay("Script is saved");
      displaystats();
      return 1;
   }
   else {
      strcpy(inpbuf, SYSERR);
      announce_and_wait(RED_EVER);
      displaystats();
   }
   return 0;
}

static void add_include_entry(char *patname, long loadtime, coord_t hotx,
                                coord_t hoty, int xx, int xy, int yx, int yy) {
   LoadReq *newreq;
/* Create new include block and load it up */
   newreq = (LoadReq *)malloc(sizeof(LoadReq));
   strncpy(newreq->patname, patname, PATNAMESIZ);
   newreq->loadtime = loadtime;
   newreq->hotx = hotx;
   newreq->hoty = hoty;
   newreq->rxx = xx;
   newreq->rxy = xy;
   newreq->ryx = yx;
   newreq->ryy = yy;
/* Insert new request at end of list */
   newreq->next = loadscript->next;
   loadscript->next = newreq;
   loadscript = newreq;
}

void confirmload(void) {
   if (hashmode && tentativeRoot) {
      hash_confirmload();
      displaystats();
      return;
   }
   if (tentative.tiles) {
      prep_tentative();/* undo boxing */     
      if (tentative.cellmass != active.cellmass) {
         generate(&tentative);
         strcpy(inpbuf, "The phases are not match! One step for tentative pattern is made");
         announce_and_wait(RED_EVER);
      }    
/* add completed load to load script */
      add_include_entry(tentpat, active.generations - tentative.generations,
                                             loadx, loady, txx, txy, tyx, tyy);                                            
      copy_tentative();      
/* clear the tentative pattern */
      clear_pattern(&tentative);     
   }
   adjhash(&active);
   displaystats();
}

void genload() {
/* perform some generations on loaded cells */
   static char genmsg[] = "Number of generations to perform (default=1): ";
   int gens = 1;
   minbuflen = strlen(genmsg);
   announce(genmsg);
   getxstring();
   if (ks == XK_Escape) goto EXIT;
   sscanf(inpbuf + strlen(genmsg), "%d", &gens);
   while (gens-- > 0) {
      if (divis(gens,100)) {
         sprintf(inpbuf, "Generating: %d steps left (type X to halt)", gens);
         DoExpose(inputw);
      }
      if (tentative.tiles)
         generate(&tentative);
      else
         generate(&active);
      if (breakreq())
         break;
   }
EXIT:
   displaystats();
}

int loadfile_req(char *fn) {
   char tmp[PATNAMESIZ], loaddir[PATNAMESIZ];
   LoadReq *loadqueue = 0;
   if (!stashed[0])
      strcpy(stashed, fn);
/* redefine user load directory (but not pattern library) */
   strcpy(loaddir, fn);
   seppatdir(loaddir, tmp);
/* don't need to add life extension now, since it will be added in do_loadfile */
   txx = tyy = 1;
   txy = tyx = 0;
   add_loadreq(&loadqueue, 0, fn, 0, aloadmode? STARTX: loadx,
                                         aloadmode? STARTY: loadx, 1, 0, 0, 1);
   dirs[0] = loaddir; /* support for rules & palette file */
   if (!do_loadreq(loadqueue, aloadmode? &active: &tentative))
      return 0;
   dirs[0] = (char*) "";
   if (active.tilecount == 0) {
      unsigned cellwidth, cellheight;
      if ((tentative.xmax|tentative.xmin|tentative.ymax|tentative.ymin) == 0)
         bounding_box(&tentative);
      while (1) {
         cellwidth = SCALE(width - BORDERWIDTH*2);
         cellheight = SCALE(height - INPUTH - BORDERWIDTH*3);
         if (cellwidth >= tentative.xmax - tentative.xmin
                               && cellheight >= tentative.ymax - tentative.ymin
                                                          || scale <= MINSCALE)
            break;
         --scale;
         loadx = cellwidth/2 + xpos;
         loady = cellheight/2 + ypos;
      }
      setscale(scale);
   }
   adjhash(&tentative);
   redraw_lifew();
   showstates();
   showrules();
   return 1;
}

void makefullpath(char *fullpath, char *fn, char **dirp) {
   if ((*dirp)[strlen(*dirp) - 1] == '/')
      sprintf(fullpath, "%s%s", *dirp, fn);
   else
      sprintf(fullpath, "%s/%s", *dirp, fn);
}

int find_file(char *file, char **dirs) {
   char **dirp, dirstore[MAXDIRS][PATNAMESIZ], *subdirs[MAXDIRS];
   int subdirscount;
   if (file[0] == '/')
      strcpy(rfullpath, file);
   else if (strchr(file, '/'))
   { /* relative to absolute path conversion. is it needed? */
      strcpy(dirstore[0], file);
      *strrchr(dirstore[0], '/') = 0;
      chdir(dirstore[0]);
      getcwd(rfullpath, PATNAMESIZ);
      sprintf(rfullpath + strlen(rfullpath), "/%s",
                                        dirstore[0] + strlen(dirstore[0]) + 1);
      chdir(curdir);
   }
   else
   {
      for (dirp = dirs; *dirp; dirp++) {
         DIR *dp;
         struct dirent *entry;
         struct stat sb;
         if (**dirp == 0) continue;
         if (dp = opendir(*dirp)) {
            subdirscount = 0;
            while (entry = readdir(dp)) {
               if (*entry->d_name == '.') continue;
               makefullpath(rfullpath, entry->d_name, dirp);
               if (stat(rfullpath, &sb)) continue;
               if (sb.st_mode&S_IFREG && sb.st_mode&S_IRUSR &&
                                                !strcmp(entry->d_name, file)) {
                  closedir(dp);
                  return 0; /*ok*/
               }
               if (sb.st_mode&S_IFDIR && subdirscount < MAXDIRS)
                  subdirs[subdirscount++] = strcpy(dirstore[subdirscount],
                                                                    rfullpath);
            }
            closedir(dp);
            subdirs[subdirscount] = 0;
            if (find_file(file, subdirs) == 0)
               return 0;
         }
         else
         {  /* not R_OK, but X_OK */
            makefullpath(rfullpath, file, dirp);
            if (access(rfullpath, R_OK) == 0)
               return 0; /*ok*/
         }
      }
      return 1; /* no such file */
   }
   return 0;
}

char* use_rules(char *rulefile, char **dirs) { /* load a given ruleset by name */
   char *errmsg = 0, *p;
   if (find_file(rulefile, dirs))
      return strerror(ENOENT); /* no such file */
   else
   {
      if ((errmsg = readrules(rfullpath)) == 0) {
         if (p = strrchr(rulefile, '/'))
            set_active_rules(p + 1, 1);
         else
            set_active_rules(rulefile, 1);
         strcpy(pathtorules, rfullpath);
         showrules();
         set_paintcolor();
      }
      return errmsg;
   }
}

static int setrules(char *rules) {
   char *errmsg;
   if (errmsg = use_rules(rules, dirs)) {
      sprintf(inpbuf, "Can't load %s: %s", rules, errmsg);
      err_rules();
      return 0;
   }
   return 1;
}

void loadrules(char *rn) { /* load rules from a transition-definition file */
   char rules[RULESNAMELEN];
   strcpy(rules, rn);
   if (cuttopology(rules, 1)) {
      if (setrules(rules))
         if (set_topology(topologyinfo) > 0) {
            sprintf(inpbuf, "Can't set topology %s", topologyinfo);
            err_rules();
         }
         else
            fixpatterntopology();
   }
   else
      setrules(rules);
   displaystats();
   showstates();
   showrules();
}

char* readcolors(char *pn) {
   char buf[BUFSIZ], s[BUFSIZ];
   int r, g, b, n, i = 0;
   FILE *fi = fopen(pn, "r");
   while (!feof(fi)) {
      fgets(buf, BUFSIZ, fi);
      if (sscanf(buf, " #%s", s) == 0) {
         if (sscanf(buf, " color = %d %d %d %d", &n, &r, &g, &b) == 4 &&
                                    n < MAXCOLORS && r < 256 && g < 256 && b < 256) {
            newcolors[i].cn = n;
            sprintf(s, "#%02x%02x%02x", r, g, b);
            strcpy(newcolors[i++].c, s);
         }
         else
            return (char*) "wrong format";
      }
   }
   fclose(fi);
   for (n = 0; n < i; n++) {
      r = newcolors[n].cn;
      strcpy(currentcolors[r], newcolors[n].c);
      setonecolor(r, newcolors[n].c);
      xgcv.foreground = exact.pixel;
      XChangeGC(disp, cellgc[r], GCForeground, &xgcv);
      if (r == 0)
         XSetWindowBackground(disp, lifew, cellcolor->pixel);
   }
   return 0;
}

char* use_colors(char *palfile, char **dirs) { /* load a given palette by name */
   if (find_file(palfile, dirs))
      return strerror(ENOENT); /* no such file */
   else
      return readcolors(rfullpath);
}

void loadcolors(char *pn) { /* load palette from a file */
   char *errmsg;
   if (strlen(pn) < 7 || strcmp(pn + strlen(pn) - 7, ".colors"))
      strcat(pn, ".colors");
   if (errmsg = use_colors(pn, dirs)) {
      sprintf(inpbuf, "Can't load %s: %s", chop_msg(pn, strlen(errmsg)), errmsg);
      announce_and_wait(RED_EVER);
      displaystats();
   }
   else {
      strcpy(colorfile, rfullpath);
      showstates();
   }
}
