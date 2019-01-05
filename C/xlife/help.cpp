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
A lot of modifications were added at 2001, 2011-13 by Vladimir Lidovski vol.litwr@gmail.com
(C) This version of XLife may be used under the same conditions as mentioned above
$Id: help.cpp 362 2017-02-08 16:29:02Z litwr $
*/

#include "defs.h"
#include "file.h"
#include "history.h"
#include "tile.h"

static const char *strs[] = { 
#include "help.h"
0
};

void redraw_help(void) {
   int i;
   XClearWindow(disp, helpw);
   for (i = 0; strs[i]; i++)
      XDrawString(disp, helpw, itextgc, 10, i*(FONTHEIGHT - 1) + 16, strs[i],
                                                              strlen(strs[i]));
}

void help(void) {
   helpw_mode = -1;
LOOP:
   XRaiseWindow(disp, helpw);
   redraw_help();
   wait_activity(0);
   XLowerWindow(disp, helpw);
   if (event.type == KeyPress) {
      XLookupString(&event.xkey, keybuf, 16, &ks, 0);
      if (ks == XK_Shift_L || ks == XK_Shift_R || *keybuf == '?')
         goto LOOP;
      helpw_mode = 0;
      if (!DoKeySymIn(ks))
         if (*keybuf && strchr(
            "Cl!%^#bMZr*+=-hpc$/Egfms<>ojPWDNAVKGUIduLYXnRTFta@vSBiQ12346789H",
                                                                      *keybuf))
            DoKeyIn(keybuf); /* k.Ox()[]{}J05 -- excluded */
   }
   showstates();
}

static char* yesno(int d) {
   static char yes[] = "yes", no[] = "no";
   if (d)
      return yes;
   else
      return no;
}

void pseudocolor_msg(void) {
   switch (pseudocolor) {
      case 0:
         strcat(inpbuf, "no");
         break;
      case 1:
         strcat(inpbuf, "Mark out new cells");
         break;
      case 2:
         strcat(inpbuf, "Mark out new and removed cells");
         break;
      case 3:
         strcat(inpbuf, "Mark out removed cells");
         break;
      case 4:
         strcat(inpbuf, "Show only changed cells");
   }
}

