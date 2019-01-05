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
 * A lot of modifications were added at 2011-13 by Vladimir Lidovski vol.litwr@gmail.com
 * (C) This version of XLife may be used under the same conditions as mentioned above
 * $Id: key.cpp 348 2014-05-18 03:22:59Z litwr $
 */

#include <stdlib.h>
#include "defs.h"
#include "tile.h"
#include "colors.h"

void cursorshow(Window w, int x, int y, GC gc) {
   static int t;
   if (mstime()&0x100) {
      if (t) XDrawString(disp, w, ntextgc, x, y, "_", 1), t = 0;
   }
   else
      if (!t) XDrawString(disp, w, gc, x, y, "_", t = 1);
}

void getxstring(void) {
    int offset = 0, buflen, windowlen = (width - BORDERWIDTH*2)/FONTWIDTH;
    char tmpinpbuf[INPBUFLEN];
    XLowerWindow(disp, rulew);
    XLowerWindow(disp, coordw);
    for (;;) {
        cursorshow(inputw,
		   INPUTXOFF + (strlen(inpbuf) >= windowlen ? windowlen - 1: strlen(inpbuf))*FONTWIDTH,
		   INPUTYOFF, itextgc);
        if (XCheckMaskEvent(disp, KeyPressMask|ButtonPressMask|Button1MotionMask
                |PointerMotionMask|Button3MotionMask|ButtonReleaseMask|ExposureMask
                |StructureNotifyMask, &event)) {
	   /* handle other kinds of events if no key is pressed  */
	   strcpy(tmpinpbuf, inpbuf);
	   switch(event.type) {
	   case ConfigureNotify:
	      DoResize();
	      continue;
	   case Expose:
	      DoExpose(event.xexpose.window);
           default:
              continue;
           case KeyPress:;
	   }
	   strcpy(inpbuf, tmpinpbuf);
	   /* KeyPress events only! */
	   XLookupString(&event.xkey, keybuf, 16, &ks, 0);
	   if (IsModifierKey(ks))
		continue;
	    /* compute window length for rescrolling */
           if (ks == XK_Return || ks == XK_Linefeed) {
EXIT:           XRaiseWindow(disp, rulew);
                XRaiseWindow(disp, coordw);
                return;
           }
	   if (ks == XK_Escape) {
	      inpbuf[minbuflen] = 0;
	      goto EXIT;
	   }
           if (ks == XK_BackSpace || ks == XK_Delete) {
			buflen = strlen(inpbuf);
			if (buflen > minbuflen) {
			    inpbuf[buflen - 1] = 0;
			    offset = (buflen > windowlen) ? buflen - windowlen : 0;
                            XClearWindow(disp, inputw);
			    XDrawString(disp, inputw, ntextgc, INPUTXOFF, INPUTYOFF, inpbuf + offset,
                                 (buflen - 1 >= windowlen ? windowlen - 1: buflen - 1));
			}
	   }
	   else
	   {
                        if (strlen(inpbuf) + strlen(keybuf) > INPBUFLEN)
                            keybuf[INPBUFLEN - strlen(inpbuf)] = 0;
			strcat(inpbuf, keybuf);
			buflen = strlen(inpbuf);
			offset = (buflen >= windowlen) ? buflen - windowlen + 1: 0;
			XClearWindow(disp, inputw);
			XDrawString(disp, inputw, ntextgc, INPUTXOFF, INPUTYOFF,
                            inpbuf + offset, buflen - offset);
	   }
	}
    }
}

void wait_activity(int m) {
   XLowerWindow(disp, rulew);
   XLowerWindow(disp, coordw);
   if (convmode) return;
   for (;;) {
      if (m) XSetWindowBackground(disp, inputw, cellcolor[m].pixel);
      XClearWindow(disp, inputw);
      XDrawString(disp, inputw, ntextgc, INPUTXOFF, INPUTYOFF, inpbuf,
                                                               strlen(inpbuf));
      if (m) XSetWindowBackground(disp, inputw, fcolor);
      XMaskEvent(disp, KeyPressMask|ButtonPressMask|Button1MotionMask
            |PointerMotionMask|Button3MotionMask|ButtonReleaseMask|ExposureMask
                                                  |StructureNotifyMask, &event);
      switch(event.type) {
      case ButtonPress:
      case KeyPress:
         XRaiseWindow(disp, rulew);
         XRaiseWindow(disp, coordw);
         return;
      case ConfigureNotify:
         DoResize();
         break;
      case Expose:
         DoExpose(event.xexpose.window);
      }
   }
}

void announce_and_wait(int m) {
   if (convmode) {
      fprintf(stderr, "%s\n", inpbuf);
      exit(2);
   }
   strcat(inpbuf, " (press a key)");
   wait_activity(m);
}

