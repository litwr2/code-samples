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
Several modifications were added at 2011 by Vladimir Lidovski vol.litwr@gmail.com
(C) This version of XLife may be used under the same conditions as mentioned above
$Id: collect.cpp 334 2014-04-05 09:11:35Z litwr $
*/

/* collect.c: Collects separate included files into one file 
              that uses pattern blocks */

#include <sys/types.h>
#include <pwd.h>
#include <dirent.h>
#include <stdlib.h>
#include "defs.h"
#include "file.h"
#include "colors.h"

static cellcount_t generations;
static FILE *out;

static char *deletelifeext(char *buf) {
   int len = strlen(buf);
   if (!strcmp(buf + len - strlen(LOADEXT), LOADEXT))
      buf[len - strlen(LOADEXT)] = '\0';
   return(buf);
}

static void add_loadreq1(LoadReq **loadqueue, char *patname, int relpath, unsigned long loadtime) {
   LoadReq *newreq;
  /* Find last entry where request can go while maintaining time order.
     Hopefully, this will cause the resulting file to be ordered in a nice
     way.

     After loop, we have a pointer to the pointer that must be modified
     (Could be pointer to list itself). */ 

   while (*loadqueue && (*loadqueue)->loadtime <= loadtime)
      loadqueue = &((*loadqueue)->next); 
/* Create new request block and load it up */
   newreq = (LoadReq*)malloc(sizeof(LoadReq));
   newreq->loadtime = loadtime;
   newreq->relpath = relpath;
/* Silently truncates long file names--should probably tell user */
   strncpy(newreq->patname, patname, PATNAMESIZ);
/* Insert new request in queue */
   newreq->next = *loadqueue;
   *loadqueue = newreq;
}

static int seen(LoadReq **seenlist, char *patname) {
   LoadReq *newpat;
/* Return whether pattern has been seen.  If not, add to list. */
   while (*seenlist && strcmp((*seenlist)->patname, patname))
      seenlist = &((*seenlist)->next); 
   if (*seenlist == NULL) {
/* Create new block and store name */
      newpat = (LoadReq*)malloc(sizeof(LoadReq));
      strncpy(newpat->patname, patname, PATNAMESIZ);
/* Insert new pattern on list */
      newpat->next = *seenlist;
      *seenlist = newpat;
      return 0;
   }
   return 1;
}