void redraw_viewvars(void) {
   double ul_max = (1UL << 31)*2.;
   int k = 0, i;
   XClearWindow(disp, helpw);
   sprintf(inpbuf, "Window size: %dx%d", width, height);
   XDrawString(disp, helpw, itextgc, 10, ++k*FONTHEIGHT, inpbuf,
                                                               strlen(inpbuf));
   if (stashed[0]) {
      sprintf(inpbuf, "Last loaded filename: %s", stashed);
      XDrawString(disp, helpw, itextgc, 10, ++k*FONTHEIGHT, inpbuf,
                                                               strlen(inpbuf));
      sprintf(inpbuf, "Number of comment lines: %d", numcomments);
      XDrawString(disp, helpw, itextgc, 10, ++k*FONTHEIGHT, inpbuf,
                                                               strlen(inpbuf));
      if (outcome[0]) {
         sprintf(inpbuf, "#K line: %s", outcome);
         XDrawString(disp, helpw, itextgc, 10, ++k*FONTHEIGHT, inpbuf,
                                                               strlen(inpbuf));
      }
      sprintf(inpbuf, "Quantity of scripts to save: %d", i = loadscriptstat());
      XDrawString(disp, helpw, itextgc, 10, ++k*FONTHEIGHT, inpbuf,
                                                               strlen(inpbuf));
      if (i--) {
         sprintf(inpbuf, "%s", loadscriptfn(1));
         while (i--)
            sprintf(inpbuf + strlen(inpbuf), " %s", loadscriptfn(0));
         XDrawString(disp, helpw, itextgc, 25, ++k*FONTHEIGHT, inpbuf,
                                                               strlen(inpbuf));
      }
   }
   if (colorfile[0]) {
      sprintf(inpbuf, "Palette filename: %s", colorfile);
      XDrawString(disp, helpw, itextgc, 10, ++k*FONTHEIGHT, inpbuf,
                                                               strlen(inpbuf));
   }
   if (fname[0]) {
      sprintf(inpbuf, "Internal filename: %s", fname);
      XDrawString(disp, helpw, itextgc, 10, ++k*FONTHEIGHT, inpbuf,
                                                               strlen(inpbuf));
   }
   sprintf(inpbuf, "Topology: %c ", topology);
   if (limits) {
      if (limits&1)
         sprintf(inpbuf + strlen(inpbuf), "%ux", x_max_limit - x_min_limit);
      else
         sprintf(inpbuf + strlen(inpbuf), "%.0fx", ul_max);
      if (limits&2)
         sprintf(inpbuf + strlen(inpbuf), "%u", y_max_limit - y_min_limit);
      else
         sprintf(inpbuf + strlen(inpbuf), "%.0f", ul_max);
   }
   else
      sprintf(inpbuf + 12, "%.0fx%.0f", ul_max, ul_max);
   XDrawString(disp, helpw, itextgc, 10, ++k*FONTHEIGHT, inpbuf,
                                                               strlen(inpbuf));
   sprintf(inpbuf, "Origin: (%u, %u)", xorigin, yorigin);
   XDrawString(disp, helpw, itextgc, 10, ++k*FONTHEIGHT, inpbuf,
                                                               strlen(inpbuf));
   if (bounding_box(&active)) {
      sprintf(inpbuf, "Active pattern size: %dx%d",
                 active.xmax - active.xmin + 1, active.ymax - active.ymin + 1);
      XDrawString(disp, helpw, itextgc, 10, ++k*FONTHEIGHT, inpbuf,
                                                               strlen(inpbuf));
   }
   if (*pathtorules) {
      sprintf(inpbuf, "Rules filename: %s", pathtorules);
      XDrawString(disp, helpw, itextgc, 10, ++k*FONTHEIGHT, inpbuf,
                                                               strlen(inpbuf));
   }
   sprintf(inpbuf, "Delay for the evolution (msec): %u", delay);
   XDrawString(disp, helpw, itextgc, 10, ++k*FONTHEIGHT, inpbuf,
                                                               strlen(inpbuf));
   sprintf(inpbuf, "Jump length for `go' command: %d", hideperiod);
   XDrawString(disp, helpw, itextgc, 10, ++k*FONTHEIGHT, inpbuf,
                                                               strlen(inpbuf));
   sprintf(inpbuf, "Hash algorithm: %s", yesno(hashmode));
   XDrawString(disp, helpw, itextgc, 10, ++k*FONTHEIGHT, inpbuf,
                                                               strlen(inpbuf));
   if (hashmode) {//////
      char temp[25] = {0};
      switch(currentHashMode) {
         case 1:
            strcpy(temp, "growing step mode");
            break;
         case 2:
            strcpy(temp, "Golly like mode");
            break;     
         case 3:
            strcpy(temp, "constant step mode");
            break;
      }                        
      sprintf(inpbuf, "Current hash mode: %s", temp);
      XDrawString(disp, helpw, itextgc, 10, ++k*FONTHEIGHT, inpbuf,
                                                               strlen(inpbuf));                                                                 
   }
   sprintf(inpbuf, "Wireframe tentative pattern mode: %s", yesno(wireframe));
   XDrawString(disp, helpw, itextgc, 10, ++k*FONTHEIGHT, inpbuf,
                                                               strlen(inpbuf));
   sprintf(inpbuf, "Oscillator check mode: %s", yesno(oscillators));
   XDrawString(disp, helpw, itextgc, 10, ++k*FONTHEIGHT, inpbuf,
                                                               strlen(inpbuf));
   if (ev_mode == VALENCE_DRIVEN) {
      strcpy(inpbuf, "Pseudocolor mode: ");
      pseudocolor_msg();
      XDrawString(disp, helpw, itextgc, 10, ++k*FONTHEIGHT, inpbuf,
                                                               strlen(inpbuf));
   }
   sprintf(inpbuf, "History record mode: %s", yesno(truehistory));
   XDrawString(disp, helpw, itextgc, 10, ++k*FONTHEIGHT, inpbuf,
                                                               strlen(inpbuf));
   sprintf(inpbuf, "Density for the random filled area: %.2f%%",
                                                              rnd_density*100);
   XDrawString(disp, helpw, itextgc, 10, ++k*FONTHEIGHT, inpbuf,
                                                               strlen(inpbuf));
   if (randomseed) {
      sprintf(inpbuf, "Random generator seed: %d", randomseed);
      XDrawString(disp, helpw, itextgc, 10, ++k*FONTHEIGHT, inpbuf,
                                                               strlen(inpbuf));
   }
#ifdef HASARRAY
   hashstat(&active);
   XDrawString(disp, helpw, itextgc, 10, ++k*FONTHEIGHT, inpbuf,
                                                               strlen(inpbuf));
   if (tentative.tiles) {
      hashstat(&tentative);
      XDrawString(disp, helpw, itextgc, 10, ++k*FONTHEIGHT, inpbuf,
                                                               strlen(inpbuf));
   }
   if (truehistory) {
      historyhashstat(0);
      XDrawString(disp, helpw, itextgc, 10, ++k*FONTHEIGHT, inpbuf,
                                                               strlen(inpbuf));
      historyhashstat(1);
      XDrawString(disp, helpw, itextgc, 10, ++k*FONTHEIGHT, inpbuf,
                                                               strlen(inpbuf));
   }
#endif
   XDrawString(disp, helpw, itextgc, 5, (k + 2)*FONTHEIGHT,
                                                "Press a key to continue", 23);
}