void test_transition(void) {
/* interactively probe the transitions table */
   char outbuf[100];
   static char TESTPROMPT[] = "Test transition: ";
   if (state == STOP) {
      char s, n1, n2, n3, n4, n5, n6, n7, n8;
      int ns;
      announce(TESTPROMPT);
      getxstring();
      strcpy(outbuf, inpbuf + sizeof(TESTPROMPT) - 1);
      if (ev_mode == TAB8_DRIVEN) {
         if (sscanf(outbuf, "%c%c%c%c%c%c%c%c%c", &s, &n1, &n2, &n3, &n4, &n5,
                           &n6, &n7, &n8) != 9 || !is_state(s) || !is_state(n1)
            || !is_state(n2) || !is_state(n3) || !is_state(n4) || !is_state(n5)
                            || !is_state(n6) || !is_state(n7) || !is_state(n8))
            announce("This command wants exactly nine valid state codes.");
         else {
            sprintf(outbuf, "%c%c%c%c%c%c%c%c%c", s, n1, n2, n3, n4, n5, n6,
                                                                       n7, n8);
            if ((ns = btrans[stoi(s)][stoi(n1)][stoi(n2)][stoi(n3)][stoi(n4)]
                         [stoi(n5)][stoi(n6)][stoi(n7)][stoi(n8)]) == BADSTATE)
               if (passive >= 0)
                  strcat(outbuf, " has no defined outcome");
               else
                  sprintf(outbuf + strlen(outbuf), ": %c", s);
            else
               sprintf(outbuf + strlen(outbuf), ": %c", itos(ns));
            announce(outbuf);
         }
      }
      else {
         if (sscanf(outbuf, "%c%c%c%c%c", &s, &n1, &n2, &n3, &n4) != 5
                              || !is_state(s) || !is_state(n1) || !is_state(n2)
                                             || !is_state(n3) || !is_state(n4))
            announce("This command wants exactly five valid state codes.");
         else {
            sprintf(outbuf, "%c%c%c%c%c", s, n1, n2, n3, n4);
            if ((ns = ztrans[stoi(s)][stoi(n1)][stoi(n2)][stoi(n3)][stoi(n4)])
                                                                   == BADSTATE)
               strcat(outbuf, " has no defined outcome.");
            else
               sprintf(outbuf + strlen(outbuf), ": %c", itos(ns));
            announce(outbuf);
         }
      }
   }
}

void set_transition(void) {
   char outbuf[100], n = 6;
   static char SETPROMPT[] = "Set transition: ";
   if (ev_mode == TAB8_DRIVEN) n = 10;
   if (state == STOP) {
      int s[n], i;
      FILE *fp;
      announce(SETPROMPT);
      getxstring();
      strcpy(outbuf, inpbuf + sizeof(SETPROMPT) - 1);
      if (strlen(outbuf) >= n) {
         for (i = 0; i < n; i++)
            if ((s[i] = stoi(outbuf[i])) < 0 || s[i] >= maxstates) {
error:
               strcpy(inpbuf, "Wrong transition");
               announce_and_wait(RED_EVER);
               displaystats();
               return;
            }
         if (ev_mode == TAB8_DRIVEN) {
            make_transition8(1, s[0], s[1], s[2], s[3], s[4], s[5], s[6], s[7],
                                                                   s[8], s[9]);
            sprintf(outbuf, "%c%c%c%c%c%c%c%c%c%c", itos(s[0]), itos(s[1]),
                    itos(s[2]), itos(s[3]), itos(s[4]), itos(s[5]), itos(s[6]),
                                           itos(s[7]), itos(s[8]), itos(s[9]));
         }
         else {
            make_transition(1, s[0], s[1], s[2], s[3], s[4], s[5]);
            sprintf(outbuf, "%c%c%c%c%c%c", itos(s[0]), itos(s[1]), itos(s[2]),
                                           itos(s[3]), itos(s[4]), itos(s[5]));
         }
         announce(outbuf);
         if (fp = fopen(PATCH_LOG, "a")) {
            fprintf(fp, "%s", outbuf);
            stamp("\t#", fp);
            fclose(fp);
         }
      }
      else
         goto error;
   }
}

cell_t patch_transition(cell_t s, cell_t n1, cell_t n2, cell_t n3, cell_t n4) {
   char outbuf[100], ns;
   static char PATCHPROMPT[] = "New state for neighborhood %c%c%c%c%c: ";
   sprintf(outbuf, PATCHPROMPT, itos(s), itos(n1), itos(n2), itos(n3),
                                                                     itos(n4));
   for (;;) {
      announce(outbuf);
      minbuflen = strlen(inpbuf);
      getxstring();
      ns = inpbuf[strlen(inpbuf) - 1];
      if (is_state(ns) && stoi(ns) < maxstates)
         break;
   }
   make_transition(0, s, n1, n2, n3, n4, stoi(ns));
   sprintf(inpbuf, "%c%c%c%c%c: %c", itos(s), itos(n1), itos(n2), itos(n3),
                                                                 itos(n4), ns);
   DoExpose(inputw);
   return stoi(ns);
}

cell_t patch_transition8(cell_t s, cell_t n1, cell_t n2, cell_t n3, cell_t n4,
                                  cell_t n5, cell_t n6, cell_t n7, cell_t n8) {
   char outbuf[100], ns;
   static char PATCHPROMPT[]
                           = "New state for neighborhood %c%c%c%c%c%c%c%c%c: ";
   sprintf(outbuf, PATCHPROMPT, itos(s), itos(n1), itos(n2), itos(n3),
                             itos(n4), itos(n5), itos(n6), itos(n7), itos(n8));
   for (;;) {
      announce(outbuf);
      minbuflen = strlen(inpbuf);
      getxstring();
      ns = inpbuf[strlen(inpbuf) - 1];
      if (is_state(ns) && stoi(ns) < maxstates)
         break;
   }
   make_transition8(0, s, n1, n2, n3, n4, n5, n6, n7, n8, stoi(ns));
   sprintf(inpbuf, "%c%c%c%c%c%c%c%c%c: %c", itos(s), itos(n1), itos(n2),
               itos(n3), itos(n4), itos(n5), itos(n6), itos(n7), itos(n8), ns);
   DoExpose(inputw);
   return stoi(ns);
}