static int do_loadfile1(LoadReq **seenlist, LoadReq **loadqueue, int ismain) {
    char patname[PATNAMESIZ], patfield[PATNAMESIZ], *p;
    FILE *loadfl;
    char patfile[BUFSIZ];
    LoadReq *tmpqptr;
    int relpath;
/* Get load request */
    strcpy(patname, (*loadqueue)->patname);
    relpath = (*loadqueue)->relpath;
/* Delete request from queue */
    tmpqptr = (*loadqueue)->next;
    free(*loadqueue);
   (*loadqueue) = tmpqptr;
/* separate filename from pattern name */
    parse_patname(patname, patfield);
/* add load extension to filename if needed */
    strcpy(patname, add_opt_ext(patname, LOADEXT));
    if (patname[0] == '/')
        strcpy(patfile, patname);
    else
    {
        char    **dirp;
        for (dirp = dirs + 1; *dirp; dirp++) {
            DIR *dp;
            struct dirent *entry;
            if ((dp = opendir(*dirp))) {
                 while ((entry = readdir(dp))) {
                     sprintf(patfile, "%s/%s/%s",
                                    *dirp, entry->d_name, patname);
                     if (access(patfile, R_OK) == 0) {
                         closedir(dp);
                         goto foundit;
                     }
                 }
                 closedir(dp);
            }
            else {
                sprintf(patfile, "%s/%s", *dirp, patname);
                if (access(patfile, R_OK) == 0)
                    goto foundit;
            }
        }
        return 0;
    }
 foundit:
   while (p = strstr(patfile, "./"))  /* removes exessive part */
      do
         *p = p[2];
      while (*p++);
   if ((loadfl = fopen(patfile,"r")) == NULL)
      return 0;
   else {
      char buf[BUFSIZ], patid[PATNAMESIZ], pardir[PATNAMESIZ];
      int endpattern = 0, found = 0, relpathoffset = 0;

/* print comment telling where pattern was found */
      fprintf(out, "## FOUND IN (%s):\n", patfile);

        /* If we are searching for a specific pattern in the file,
           then we skip lines till we find it.  This is a pretty tacky way
           to handle multiple pattern definitions in the same file,
           but files with #I format probably won't get big enough for
           anyone to notice.  Really, we should implement a dictionary to
           save the location of a pattern definition in a file the first time
           we see it.
        */
        deletelifeext(patname + relpath);
        if (patfield[0]) {
           while (!found && fgets(buf, BUFSIZ, loadfl)) {
              if (buf[0] == '#' && buf[1] == 'B') {
                 sscanf(buf + 2, " %s", patid);
                 found = !strcmp(patfield, patid);
              }
           }
           if (!found) {
              fclose(loadfl);
              return 0;
           } else if (!ismain)
              fprintf(out, "#B %s.%s\n", patname + relpath, patfield);
        } else if (!ismain)
           fprintf(out, "#B %s\n", patname + relpath);
        while (fgets(buf, BUFSIZ, loadfl) && !endpattern) {
            if (buf[0] == '#') {
                char incl[BUFSIZ];
                char tmpstring[PATNAMESIZ];
                int xoff, yoff, rotate, flip;
                unsigned long loadtime;
                incl[0] = '\0';
                switch(buf[1]) {
                  case 'B':
                    /* Anything between #B and #E is ignored, since it
                       is a pattern definition to be included later.
                       #B and #E cannot be nested, so we just skip till
                       we pass #E */
                    while (fgets(buf, BUFSIZ, loadfl)
                           && !(buf[0] == '#' && buf[1] == 'E')) {}
                    break;

                  case 'E':
                    /* if we hit a "#E" at this point, then it must be
                       the one that closes the "#B <patfield>" that we
                       are reading (assuming a syntactically correct file) */
                    endpattern = 1;
                    if (!ismain) fprintf(out, "%s", buf);
                    break;

                  case 'I':
                    xoff = yoff = rotate = loadtime = 0;
                    flip = 1;
                    sscanf(buf + 2, " %s %d %d %d %d %ld", incl,
                                 &xoff, &yoff, &rotate, &flip, &loadtime);
                    /* Silently ignore invalid flips */
                    if (flip != 1 && flip != -1) flip=1;

                    /* if included pattern begins with ':' then assume
                       it's in active file */
                    if (incl[0] == ':') {
                       strcpy(tmpstring, patfile);
                       strcat(tmpstring, incl);
                       strcpy(incl, tmpstring);
                    } else {
                       /* if relative path given, assume directory of parent */
                       if (!strchr("/~", incl[0])) {
                          strcpy(pardir, patfile);
                          seppatdir(pardir, tmpstring);
                          relpathoffset = strlen(pardir);
                          strcat(pardir, incl);
                          strcpy(incl, pardir);
                       }
                    }
                    if (!seen(seenlist,incl))
                      add_loadreq1(loadqueue, incl, relpathoffset,
                                   generations + loadtime);
                    parse_patname(incl, tmpstring);
                    deletelifeext(incl);
                    if (tmpstring[0])
                       fprintf(out, "#I :%s.%s %d %d %d %d %ld\n", incl + relpathoffset,
                                tmpstring, xoff, yoff, rotate, flip, loadtime);
                    else
                       fprintf(out, "#I :%s %d %d %d %d %ld\n", incl + relpathoffset,
                                    xoff, yoff, rotate, flip, loadtime);
                    break;

                  default:
                    fprintf(out, "%s", buf);
                }
            }
            else fprintf(out, "%s", buf);
        }
        fclose(loadfl);
        if (patfield[0] == '\0' && !ismain) fprintf(out, "#E\n");
        fprintf(out, "\n");
        return 1;
   }
}

static void collect_loadreq(LoadReq **seenlist, LoadReq *loadqueue) {
   char thispat[PATNAMESIZ];
   strcpy(thispat,loadqueue->patname);
   if (!do_loadfile1(seenlist, &loadqueue, 1)) {
      sprintf(inpbuf, "Can't load (%s)\n", thispat);
      announce_and_wait(RED_EVER);
   }
   while (loadqueue) {
      generations = loadqueue->loadtime;
      strcpy(thispat, loadqueue->patname);
      if (!do_loadfile1(seenlist, &loadqueue, 0)) {
         sprintf(inpbuf, "Can't load (%s)\n", thispat);
         announce_and_wait(RED_EVER);
      }
   }
}

void collect(char *patname, int savemode) {
   char outbuf[PATNAMESIZ], result[PATNAMESIZ];
   LoadReq *loadqueue = NULL, *seenlist = NULL;
   if (!strcpy(outbuf, checktilda(patname)))
      return;
   if (savemode) {
      strcpy(result, outbuf);
      strcat(result, ".res");
      out = fopen(result, "w");
      fprintf(out, "##XLife\n");
   }
   else
      out = stdout;
   generations = 0;
   add_loadreq1(&loadqueue, outbuf, 0, generations);
   seen(&seenlist, outbuf);
   collect_loadreq(&seenlist, loadqueue);
   if (savemode) {
      fclose(out);
      remove(outbuf);
      rename(result, outbuf);
   }
}