void viewvars(void) {
   helpw_mode = -2;
   XRaiseWindow(disp, helpw);
   redraw_viewvars();
   wait_activity(0);
   XLowerWindow(disp, helpw);
   helpw_mode = 0;
   displaystats();
}

int redraw_comments(void) {
   int i = 0;
   XClearWindow(disp, helpw);
   while (height > (i + 3)*FONTHEIGHT && helpw_mode + i < numcomments) {
      XDrawString(disp, helpw, itextgc, 10, (i + 1)*FONTHEIGHT,
                   comments[helpw_mode + i], strlen(comments[helpw_mode + i]));
      i++;
   }
   XDrawString(disp, helpw, itextgc, 5, (i + 2)*FONTHEIGHT,
                                                "Press a key to continue", 23);
   return i;
}

void view_comments(void) {
   if (numcomments) {
      XRaiseWindow(disp, helpw);
      do {
         eo_comments = redraw_comments();
         wait_activity(0);
         helpw_mode += eo_comments;
      } while (helpw_mode < numcomments);
      XLowerWindow(disp, helpw);
   }
   eo_comments = helpw_mode = 0;
}

void redraw_slashinfo(void) {
   int i, q = 0, h, l;
   char s[80];
   long double t = 0, z = (1UL << 31)*2.;
   XClearWindow(disp, helpw);
   h = width/14/FONTWIDTH;
   if ((l = bounding_box(&active)) == 0)
      XDrawString(disp, helpw, itextgc, 10, FONTHEIGHT, "Life is extinct", 15);
   else {
      for (i = 1; i < maxstates; i++)
         if (active.cellcount[i]) {
            t += active.cellcount[i];
            sprintf(s, "[%d] %d", i, active.cellcount[i]);
            XDrawString(disp, helpw, itextgc, 10 + (q%h)*14*FONTWIDTH,
                                           (q/h + 1)*FONTHEIGHT, s, strlen(s));
            q++;
         }
      if (maxstates > 2) {
         sprintf(s, "[*] %.0Lf", t);
         XDrawString(disp, helpw, itextgc, 10 + (q%h)*14*FONTWIDTH,
                                           (q/h + 1)*FONTHEIGHT, s, strlen(s));
         q++;
      }
      if (limits) {
         if (limits&1)
            z = x_max_limit - x_min_limit;
         if (limits&2)
            z *= y_max_limit - y_min_limit;
         else
            z *= (1UL << 31)*2.;
      }
      else
         z *= z;
      sprintf(s, "[0] %.0Lf", z - t);
      XDrawString(disp, helpw, itextgc, 10, (q/h + 2)*FONTHEIGHT, s,
                                                                    strlen(s));
   }
   XDrawString(disp, helpw, itextgc, 5, (q/h + 5)*FONTHEIGHT,
                                                "Press a key to continue", 23);
}

void view_slashinfo(void) {
   helpw_mode = -3;
   XRaiseWindow(disp, helpw);
   if (hashmode)
      redraw_hash_slashinfo();
   else
      redraw_slashinfo();
   wait_activity(0);
   XLowerWindow(disp, helpw);
   helpw_mode = 0;
}
