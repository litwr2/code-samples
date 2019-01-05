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
 * A lot of modifications were added at 2001, 2011-14 by Vladimir Lidovski vol.litwr@gmail.com
 * Several important additions were made at 2013-14 by Yaroslav Zotov zotovyaa@mail.ru
 * (C) This version of XLife may be used under the same conditions as mentioned above
 * $Id: main.cpp 364 2018-03-31 08:57:04Z litwr $
 */

#include <thread>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/Xatom.h>
#include <unistd.h>
#include <stdlib.h>
#include "defs.h"
#include "tile.h"
#include "file.h"
#include "xwidget.h"
#include "colors.h"
#include "clipboard.h"
#include "patchlevel.h"
#include "framebuffer.h"
#include "history.h"
#include "topology.h"
extern unsigned threads;
void phase1(unsigned);
extern thread thread_ref[];
extern unsigned finisher[];

#ifdef MICROSOFT
#ifndef WINVER
#define WINVER 0x0500//not needed with some compilers
#endif
//#include <windows.h>
#endif

unsigned universeChanged, insideHashAlgorithm = 0;//////////////hashlife
int currentHashMode = 1;//1 - "classical" hash mode (fast growing step), 2 - Golly mode (less memory consumption), 3 - constant step
char strBuf[120] = {'\0'};//////////////hashlife
int multiColor = 0, shortText;//////////////hashlife
int modeSelected = 0, convst;

#define returnIfInsideHashLife if(insideHashAlgorithm) return;
#define breakIfInsideHashLife returnIfInsideHashLife //#define breakIfInsideHashLife if(insideHashAlgorithm) break;
                         
#define HASHMODECHANGE(mode, description)\
	if(!(topology == 'Q' && !oscillators && !truehistory && !runcounter && !((born & 1) != 0 && (live & 256) == 0))) {\
		strcpy(inpbuf, "Hash algorithm is not supported for this rule");\
		announce_and_wait(RED_EVER);\
		break;\
	}\
	modeSelected = mode;\
	if (!hashmode)\
		toggle_hash();\
	currentHashMode = mode;\
	redrawModeWindow();\
	gc(1);\
	announce_and_delay(description);\
	displaystats();\
	XLowerWindow(disp, modew);\
	return;
	
#define TILEMODE\
	if(hashmode)\
		toggle_hash();\
	modeSelected = 0;\
	redrawModeWindow();\
	announce_and_delay("Tile algorithm activated");\
	displaystats();\
	state = STOP;\
	XLowerWindow(disp, modew);\
	return;      

/*#define	char	unsigned char*/
#include "icon.h"
#include "cursor.h"
/*#undef char*/

#ifndef GRAB_FRACTION
#define GRAB_FRACTION	0.8	/* fraction of screen to size */
#endif /* GRAB_FRACTION */

coord_t savex, savey;	/* last point seen during boxing */
static coord_t xmarker[3], ymarker[3], linex, liney;

void redrawModeWindow()
{
	XClearWindow(disp, modew);
	XRaiseWindow(disp, modew);
	XDrawString(disp, modew, ntextgc, 35, 20, "Choose algorithm", 16);
	if (modeSelected == 0)	 
		XFillRectangle(disp, modew, cellgc[BLUE_EVER]/*cellgc[2]*/, 28, 28, 154, 49);
	if (!hashmode)	 
		XFillRectangle(disp, modew, cellgc[GREEN_EVER], 30, 30, 150, 45);
	else
		XFillRectangle(disp, modew, cellgc[WHITE_EVER], 30, 30, 150, 45);		
	XDrawString(disp, modew, ntextgc, 85, 58, "Tile", 4);
	if(modeSelected == 1)
		XFillRectangle(disp, modew, cellgc[BLUE_EVER], 28, 78, 154, 49);		
	if (hashmode && currentHashMode == 1)	 
		XFillRectangle(disp, modew, cellgc[GREEN_EVER], 30, 80, 150, 45);
	else
		XFillRectangle(disp, modew, cellgc[WHITE_EVER], 30, 80, 150, 45);		
	XDrawString(disp, modew, ntextgc, 65, 108, "Hyperhash", 9);	
	if(modeSelected == 2)
		XFillRectangle(disp, modew, cellgc[BLUE_EVER], 28, 128, 154, 49);
	if (hashmode && currentHashMode == 2)	 
		XFillRectangle(disp, modew, cellgc[GREEN_EVER], 30, 130, 150, 45);
	else
		XFillRectangle(disp, modew, cellgc[WHITE_EVER], 30, 130, 150, 45);			
	XDrawString(disp, modew, ntextgc, 70, 158, "Fasthash", 8); 
	if(modeSelected == 3)
		XFillRectangle(disp, modew, cellgc[BLUE_EVER], 28, 178, 154, 49);		
	if (hashmode && currentHashMode == 3)	 
		XFillRectangle(disp, modew, cellgc[GREEN_EVER], 30, 180, 150, 45);
	else
		XFillRectangle(disp, modew, cellgc[WHITE_EVER], 30, 180, 150, 45);		
	XDrawString(disp, modew, ntextgc, 70, 208, "Stephash", 8);					
	XFlush(disp);
}

void redraw_lifew() {
   XClearWindow(disp, lifew);
   pivot = 0;
   fb_clear();
   drawboundedgrid();
   redisplay(MESH);
}

void DoExpose(Window win) {
   if (convmode) return;
   if (win == lifew) {
      XClearWindow(disp, lifew);
      pivot = 0;
      drawboundedgrid();
#if VFREQ != 0
      prev_vsync.tv_sec--;
#endif
      redisplay(MESH + EXPOSE);
   }
   else if (win == inputw) {
      XClearWindow(disp, inputw);
      XDrawString(disp, inputw, ntextgc, INPUTXOFF, INPUTYOFF, inpbuf,
                                                               strlen(inpbuf));
   }
   else if (win == rulew)
      showrules();
   else if (win == statew)
      showstates();
   else if (win == loadw)
      drawloadwidget();
   else if (win == helpw) {
      if (helpw_mode == -3) {
         if (hashmode)
            redraw_hash_slashinfo();
         else
            redraw_slashinfo();
      }
      else if (helpw_mode == -1)
         redraw_help();
      else if (helpw_mode < 0)
         redraw_viewvars();
      else
         eo_comments = redraw_comments();
   }
}

void ResizeLW(int oheight) {
   XResizeWindow(disp, rulew, rulew_len, INPUTH);
   XMoveWindow(disp, rulew, width - rulew_len
                    - dispcoord*(COORDW + BORDERWIDTH) - BORDERWIDTH, oheight);
   if (maxstates > 2) {
      if (width > maxstates*(FONTWIDTH + 3) - BORDERWIDTH - STATEOFF) {
         statescols = maxstates;
         XMoveWindow(disp, statew, width - maxstates*(FONTWIDTH + 3)
                                      - 3*BORDERWIDTH - STATEOFF, BORDERWIDTH);
         XResizeWindow(disp, statew, maxstates*(FONTWIDTH + 3) + STATEOFF,
                                                                       INPUTH);
      }
      else {
         int w, h;
         w = (width + BORDERWIDTH)/(FONTWIDTH + 3);
         h = maxstates/w + (maxstates%w? 1: 0);
         statescols = maxstates/h + (maxstates%h? 1: 0);
         XMoveWindow(disp, statew, width - statescols*(FONTWIDTH + 3)
                                      - 3*BORDERWIDTH - STATEOFF, BORDERWIDTH);
         XResizeWindow(disp, statew, statescols*(FONTWIDTH + 3) + STATEOFF,
                                                (FONTHEIGHT + 3)*h + STATEOFF);
      }
      showstates();
   }
   else
      XLowerWindow(disp, statew);
   XMoveWindow(disp, coordw, width - dispcoord*(COORDW + BORDERWIDTH)
                                                       - BORDERWIDTH, oheight);
}

void DoResize(void) {
   int owidth = width, oheight = height;
   if ((width = event.xconfigure.width) < 50)
      XResizeWindow(disp, mainw, width = 50, height);
   if ((height = event.xconfigure.height) < 70)
      XResizeWindow(disp, mainw, width, height = 70);
   xpos += sgn(owidth - width)*shr(abs(owidth - width), scale + 1);
   ypos += sgn(oheight - height)*shr(abs(oheight - height), scale + 1);
   bigXPos += (bigNumber)sgn(owidth - width)*shr(abs(owidth - width), scale + 1);
   bigYPos += (bigNumber)sgn(oheight - height)*shr(abs(oheight - height), scale + 1);
   SETBIGSCREENWIDTH;
   SETBIGSCREENHEIGHT;
   inputlength = width/FONTWIDTH;
   oheight = height - INPUTH - BORDERWIDTH*3;
   if (oheight < 5) oheight = 5;
   ResizeLW(oheight);
   XResizeWindow(disp, lifew, width - BORDERWIDTH*2, oheight);
   XResizeWindow(disp, helpw, width - BORDERWIDTH*2, height - BORDERWIDTH*2);
   if (helpw_mode || eo_comments) DoExpose(helpw);
   XMoveWindow(disp, inputw, 0, oheight);
   XResizeWindow(disp, inputw, width - BORDERWIDTH*2, INPUTH);
}

void alloc_states(unsigned new_ev_mode, unsigned newmaxstates) {
   if (maxstates == newmaxstates && new_ev_mode == ev_mode)
      return;
   confirmload(); /* it is necessary for changesize */
/*    if (newmaxstates < maxstates) clear_all();*/
   clipboard_copy(&active);
   if (new_ev_mode == VALENCE_DRIVEN)
      changesize(sizeof(cellbox::twostate_t));
   else if (new_ev_mode == PAYOFF_DRIVEN || new_ev_mode == TAB8_DRIVEN)
      changesize(sizeof(cellbox::pstate_t));
   else
      changesize(sizeof(cellbox::nstate_t)); /* also gstate_t */
   ev_mode = new_ev_mode;
   clipboard_flush(&active, maxstates = newmaxstates);
   ResizeLW(height - INPUTH - BORDERWIDTH*3);
}

int DoKeySymIn(KeySym keysym) {
   int dx, dy;
   coord_t sxpos, sypos;
   switch (keysym) {
   case XK_4:
   case XK_KP_4:
   case XK_Left:
   case XK_KP_Left:
      prep_tentative();
      if (event.xkey.state & ShiftMask) {
         xpos -= dx = SCALE(width/4);
         bigXPos -= bigDX = SCALE((bigNumber)width/4);
         SETBIGSCREENWIDTH;
         SETBIGSCREENHEIGHT;
      } else {
         xpos -= dx = (scale >= 0) ? 1 : 1 << -scale;
         bigXPos -= bigDX = (scale >= 0) ? 1 : 1 << (bigNumber)-scale;
         SETBIGSCREENWIDTH;
         SETBIGSCREENHEIGHT;         
      }
      display_move(0, -dx, 0);
      break;
   case XK_6:
   case XK_KP_6:
   case XK_Right:
   case XK_KP_Right:
      prep_tentative();
      if (event.xkey.state & ShiftMask) {
         xpos += dx = SCALE(width/4);
         bigXPos += bigDX = SCALE((bigNumber)width/4);
         SETBIGSCREENWIDTH;
         SETBIGSCREENHEIGHT;
      } else {
         xpos += dx = (scale >= 0) ? 1 : 1 << -scale;
         bigXPos += bigDX = (scale >= 0) ? 1 : 1 << (bigNumber)-scale; 
         SETBIGSCREENWIDTH;
         SETBIGSCREENHEIGHT;
      }
      display_move(0, dx, 0);
      break;
   case XK_8:
   case XK_KP_8:
   case XK_Up:
   case XK_KP_Up:
      prep_tentative();
      if (event.xkey.state & ShiftMask) {
         ypos -= dy = SCALE(height/4);
         bigYPos -= bigDY = SCALE((bigNumber)height/4);
         SETBIGSCREENWIDTH;
         SETBIGSCREENHEIGHT;
      } else {
         ypos -= dy = (scale >= 0) ? 1 : 1 << -scale;
         bigYPos -= bigDY = (scale >= 0) ? 1 : 1 << (bigNumber)-scale;
         SETBIGSCREENWIDTH;
         SETBIGSCREENHEIGHT;
      }
      display_move(0, 0, -dy);
      break;
   case XK_2:
   case XK_KP_2:
   case XK_Down:
   case XK_KP_Down:
      prep_tentative();
      if (event.xkey.state & ShiftMask) {
         ypos += dy = SCALE(height/4);
         bigYPos += bigDY = SCALE((bigNumber)height/4);
         SETBIGSCREENWIDTH;
         SETBIGSCREENHEIGHT;         
      } else {
         ypos += dy = (scale >= 0) ? 1 : 1 << -scale;
         bigYPos += bigDY = (scale >= 0) ? 1 : 1 << (bigNumber)-scale;   
         SETBIGSCREENWIDTH;
         SETBIGSCREENHEIGHT;               
      }
      display_move(0, 0, dy);
      break;
   case XK_Home:
   case XK_7:
   case XK_KP_7:
   case XK_KP_Home:
      prep_tentative();
      if (event.xkey.state & ShiftMask) {
         xpos -= dx = SCALE(width/4);
         bigXPos -= bigDX = SCALE((bigNumber)width/4);
         ypos -= dy = SCALE(height/4);
         bigYPos -= bigDY = SCALE((bigNumber)height/4);
         SETBIGSCREENWIDTH;
         SETBIGSCREENHEIGHT;         
      }
      else {
         xpos -= dx = (scale >= 0) ? 1 : 1 << -scale;
         bigXPos -= bigDX = (scale >= 0) ? 1 : 1 << (bigNumber)-scale;
         ypos -= dy = (scale >= 0) ? 1 : 1 << -scale;
         bigYPos -= bigDY = (scale >= 0) ? 1 : 1 << (bigNumber)-scale; 
         SETBIGSCREENWIDTH;
         SETBIGSCREENHEIGHT;         
      }
      display_move(0, -dx, -dy);
      break;
   case XK_Page_Up:
   case XK_9:
   case XK_KP_9:
   case XK_KP_Page_Up:
      prep_tentative();
      if (event.xkey.state & ShiftMask) {
         xpos += dx = SCALE(width/4);
         bigXPos += bigDX = SCALE((bigNumber)width/4);
         ypos -= dy = SCALE(height/4);
         bigYPos -= bigDY = SCALE((bigNumber)height/4);
         SETBIGSCREENWIDTH;
         SETBIGSCREENHEIGHT;         
      }
      else {
         xpos += dx = (scale >= 0) ? 1 : 1 << -scale;
         bigXPos += bigDX = (scale >= 0) ? 1 : 1 << (bigNumber)-scale;
         ypos -= dy = (scale >= 0) ? 1 : 1 << -scale;
         bigYPos -= bigDY = (scale >= 0) ? 1 : 1 << (bigNumber)-scale;
         SETBIGSCREENWIDTH;
         SETBIGSCREENHEIGHT;         
      }
      display_move(0, dx, -dy);
      break;
   case XK_Page_Down:
   case XK_3:
   case XK_KP_3:
   case XK_KP_Page_Down:
      prep_tentative();
      if (event.xkey.state & ShiftMask) {
         xpos += dx = SCALE(width/4);
         bigXPos += bigDX = SCALE((bigNumber)width/4);
         ypos += dy = SCALE(height/4);
         bigYPos += bigDY = SCALE((bigNumber)height/4);
         SETBIGSCREENWIDTH;
         SETBIGSCREENHEIGHT;         
      }
      else {
         xpos += dx = (scale >= 0) ? 1 : 1 << -scale;
         bigXPos += bigDX = (scale >= 0) ? 1 : 1 << (bigNumber)-scale;
         ypos += dy = (scale >= 0) ? 1 : 1 << -scale;
         bigYPos += bigDY = (scale >= 0) ? 1 : 1 << (bigNumber)-scale;
         SETBIGSCREENWIDTH;
         SETBIGSCREENHEIGHT;         
      }
      display_move(0, dx, dy);
      break;
   case XK_End:
   case XK_1:
   case XK_KP_1:
   case XK_KP_End:
      prep_tentative();
      if (event.xkey.state & ShiftMask) {
           xpos -= dx = SCALE(width/4),
           ypos += dy = SCALE(height/4);
           bigXPos -= bigDX = SCALE((bigNumber)width/4);
           bigYPos += bigDY = SCALE((bigNumber)height/4);
           SETBIGSCREENWIDTH;
           SETBIGSCREENHEIGHT;           
      }
      else {
           xpos -= dx = (scale >= 0) ? 1 : 1 << -scale;
           bigXPos -= bigDX = (scale >= 0) ? 1 : 1 << (bigNumber)-scale;
           ypos += dy = (scale >= 0) ? 1 : 1 << -scale;
           bigYPos += bigDY = (scale >= 0) ? 1 : 1 << (bigNumber)-scale;
           SETBIGSCREENWIDTH;
           SETBIGSCREENHEIGHT;
      }
      display_move(0, -dx, dy);
      break;
   case XK_5:
   case XK_KP_5:
   case XK_KP_Begin:
      prep_tentative();
      sxpos = xpos, sypos = ypos;
      if (hashmode)
         hashMiddleScr();
      else {
         center();
         display_move(0, xpos - sxpos, ypos - sypos);
      }
      break;
   case XK_0:
   case XK_KP_0:
   case XK_KP_Insert:
      prep_tentative();
      sxpos = xpos, sypos = ypos;
      median();
      display_move(0, xpos - sxpos, ypos - sypos);
      break;
   case XK_Help:
      help();
      break;
   case XK_Tab:
      if (tentative.tiles && paintcolor > 1)
         change_tentative_color();
      break;
   default:
      return 0;
   }
/* could process it */
   return 1;
}

void inc_scale(void) {
   if (scale < MAXSCALE) {
      setscale(++scale);
      xpos += SCALE(event.xmotion.x);
      ypos += SCALE(event.xmotion.y);  
      if (hashmode) {  
         bigXPos += SCALE((bigNumber)event.xmotion.x);
         bigYPos += SCALE((bigNumber)event.xmotion.y);     
         SETBIGSCREENWIDTH;
         SETBIGSCREENHEIGHT;
      }   
      redraw_lifew();
   }
}

void dec_scale(void) {
   if (hashmode) {
      bigXPos -= SCALE((bigNumber)event.xmotion.x); 
      bigYPos -= SCALE((bigNumber)event.xmotion.y); 
      xpos -= SCALE(event.xmotion.x);     
      ypos -= SCALE(event.xmotion.y);      
      setscale(--scale);
      SETBIGSCREENWIDTH;
      SETBIGSCREENHEIGHT;      
      redraw_lifew();
      return;
   }
   if (scale > MINSCALE) {
      xpos -= SCALE(event.xmotion.x);     
      ypos -= SCALE(event.xmotion.y);    
      setscale(--scale);
      redraw_lifew();      
   }   
}

void set_active_rules(const char *s, int force) {
   int i = rulew_len;
   strcpy(active_rules, s);
   rulew_len = (strlen(s) + 1)*FONTWIDTH;
   if (force && i != rulew_len)
      ResizeLW(height - INPUTH - BORDERWIDTH*3);
}

int rules_changed(void) {
   char *pn = active_rules, *po = saved_rules;
   if (hashmode && strcmp(po,pn))//hashlife rules changed
      return 1;
   if (!strcmp(pn, po))
      return 0;
   if (strstr(pn, po));
   else if (strstr(po, pn)) {
      po = active_rules;
      pn = saved_rules;
   }
   else
      return 1;
   if (!strcmp(pn + strlen(po), "+"))
      return 0;
   return 1;
}

void fixpatterntopology(void) {
   if (bounding_box(&active) && limits && (active.xmin < x_min_limit && x_min_limit
              || active.xmax >= x_max_limit && x_max_limit
              || active.ymin < y_min_limit && y_min_limit
              || active.ymax >= y_max_limit && y_max_limit)) {
             make_tentative(active.xmin, active.ymin, active.xmax, active.ymax);
             copy_tentative();
             clear_pattern(&tentative);
        }
}

static int fix_mouse_coord(int coord, int limit) {
   if (coord < 0)
      return 0;
   if (coord > limit)
      return limit - 40;
   return coord;
}

static void adjust_pivotpos() {
   if (tentative.tiles && bounding_box(&tentative) < 1000000) {
/* don't change pivot position for the very big pattern */
      clipboard_copy(&tentative);
      clipboard.x -= tentative.xmin - STARTX;
      clipboard.y -= tentative.ymin - STARTY;
      if (!wireframe) boxpattern(0);
      clear_pattern(&tentative);
      clipboard_flush(&tentative, maxstates);
      if (!wireframe) boxpattern(CYAN_EVER);
   }
}

void toggle_hash(void) {
   if (!hashmode)
      confirmload();
   if (hashmode = !hashmode) {
      unsigned long n, m;
      dispboxes = 1;
      n = bounding_box(&active);
      m = active.generations;
      if (topology != 'Q' || oscillators || truehistory || runcounter || (born & 1) != 0 && (live & 256) == 0)
        {
           strcpy(inpbuf, "Hash algorithm is not supported for this rule");
           announce_and_wait(RED_EVER);
           return;
        }
      //initHashlife();    
      clearHashLife();
      initRootNode(&active, active.xmax, active.xmin, active.ymax, active.ymin);            
      clear_pattern(&active);
      //confirmload();
      active.cellcount[1] = n;
      active.generations = m;
      universeChanged = 0;
      bigXPos = xpos; 
      bigYPos = ypos; 
      SETBIGSCREENWIDTH;
      SETBIGSCREENHEIGHT;
   }
   else
      hash2tile(ev_mode);
}

void DoKeyIn(char *kbuf) {
int i;
    int num, dx, dy;
    switch(kbuf[0]) {
    case 0:
       return;

    case 'r':
        redraw_lifew();
        displaystats();
        break;

    case 'R':
        breakIfInsideHashLife;
        strcpy(saved_rules, active_rules);
        newrules(0);
        displaystats();
        if (rules_changed()) {
           if (hashmode && (topology != 'Q' || oscillators || truehistory || runcounter || (born & 1) != 0 && (live & 256) == 0))
           {
              strcpy(inpbuf, "Hash algorithm is not supported for this rule");
              announce_and_wait(RED_EVER);
              break;
           }
           free_loadscript();
           if (hashmode)
           {
    	      hash2tile(ev_mode);
    	      clearHashLife();
    	      bounding_box(&active);
              initRootNode(&active, active.xmax, active.xmin, active.ymax, active.ymin);
              fb_flush();
              redisplay(0);
    	   }
    	   else
    	      clearHashLife();
    	   universeChanged = 1;
        }
        showrules();  
        break;

    case 'X':
        breakIfInsideHashLife;
        widget_main(WIDST_LOAD_PALETTE);
        break;

    case 'Z':
        InitCurrentColors();
        DefaultPalette();
        redisplay(MESH);
        break;

    case 'z':   	     
        breakIfInsideHashLife;            
        redrawModeWindow();
        while(1)
          while (XCheckMaskEvent(disp, KeyPressMask|ButtonPressMask|Button1MotionMask
	        |PointerMotionMask|Button3MotionMask|ButtonReleaseMask|ExposureMask
	        |StructureNotifyMask, &event)) {
	    switch(event.type) {
	      case Expose:
	        redrawModeWindow();
	        redraw_lifew();
	        break;    
	      case ConfigureNotify:
	        redrawModeWindow();	      
		DoResize();
                redraw_lifew();
		break;
	      case KeyPress:
	        XLookupString(&event.xkey, keybuf, 16, &ks, 0);
	        if(ks == XK_Escape || ks == 'Q') {//hide modew window
	           XLowerWindow(disp, modew);
	           return;//exit cycle
	        }
	        if (ks == 't' || ks == 'T') {
	           TILEMODE;
	        }
	        if (ks == 'h' || ks == 'H') {
	           HASHMODECHANGE(1, "Hash algorithm activated: hyperstep");
	        }
	        if (ks == 'f' || ks == 'F') {
	           HASHMODECHANGE(2, "Hash algorithm activated: faststep");	
	        }	           	                   	        
	        if (ks == 's' || ks == 'S') {
	           HASHMODECHANGE(3, "Hash algorithm activated: conststep");
	        }
	        if (ks == XK_Up) {
	           modeSelected = modeSelected == 0 ? 3 : modeSelected - 1;
	           redrawModeWindow();
	        }
	        if (ks == XK_Down) {
	           modeSelected = (modeSelected + 1)%4;
	           redrawModeWindow();
	        } 
	        if (ks == XK_Return) {
	           switch (modeSelected) {
	              case 0:
	                TILEMODE;
	                break;
	              case 1:
	                HASHMODECHANGE(1, "Hash algorithm activated: hyperstep");
	                break;
	              case 2:
	                HASHMODECHANGE(2, "Hash algorithm activated: faststep");
	                break;
	              case 3:
	                HASHMODECHANGE(3, "Hash algorithm activated: conststep");
	                break;	                	                	                
	           }
	        } 	        	                 
		break;
	      case ButtonPress:
	        if (event.xbutton.window == modew && event.xbutton.button == 1) {
	           if (event.xbutton.x > 28 && event.xbutton.x < 182 && event.xbutton.y > 28 && event.xbutton.y < 77) {
	              TILEMODE;
	           }
	           if (event.xbutton.x > 28 && event.xbutton.x < 182 && event.xbutton.y > 78 && event.xbutton.y < 127) {
	              HASHMODECHANGE(1, "Hash algorithm activated: hyperstep");
	           }
	           if (event.xbutton.x > 28 && event.xbutton.x < 182 && event.xbutton.y > 128 && event.xbutton.y < 177) {
	              HASHMODECHANGE(2, "Hash algorithm activated: faststep");	                                      
	           }
	           if (event.xbutton.x > 28 && event.xbutton.x < 182 && event.xbutton.y > 178 && event.xbutton.y < 227) {
	              HASHMODECHANGE(3, "Hash algorithm activated: conststep");	              	      
	           }        	              	              
	        }
                break;
	    }
        }
        break;

    case 'q':
       {
           char inpTxt[25];
           sprintf(inpTxt, "Input options (%c%c): ", multiColor == 1 ? 'M' : 'm', shortText == 1 ? 'S' : 's');
           announce(inpTxt);// 'M' for multicolor / 'm' for onecolor, 'S' for short text / 's' for full)
           minbuflen = 20;
           getxstring();
           char tmp[20] = {0}, *p;
           strcpy(tmp, inpbuf + minbuflen);
           int flag = 1;
           for (p = tmp; *p && flag; p++) {
              switch (*p) {
                 case 'M':
                    multiColor = 1;
                    announce_and_delay("Hash multicolor mode chosen");
                    break;
                 case 'm':
                    multiColor = 0;
                    announce_and_delay("Hash two colors mode chosen");
                    break;
                 case 'S':
                    shortText = 1;
                    announce_and_delay("Short strings for the status line chosen");
                    break;
                 case 's':
                    shortText = 0;
                    announce_and_delay("Long strings for the status line chosen");
                    break;
                 default:
                    sprintf(inpbuf, "Wrong option '%c'", *p);
                    insideHashAlgorithm = 0;
                    announce_and_wait(RED_EVER);
                    flag = 0;
              }
           }
           displaystats();
           redraw_lifew();
        }
        break;

    case 'F':
        breakIfInsideHashLife;
        strcpy(saved_rules, active_rules);
        widget_main(WIDST_LOAD_RULE);
        break;

    case '=':
    case '+':
        inc_scale();
        break;

    case '-':
        dec_scale();
        break;

    case '.':
        prep_tentative();
        xpos += dx = SCALE(event.xbutton.x - width/2);
        ypos += dy = SCALE(event.xbutton.y - height/2);
        bigXPos += (bigNumber)SCALE(event.xbutton.x - width/2);
        bigYPos += (bigNumber)SCALE(event.xbutton.y - height/2);
        display_move(0, dx, dy);
        break;

    case 'g':
        if (state != STOP && maxstates > 2)
           XRaiseWindow(disp, statew);
        else
           XLowerWindow(disp, statew);
        if (hashmode) {
           state = state == HASHRUN ? STOP : HASHRUN;
           lastx--;
           break;
        }
        breakIfInsideHashLife;        
        confirmload();
        state = state == RUN ? STOP : RUN;
        lastx--;
        bounding_box(&active);
        redraw_lifew();
        break;

    case '@':
        breakIfInsideHashLife;
        if (ev_mode == VALENCE_DRIVEN || historymode) {
           int flag = 0;
           if (hashmode) {
    	      hash2tile(ev_mode);
    	      flag = 1;
    	   }
           char s[100];
           confirmload();
           strcpy(s, active_rules);
           if (historymode)
              s[strlen(s) - 1] = 0;
           else
              strcat(s, "+");
           if (flag)
              hashmode = 0;
           file_rules(s);
           if (flag) {
              hashmode = 1;
    	      clearHashLife();
    	      bounding_box(&active);
              initRootNode(&active, active.xmax, active.xmin, active.ymax, active.ymin);
              fb_flush();
              redisplay(0);
           }
        }
        else {
           strcpy(inpbuf, "History is only allowed for 2-state automata");
           announce_and_wait(RED_EVER);
           displaystats();
        }
        break;

    case 'H':
        breakIfInsideHashLife;
        if (truehistory = !truehistory) {
           confirmload();
           clearhistorytiles();
           pattern2history(&active, 0);
           announce("History record mode is on");
        }
        else {
           redisplay(0);
           announce("History record mode is off");
        }
        break;

    case 'c':
        breakIfInsideHashLife;    
        bounding_box(&active);
        if (oscillators) {
osc_on_msg:
	   strcpy(inpbuf, "Oscillator check mode is on!");
           announce_and_wait(RED_EVER);
	}
        else
	   dispboxes ^= 1;
        displaystats();
	break;

    case '*':
        dispspeed = !dispspeed;
        displaystats();
        break;

    case '$':
        breakIfInsideHashLife;
        bounding_box(&active);
        if (oscillators)
	  goto osc_on_msg;
        else
          dispchanges ^= 1;
        displaystats();
        break;

    case 'P':
        breakIfInsideHashLife;
        if (oscillators ^= 1) {
           save_dispboxes = dispboxes;
           save_dispchanges = dispchanges;
           dispboxes = dispchanges = 1;
           confirmload();
           copypattern(&active, &oscillator_check);
           generate(&active);
           generate(&oscillator_check);
           oscillator_check.generations = active.generations;
           strcpy(inpbuf, "Oscillator check mode is on!");
           announce_and_wait(GREEN_EVER);
           state = RUN;
        }
        else {
           dispboxes = save_dispboxes;
           dispchanges = save_dispchanges;
	   announce("Oscillator check mode is off");
        }
        break;

    case 'E':
        breakIfInsideHashLife;
        if ((pseudocolor = (pseudocolor + 1)%5) > 0) {
           pseudopaint[0][1] = pseudocolor == 1 ? 0 : 2;
           pseudopaint[1][0] = pseudocolor == 3 ? 1 : 3;
           pseudopaint[1][1] = pseudocolor == 4 ? 0 : 1;
           strcpy(inpbuf, "Pseudocolor mode: ");
           pseudocolor_msg();
           announce(inpbuf);
        }
        else
           announce("Pseudocolor mode is off");
        redisplay(MESH);
        break;

    case 'o':
        breakIfInsideHashLife;
        if (hashmode) {
           if (universeChanged) {
              toggle_hash(); //bad!
              toggle_hash();
              universeChanged = 0;
	   }
	   hashGenerate(&active, strBuf);
           lastx--;
           fb_flush();
           redisplay(MESH);
           displaystats();
           break;
        }
        /*else {
           toggle_hash();
           announce_and_delay("Hash algorithm stopped - Tile algorithm activated");
        }*/
        universeChanged = 1;
        num = tentative.tiles != 0;
        confirmload();
        generate(&active);
        lastx--;
        redisplay(num);
        displaystats();
        break;

    case 'p':
	if (dispcoord)
	    dispcoord = FALSE;
	else
	{
	    lastx--;	/* fake move to force coordinates to print */
	    dispcoord = TRUE;
	}
/* force resize of input window */
	ResizeLW(height - INPUTH - BORDERWIDTH*3);
	showrules();
	break;

    case 'n': {
        coord_t sxpos = xpos, sypos = ypos;
        prep_tentative();
        current_tile();
        display_move(0, xpos - sxpos, ypos - sypos);
        break;
    }

    case '(':
    case '[':
    case '{':
        breakIfInsideHashLife;
	for (num = 0; kbuf[0] != "([{"[num]; num++);
	xmarker[num] = XPOS(event.xmotion.x);
	ymarker[num] = YPOS(event.xmotion.y);
	sprintf(inpbuf, "Marker %d set to (%d,%d)",
		num, xmarker[num] - xorigin, ymarker[num] - yorigin);
	DoExpose(inputw);
	break;

    case ')':
    case ']':
    case '}':
        breakIfInsideHashLife;
	for (num = 0; kbuf[0] != ")]}"[num]; num++);
	xpos += xmarker[num] - XPOS(event.xmotion.x);
	ypos += ymarker[num] - XPOS(event.xmotion.y);
	redraw_lifew();
        break;

   case 'J':
      announce("Jump to x, y: ");
      minbuflen = 14;
      getxstring();
      {
         int x, y;
         if (sscanf(inpbuf + 13, "%d, %d", &x, &y) == 2) {
            xpos += x + xorigin - XPOS(event.xmotion.x);
            ypos += y + yorigin - YPOS(event.xmotion.y);
            redraw_lifew();
         }
         displaystats();
      }
      break;

   case 'T':
      breakIfInsideHashLife;
      confirmload();
      announce("Enter topology: ");
      minbuflen = 16;
      getxstring();
      if (!inpbuf[16])
         goto exit_T;
      {
         char s[80];
         strcpy(s, inpbuf + 16);
         strncpy(inpbuf, " Wrong topology ", 16);
         if (set_topology(s) > 0) {
            announce_and_wait(RED_EVER);
            goto exit_T;
         }
      }
      fixpatterntopology();
      redraw_lifew();
      showrules();
exit_T:
      displaystats();
      break;

    case 'j':
      breakIfInsideHashLife;
      if (hashmode) {
          announce("Enter step size: ");
          minbuflen = 17;
          getxstring();
          setStep(inpbuf + minbuflen);
          DoExpose(inputw);
      } else {
          announce("Enter jump length: ");
          minbuflen = 19;
          getxstring();
          {
             unsigned x;
             if (sscanf(inpbuf + 19, "%u", &x) == 1 && x < 200000 && x > 0)
                hideperiod = x;
          }
          if (hideperiod < 2)
             strcpy(inpbuf, "No jump");
          else
             sprintf(inpbuf, "Jump set to %u", hideperiod);
          DoExpose(inputw);
      }
      break;

    case 'O':
	announce("Origin set to active cell");
	xorigin = XPOS(event.xmotion.x);
	yorigin = YPOS(event.xmotion.y);
        showcoord(1);
	break;

    case 'Y':
        breakIfInsideHashLife;
        if (tentative.tiles) {
           clear_pattern(&active);
           copy_tentative();
           clear_pattern(&tentative);
           redraw_lifew();
           displaystats();
        }
        break;

    case 'C':
        //breakIfInsideHashLife;
        if (insideHashAlgorithm)
           insideHashAlgorithm = 2;
        if (tentativeRoot)
        {
           tentativeRoot = 0;//zero
           redraw_lifew();
           return;
        }
        clearHashLife();
        if (tentative.tiles)
           clear_pattern(&tentative);
        else {
           setscale(scale = 3);
           clear_pattern(&active);
           free_loadscript();
           numcomments = outcome[0] = 0;
           speed = 0;
           state = STOP;
           displaystats();
           center();
        }
        redraw_lifew();
	break;

    case 'S':
        //breakIfInsideHashLife;    
        widget_main(WIDST_SAVE);
	break;

    case 'W':
/* Confirm latest load before saving script */
        breakIfInsideHashLife;
        confirmload();
        savex = XPOS(fix_mouse_coord(event.xmotion.x, width));
        savey = YPOS(fix_mouse_coord(event.xmotion.y, height));
        if (loadscript == loadscript->next) {
            strcpy(inpbuf, "There is no script to save!");
	    announce_and_wait(RED_EVER);
	    displaystats();
	}
	else
            widget_main(WIDST_SAVE_SCRIPT);
	break;

    case 'D':
        breakIfInsideHashLife;    
	free_loadscript();
        numcomments = outcome[0] = 0;
	if (tentative.tiles) {
	    clear_pattern(&tentative);
	    redraw_lifew();
	    announce("Load script (and latest load) discarded");
	}
	else
	    announce("Load script discarded"); 
	break;

    case 'l':
        breakIfInsideHashLife;    
        confirmload();        
        iloadx = loadx = XPOS(fix_mouse_coord(event.xmotion.x, width));
        iloady = loady = YPOS(fix_mouse_coord(event.xmotion.y, height));     
        widget_main(WIDST_LOAD_PATTERN);       
        adjust_pivotpos();
        break;

    case 'L':
        breakIfInsideHashLife;    
        if (stashed[0]) {
           confirmload();
           iloadx = loadx = XPOS(fix_mouse_coord(event.xmotion.x, width));
           iloady = loady = YPOS(fix_mouse_coord(event.xmotion.y, height));
           strcpy(saved_rules, active_rules);
           if (!loadfile_req(stashed)) {
              sprintf(inpbuf, "Can't load %s", stashed);
              goto ERROUT;
           }
           adjust_pivotpos();
           if (rules_changed())
              free_loadscript();
        }
        else {
	   strcpy(inpbuf, "No previous load");
ERROUT:
           announce_and_wait(RED_EVER);
           displaystats();
	}
        break;

    case 'u':
        breakIfInsideHashLife;    
        prep_tentative();
        txx = tyy = 1;
        txy = tyx = 0;
        loadx = iloadx;
        loady = iloady;
        display_move(0, 0, 0);
        break;

    case 'h':
        breakIfInsideHashLife;    
	confirmload();
	if (state == HIDE) {
            state = RUN;
	    redraw_lifew();
	}
	else
	    state = HIDE;
	break;

    case '/':
        breakIfInsideHashLife;    
        view_slashinfo();
        break;

    case 'b':
        if(hashmode) {
            hashBoundingBox();
            break;
        }
        //breakIfInsideHashLife;    
        if (bounding_box(&active) == 0)
            sprintf(inpbuf, "Life is extinct");
        else
            sprintf(inpbuf,
                    "Life bounds: %d <= x <= %d  %d <= y <= %d",
                    active.xmin - xorigin, active.xmax - xorigin,
                    active.ymin - yorigin, active.ymax - yorigin);
        announce_and_wait(0);
        displaystats();
        break;

    case '?':
	help();
	break;

    case 'f':
        breakIfInsideHashLife;    
	settimeout(delay = DELAY_FAST);
	break;

    case 'm':
        breakIfInsideHashLife;    
	settimeout(delay = DELAY_MED);
	break;

    case 's':
        breakIfInsideHashLife;    
	settimeout(delay = DELAY_SLOW);
	break;

    case '>':
        breakIfInsideHashLife;    
        if (delay >= DELAY_INCREMENT)
            settimeout(delay -= DELAY_INCREMENT);
        break;

    case '<':
        breakIfInsideHashLife;    
        settimeout(delay += DELAY_INCREMENT);
        break;

    case '!':
        breakIfInsideHashLife;    
        if (hashmode)
    	   hash2tile(ev_mode);
	randomize(width, height, xpos, ypos);
	universeChanged = 1;
	break;

    case '%': {
           breakIfInsideHashLife;
           float r;
           announce("Input density (% or nothing): ");
           minbuflen = 30;
           getxstring();
           if (sscanf(inpbuf + 30, "%f", &r) == 1)
               rnd_density = r/100;
           sprintf(inpbuf, "Using %.2f%% density", rnd_density*100);
           DoExpose(inputw);
        }
        break;

   case 'i':               /* set evolution counter */
        breakIfInsideHashLife;
        announce("Number of generations to perform (default = infinite): ");
        minbuflen = 55;
        getxstring();
        if (sscanf(inpbuf + 55, "%u", &runcounter) != 1)
           runcounter = 0;
        displaystats();
        break;

    case '#': /* toggle wireframe tentative pattern mode */
        breakIfInsideHashLife;
        strcpy(inpbuf, "Wireframe mode is o");
        if (wireframe ^= 1)
           strcat(inpbuf, "n");
        else
           strcat(inpbuf, "ff");
        DoExpose(inputw);
        if (tentative.tiles)
             redraw_lifew();
        break;

    case 'M': /* toggle gridmode */
        dispmesh ^= 1;
        redraw_lifew();
        break;

    case '^': {
           breakIfInsideHashLife;
	   unsigned x = 0;
           announce("Set random seed (integer number): ");
           minbuflen = 17;
           getxstring();
	   if (sscanf(inpbuf + 16, "%u", &x) == 1)
	       srandom(randomseed = x);
	   sprintf(inpbuf, "Random seed is set to %u", x);
	   DoExpose(inputw);
        }
        break;

    case 'N':
	name_file();
	break;

    case 'A':
	comment();
	break;

    case 'V':
	view_comments();
	break;

    case 'K':
        breakIfInsideHashLife;
        numcomments = outcome[0] = 0;
        announce("Comments are discarded");
        break;

    case 'k':
        breakIfInsideHashLife;
        if (hashmode && (XPOSBIG(event.xmotion.x) >= 4294967295lu || XPOSBIG(event.xmotion.x) < 0 || YPOSBIG(event.xmotion.y) >= 4294967295lu || YPOSBIG(event.xmotion.y) < 0)) {
           strcpy(inpbuf, "picking colors is not supported outside int borders");
           announce_and_wait(RED_EVER);      
           break;
        }
        num = lookcell(&active, XPOS(event.xmotion.x), YPOS(event.xmotion.y));
        setcolor(num, 0, 0);
        break;

    case 'v':
        breakIfInsideHashLife;
        viewvars();
        break;

    case 'B':
        breakIfInsideHashLife;
        if (hashmode) {
           announce("Number of generations to perform (default = current step): ");
           minbuflen = 59;
           getxstring();
           doSomeHashSteps(inpbuf + minbuflen, 1);
           redisplay(MESH);
           displaystats();
           break;
        }
	benchmark();
        bounding_box(&active);
	break;

    case 'Q':
        //XCloseDisplay(disp);
	exit(0);

    case 'U': /* Get rid of loaded pattern */
        breakIfInsideHashLife;
        if (stashed[0]) {
           clear_pattern(&tentative);
           redraw_lifew();
        }
        break;

    case 'I': /* Force confirm of loaded pattern */
        breakIfInsideHashLife;
        confirmload();
        display_move(1, 0, 0);
        break;

    case 'd':
        breakIfInsideHashLife;
        copy_tentative();
        break;

    case 'G': /* perform some generations on loaded pattern */
        breakIfInsideHashLife;
        if (hashmode) {
           announce("Number of generations to perform (default = current step): ");
           minbuflen = 59;
           getxstring();
           doSomeHashSteps(inpbuf + minbuflen, 0);
           redisplay(MESH);
           displaystats();
        } else {
           prep_tentative();
           genload();
           lastx--;
           bounding_box(&active);
           display_move(0, 0, 0);
        }
        break;

    case 'x':
       breakIfInsideHashLife;
       if (tentative.tiles) {
         long dlx, dly, tx, ty;
         prep_tentative();
         clipboard_copy(&tentative);
         loadx += dlx = XPOS(event.xbutton.x) - loadx;
         loady += dly = YPOS(event.xbutton.y) - loady;
         iloadx += tx = (dly*txy - dlx*tyy)/(txy*tyx - txx*tyy);
         iloady += ty = (dlx*tyx - dly*txx)/(txy*tyx - txx*tyy);
         clipboard.x -= tx;
         clipboard.y -= ty;
         clear_pattern(&tentative);
         clipboard_flush(&tentative, maxstates);
       }
       break;

    case ':': //add a thread
        breakIfInsideHashLife;
        if (threads < MAX_THREADS) {
            ++threads;
            displaystats();
        }
        break;

    case ';': //delete a thread
        breakIfInsideHashLife;
        if (threads > 1) {
            --threads;
            displaystats();
        }
        break;

    case 'a':
        breakIfInsideHashLife;
        if (ev_mode == TABLE_DRIVEN || ev_mode == TAB8_DRIVEN)
	  set_transition();
	break;

    case 't':
        breakIfInsideHashLife;
        if (ev_mode == TABLE_DRIVEN || ev_mode == TAB8_DRIVEN)
	  test_transition();
    }
    kbuf[0] = '\0';	/* get rid of old keystroke so shift doesn't bring it back */
}

static int cell2line(cell_t c) {
   if (chgcell(&active, linex, liney, c)) {
      drawcell(RXPOS(linex), RYPOS(liney), c);
      fb_ins_old(linex, liney, c);
      return 1;
   }
   return 0;
}

void Motion(void) { /* handle X motion events */
    returnIfInsideHashLife;
    if (event.xmotion.window == lifew) {
        coord_t x = XPOS(event.xmotion.x), y = YPOS(event.xmotion.y);
        if (tentative.tiles || tentativeRoot) {
           if (event.xmotion.state & Button1MotionMask)
              moveload();
           return;
        }
        if (event.xmotion.state & Button1MotionMask) {
            int changed = 0;
            if (!fix_mouse_coord(event.xmotion.x, width) || !fix_mouse_coord(event.xmotion.y, height))
               return;
            if (hashmode && (XPOSBIG(event.xmotion.x) >= 4294967295lu || XPOSBIG(event.xmotion.x) < 0 || YPOSBIG(event.xmotion.y) >= 4294967295lu || YPOSBIG(event.xmotion.y) < 0)) {
               strcpy(inpbuf, "Drawing is not allowed outside int borders");
               announce_and_wait(RED_EVER);      
               return;
            }               
            while (abs(linex - x) > 0 || abs(liney - y) > 0) {
               linex += sgn((int)(x - linex)), liney += sgn((int)(y - liney));
               if (limits && !chk_limits(linex, liney))
                  break;
               if (cell2line(paintcolor)) changed = 1;
            }
            if (changed) displaystats();
        }
        else if (event.xmotion.state & Button3MotionMask) {
            /* erase the old box, draw the new one */
          if (!limits || chk_limits(loadx, loady)) {
             if (limits) {
                if (limits&1)
                  if (x < x_min_limit)
                    x = x_min_limit;
                  else if (x >= x_max_limit)
                    x = x_max_limit - 1;
                if (limits&2)
                  if (y < y_min_limit)
                    y = y_min_limit;
                  else if (y >= y_max_limit)
                    y = y_max_limit - 1;
             }
             if (savex != x || savey != y) {
                rightX = XPOSBIG((bigNumber)event.xmotion.x);
                bottomY = YPOSBIG((bigNumber)event.xmotion.y);
                erasebox(loadx, loady, savex, savey);
                drawbox(loadx, loady, savex = x, savey = y, ORANGE_EVER);
             }
          }
        }
        else if (event.xmotion.state & Button2MotionMask) {
            int changed = 0;
            if (!fix_mouse_coord(event.xmotion.x, width) || !fix_mouse_coord(event.xmotion.y, height))
               return;            
            while (abs(linex - x) > 0 || abs(liney - y) > 0) {
               linex += sgn((int)(x - linex)), liney += sgn((int)(y - liney));
               if (limits && !chk_limits(linex, liney))
                  break;
               if (cell2line(0)) changed = 1;
            }
            if (changed) displaystats();
        }
    }
}

void Button(void) { /* handle a button-press event */
    if (event.xbutton.window == statew) {
        returnIfInsideHashLife;
        if (event.xbutton.button == 1)
	    setcolor(-1, event.xbutton.x, event.xbutton.y);
        else
            entercolor(event.xbutton.x, event.xbutton.y);
    } else if (event.xbutton.window == lifew) {
        int dy;
        if(event.xbutton.button != 5 && event.xbutton.button != 4)
            returnIfInsideHashLife;
        switch (event.xbutton.button) {
        case 5:
            prep_tentative();
            ypos += dy = (scale >= 0) ? 5 : 5 << -scale;
            bigYPos += bigDX = (scale >= 0) ? 5 : 5 << (bigNumber)-scale; 
            SETBIGSCREENWIDTH;
            SETBIGSCREENHEIGHT;
            display_move(0, 0, dy);
            return;
        case 4:
            prep_tentative();
            ypos -= dy = (scale >= 0) ? 5 : 5 << -scale;
            bigYPos -= bigDY = (scale >= 0) ? 5 : 5 << (bigNumber)-scale;
            SETBIGSCREENWIDTH;
            SETBIGSCREENHEIGHT            
            display_move(0, 0, -dy);
            return;
        }
        if (tentative.tiles || tentativeRoot)
          switch (event.xbutton.button) {
          case 1:
            moveload();
            break;
          case 2:
            flipload();
            break;
          case 3:
            turnload();
          }     
        else {
          coord_t x = XPOS(event.xmotion.x), y = YPOS(event.xmotion.y);
          if (hashmode && (XPOSBIG(event.xmotion.x) >= 4294967295lu || XPOSBIG(event.xmotion.x) < 0 || YPOSBIG(event.xmotion.y) >= 4294967295lu || YPOSBIG(event.xmotion.y) < 0) && event.xbutton.button != 3) {
             strcpy(inpbuf, "Drawing is not allowed outside int borders");
             announce_and_wait(RED_EVER);      
             return;
          } 
          switch(event.xbutton.button) {
          case 1:
            if (!limits || chk_limits(x, y)) {
              showcoord(TRUE);
              linex = x, liney = y; 
              if (cell2line(paintcolor)) displaystats();
            }
            break;
          case 2:
            if (!limits || chk_limits(x, y)) {
              showcoord(TRUE);                     
              linex = x, liney = y;
              if (cell2line(0)) displaystats();
            }
            break;
          case 3:
            savex = loadx = x;
            savey = loady = y;
            tentativeX = XPOSBIG((bigNumber)event.xmotion.x);
            tentativeY = YPOSBIG((bigNumber)event.xmotion.y);
            drawbox(x, y, x, y, ORANGE_EVER);
          }
        }
    }
}

void Release() {
   returnIfInsideHashLife;
   if (event.xbutton.window == lifew && !tentative.tiles && !tentativeRoot
                                                 && event.xbutton.button == 3) {
      int dx = 0, dy = 0;
      if (loadx == savex && savey == loady) {
         drawbox(loadx, loady, loadx, loady, 0);
         xpos += dx = SCALE(event.xbutton.x - width/2);
         ypos += dy = SCALE(event.xbutton.y - height/2);
      }
      else {
         int t = hashmode;
         hashmode = 0;
         if (event.xkey.state & ShiftMask)
            randomize(abs(RXPOS(loadx) - RXPOS(savex)) + RSCALE(1), 
                                   abs(RYPOS(loady) - RYPOS(savey)) + RSCALE(1),
                                          min(loadx, savex), min(loady, savey));
         if (t) {
            make_tentative_hash(tentativeX, tentativeY, rightX, bottomY);    
            erasebox(loadx, loady, savex, savey);
            drawbox(loadx, loady, savex, savey, CYAN_EVER);
         }    
         else
            make_tentative(loadx, loady, savex, savey);
         hashmode = t;
         displaystats();
      }
      display_move(0, dx, dy);
    }
}

static XFontStruct* selectmainfont() {
   XFontStruct *f;
   char fontspec[80];
   if (f = XLoadQueryFont(disp, NORMALFONT)) return f;
   sprintf(fontspec, "-*-courier-medium-r-*--%d-*-*-*-*-*-*-*", FONTHEIGHT);
   if (f = XLoadQueryFont(disp, fontspec)) return f;
   sprintf(fontspec, "-*-courier-*-r-*--%d-*-*-*-*-*-*-*", FONTHEIGHT);
   if (f = XLoadQueryFont(disp, fontspec)) return f;
   return 0;
}

static XFontStruct* selectsmallfont() {
   XFontStruct *f;
   if (f = XLoadQueryFont(disp, "6x9")) return f;
   if (f = XLoadQueryFont(disp, "-*-*-medium-r-*--9-*-*-*-*-*-*-*")) return f;
   if (f = XLoadQueryFont(disp, "-*-*-*-r-*--8-*-*-*-*-*-*-*")) return f;
   if (f = XLoadQueryFont(disp, "-*-courier-bold-r-*--8-*-*-*-*-*-*-*")) return f;
   if (f = XLoadQueryFont(disp, "-*-*-medium-r-*--10-*-*-*-*-*-*-*")) return f;
   if (f = XLoadQueryFont(disp, "-*-*-*-r-*--9-*-*-*-*-*-*-*")) return f;
   if (f = XLoadQueryFont(disp, "-*-*-medium-r-*--11-*-*-*-*-*-*-*")) return f;
   if (f = XLoadQueryFont(disp, "-*-*-medium-r-*--12-*-*-*-*-*-*-*")) return f;
   if (f = XLoadQueryFont(disp, "*6x*")) return f;
   if (f = XLoadQueryFont(disp, "*7x*")) return f;
   return selectmainfont();
}

void make_delay(long sec, long msec) {
   struct timeval inputdelay;
   inputdelay.tv_sec = sec;
   inputdelay.tv_usec = msec;
   select(32, 0, 0, 0, &inputdelay);
}

static int check_param(int b) {
   if (b)
      fatal("Usage: xlife [-display disp] [-geometry geom]  [-l pattern-lib] [filename]\n");
   return 1;
}

static void init_load(char *initpat) {
   char *s = checktilda(initpat);
   if (s == 0) return;
   loadx = xpos;
   loady = ypos;
   txx = tyy = 1;
   txy = tyx = 0;
   if (loadfile_req(s))
      adjust_pivotpos();
}

void eventHandler() {
	while (XCheckMaskEvent(disp, KeyPressMask|ButtonPressMask|Button1MotionMask
	        |PointerMotionMask|Button3MotionMask|ButtonReleaseMask|ExposureMask
	        |StructureNotifyMask, &event)) {
	    switch(event.type) {
	      case KeyPress:
		XLookupString(&event.xkey, keybuf, 16, &ks, 0);
		if (!DoKeySymIn(ks))
                    if (event.xkey.window == lifew || !strchr("(){}[].JOkx", *keybuf))
		        DoKeyIn(keybuf);
		break;
	      case MotionNotify:
/* don't allow drawing until load is confirmed */
                Motion();
		break;
	      case ButtonPress:
		Button();
		break;
	      case ButtonRelease:
		Release();
		break;
	      case ConfigureNotify:
		DoResize();
                redraw_lifew();
		break;
	      case Expose:
	        DoExpose(event.xexpose.window);
	    }
        }
}

#ifdef MICROSOFT
int wmain(int argc, char **argv) {
#else
int main(int argc, char **argv) {
#endif
    int i, qgens = 0;
    Cursor cursor;
    Pixmap icon, cursor_data, cursor_mask;
    XSizeHints hints;
    XWMHints wm_hints;
    XClassHint class_hints;
    XSetWindowAttributes winat;
    XColor white, black;
    char *geomstring = 0;
    char *initpat = 0;
    XTextProperty iconName;
    unsigned long cwhite, cblack;
    char *icon_name = (char*) "Xlife";
    char *display = getenv("DISPLAY");
    if (strstr(argv[0], "lifeconv")) {
      saveformat = 0;
      convmode++;
      for (i = 1; i < argc; i++)
        if (*argv[i] == '-')
              if (strchr("ACDIMPRSpv?lg4", argv[i][1]))
                  switch (argv[i][1]) {
                     case '?':
le:                     fatal("Usage: lifeconv -ACDIMPRSpv? [-l pattern-lib] [-g n] filename\n");
                     case 'v':
                        printf("Lifeconv/XLife v%d.%d.%d\n", majorversion,
                                                  minorversion, patchlevel);
                        return 0;
                     case 'g':
                        if (argc <= i + 1 || sscanf(argv[++i], "%d", &qgens) != 1) goto le;
                        break;
                     case 'l':
                        if (argc <= i + 1) goto le;
                        strcpy(matchlibfile, argv[++i]);
                        break;
                     case 'p':
                        oldp++;
                        argv[i][1] = 'P';
                     default:
                        if (saveformat != 0) goto le;
                        saveformat = argv[i][1];
                  }
              else
                 goto le;
        else if (initpat == 0)
            initpat = argv[i];
    }
    else
      for (i = 1; i < argc; i++)
	if (!strcmp(argv[i], "-geometry") && check_param(argc <= i + 1))
	    geomstring = argv[++i];
        else if (!strcmp(argv[i], "-l") && check_param(argc <= i + 1))
            strcpy(matchlibfile, argv[++i]);
        else if (!strcmp(argv[i], "-display") && check_param(argc <= i + 1))
            display = argv[++i];
	else if (*argv[i] != '-')
	    initpat = argv[i];
    if (convmode && (initpat == 0 || saveformat == 0)) goto le;
    if (!getcwd(inidir, PATNAMESIZ))
        fatal("Too long dirname\n");
    if (!display)
        fatal("No display defined!\n");
    if (!(disp = XOpenDisplay(display)))
        fatal("Can't open X display\n");
    if (XDefaultDepth(disp, screen) < STATEBITS)
        fatal("Not enough colors for compiled STATEBITS value\n");
#if VFREQ != 0
    gettimeofday(&prev_vsync, 0);
#endif
    setcurdir(LIFEDIR); /*inidir*/
    fileinit();
    initcells(&active);
    initcells(&tentative);
    xpos = xorigin = lastx = STARTX;
    ypos = yorigin = lasty = STARTY;
    dispcoord = TRUE;
    dispchanges = dispboxes = FALSE;
    state = STOP;
    gentab();
    srandom(time(0));

    screen = DefaultScreen(disp);
    rootw = RootWindow(disp, screen);
    fcolor = cwhite = WhitePixel(disp, screen);
    bcolor = cblack = BlackPixel(disp, screen);

    hints.x = hints.y = 0;
    maxwidth = width = DisplayWidth(disp, screen);
    inputlength = width/FONTWIDTH;
    height = DisplayHeight(disp, screen);
    if ((pfb = (FB*) malloc(i = height*width*sizeof(struct FB))) == 0)
       fatal("Not enough memory for the framebuffer");
    else
       memset(pfb, 0, i);
    hints.width = width*GRAB_FRACTION;
    hints.height = height*GRAB_FRACTION;
    hints.flags = PPosition | PSize;
    if (geomstring) {
	int result = XParseGeometry(geomstring, &hints.x, &hints.y,
                            (unsigned*)&hints.width, (unsigned*)&hints.height);
	if (result & XNegative)
	    hints.x += (DisplayWidth(disp, screen) - hints.width)*GRAB_FRACTION;
	if (result & YNegative)
	    hints.y += (DisplayHeight(disp, screen) - hints.height)*GRAB_FRACTION;
	if (result & XValue || result & YValue) {
	    hints.flags |= USPosition;
	    hints.flags &= ~PPosition;
	}
	if (result & WidthValue || result & HeightValue) {
	    hints.flags |= USSize;
	    hints.flags &= ~PSize;
	}
    }
    mainw = XCreateSimpleWindow(disp, rootw,
		hints.x, hints.y, hints.width, hints.height, 0, fcolor, bcolor);
    if (!mainw)
	fatal("Can't open main window\n");
    icon = XCreateBitmapFromData(disp, mainw, (const char*)icon_bits, icon_width, icon_height);
    if (XStringListToTextProperty(&icon_name, 1, &iconName) == 0)
        fatal("structure allocation for iconName failed.\n");
    wm_hints.initial_state = NormalState;
    wm_hints.input = True;
    wm_hints.icon_pixmap = icon;
    wm_hints.flags = IconPixmapHint | StateHint | InputHint;
    class_hints.res_name =  argv[0];
    class_hints.res_class = (char*) "Basicwin";
    XSetWMProperties(disp, mainw, 0, &iconName, argv, argc, &hints, &wm_hints, &class_hints);
    IniPalette();
    black = cellcolor[0];
    white = cellcolor[1];
/* text display is forced to black on white */
    xgcv.background = cwhite;
    xgcv.foreground = cblack;
    ntextgc = XCreateGC(disp, mainw, GCForeground | GCBackground, &xgcv);
    xgcv.background = cblack;
    xgcv.foreground = cwhite;
    itextgc = XCreateGC(disp, mainw, GCForeground | GCBackground, &xgcv);
/* create XOR GC for pivot display */
    xgcv.foreground = cellcolor[CYAN_EVER].pixel;
    xgcv.function = GXxor;
    xorgc = XCreateGC(disp, mainw, GCForeground | GCFunction, &xgcv);
    if (!(nfont = selectmainfont()))
       fatal("Can't load font\n");
    XSetFont(disp, ntextgc, nfont->fid);
    XSetFont(disp, itextgc, nfont->fid);
    XSetFont(disp, cellgc[LOADW_BG], nfont->fid);
    XSetFont(disp, xorgc, nfont->fid);
    cfont = selectsmallfont();
    xgcv.function = GXxor;
    xgcv.foreground = cellcolor[ORANGE_EVER].pixel;
    invgc = XCreateGC(disp, mainw, GCForeground | GCFunction, &xgcv);
    XSetFont(disp, invgc, cfont->fid);
    cursor_data = XCreateBitmapFromData(disp, mainw, (const char*)cursor_data_bits,
                cursor_data_width, cursor_data_height);
    cursor_mask = XCreateBitmapFromData(disp, mainw, (const char*)cursor_mask_bits,
                cursor_mask_width, cursor_mask_height);
    cursor = XCreatePixmapCursor(disp, cursor_data, cursor_mask, &white,
                &black, cursor_data_x_hot, cursor_data_y_hot);
    XDefineCursor(disp, mainw, cursor);
    width = hints.width;
    height = hints.height;
    set_active_rules("life", 0);
    lifew = XCreateSimpleWindow(disp, mainw,
		0, 0, width - BORDERWIDTH*2,
                height - INPUTH - BORDERWIDTH*3, BORDERWIDTH, fcolor, bcolor);
    helpw = XCreateSimpleWindow(disp, mainw,
		0, 0, width - BORDERWIDTH*2, height - BORDERWIDTH*2,
                BORDERWIDTH, cwhite, cblack);
    coordw = XCreateSimpleWindow(disp, mainw,
		width - COORDW - BORDERWIDTH*2,
		height - INPUTH - BORDERWIDTH*2,
                COORDW, INPUTH, BORDERWIDTH, cblack, cwhite);
    rulew = XCreateSimpleWindow(disp, mainw,
		width - COORDW - rulew_len - BORDERWIDTH*2,
		height - INPUTH - BORDERWIDTH*2,
                rulew_len, INPUTH, BORDERWIDTH, cblack, cwhite);
    statew = XCreateSimpleWindow(disp, mainw,
                width, BORDERWIDTH, 1, INPUTH, BORDERWIDTH,
                cblack, cwhite);
    loadw = XCreateSimpleWindow(disp, mainw,
                50, /*LOADW_HEIGHT + 50 > height ? 0 : 40*/4, LOADW_WIDTH, LOADW_HEIGHT, BORDERWIDTH,
                cblack, cellcolor[LOADW_BG].pixel);
    inputw = XCreateSimpleWindow(disp, mainw, 0,
                height - INPUTH - BORDERWIDTH*2, width - BORDERWIDTH*2,
                INPUTH, BORDERWIDTH, cblack, cwhite);
    modew = XCreateSimpleWindow(disp, mainw, 200,//modew
                100, 210, 260, BORDERWIDTH, cblack, cellcolor[LOADW_BG].pixel);    
    winat.win_gravity = SouthGravity;
    XChangeWindowAttributes(disp, inputw, CWWinGravity, &winat);
    XSelectInput(disp, mainw,
#ifndef MICROSOFT
                KeyPressMask |
#endif
                ExposureMask | StructureNotifyMask);
    XSelectInput(disp, inputw, ButtonPressMask | ExposureMask);
    XSelectInput(disp, lifew, KeyPressMask | ButtonPressMask | Button1MotionMask
       | PointerMotionMask | Button3MotionMask | ButtonReleaseMask | ExposureMask);
    XSelectInput(disp, helpw, KeyPressMask | ButtonPressMask | ExposureMask);
    XSelectInput(disp, rulew, ExposureMask);
    XSelectInput(disp, statew, ExposureMask | ButtonPressMask);
    XSelectInput(disp, loadw, KeyPressMask | ExposureMask | ButtonPressMask
                                        | PointerMotionMask | LeaveWindowMask);
    XSelectInput(disp, coordw, ExposureMask);
    XSelectInput(disp, modew, ButtonPressMask | ButtonReleaseMask);//modew
    alloc_states(VALENCE_DRIVEN, 2);
    setscale(scale = 3);
    settimeout(delay = DELAY_FAST);
/*
 * Only accept one pattern since it is highly unlikely that overlaying
 * n patterns is what you want to do
 */
    if (saveformat == 'C') {
        collect(initpat, 0);
        return 0;
    }
    if (convmode) {
       init_load(initpat);
       i = bounding_box(&tentative);
       confirmload();
       if (i != bounding_box(&active))
          fatal("Can't automatically convert this pattern with topology information -- Use XLife for the conversion\n");
       while (qgens-- > 0)
          generate(&active);
       convst = 0;
#ifdef MICROSOFT
/* MINGW can't use stdout :-( */
       {
       FILE *f = fopen(argv[argc - 1], "r");
       if (f || argc < 4)
errwin:    fatalw("Wrong name for the output file. Use syntax: lifeconv OPTIONS INFILE OUTFILE");
       f = fopen(argv[argc - 1], "w");
       if (!f) goto errwin;
       saveall(f, saveformat);
       fclose(f);
       }
#else
       saveall(stdout, saveformat);
#endif
       return convst;
    }
    xpos -= SCALE(width >> 1);
    ypos -= SCALE(height >> 1);
    XMapWindow(disp, inputw);
    XMapWindow(disp, helpw);
    XMapWindow(disp, lifew);
    XMapWindow(disp, mainw);
    XMapWindow(disp, rulew);
    XMapWindow(disp, statew);
    XMapWindow(disp, loadw);
    XMapWindow(disp, coordw);
    XMapWindow(disp, modew);//modew
    XLowerWindow(disp, helpw);
    XLowerWindow(disp, statew);
    XLowerWindow(disp, loadw);
    XLowerWindow(disp, modew);//modew
    XRaiseWindow(disp, inputw);
    XRaiseWindow(disp, rulew);
    XRaiseWindow(disp, coordw);
    showrules();
    displaystats();
    if (initpat)
        init_load(initpat);
#ifdef MICROSOFT
    griddisplay(1);
#endif
    initHashlife();//hashlife
    #ifdef MICROSOFT
       /*MEMORYSTATUSEX status;
       status.dwLength = sizeof(status);
       GlobalMemoryStatusEx(&status); 
       cout << status.ullTotalPhys << endl;
       numOfNewNodes = status.ullTotalPhys/(28*3);*/
       numOfNewNodes = 10000000;
    #else
       struct sysinfo info;
       sysinfo(&info);//info.freeram/(1024*1024)*info.mem_unit //Mb
       numOfNewNodes = info.totalram/(28*3)*info.mem_unit;//28 - sizeof(node), and 3 is for 1/3 of memory
    #endif
    for (;;) {
	eventHandler();
	showcoord(FALSE);
	if (state == HASHRUN) {
	    if (universeChanged) {
               toggle_hash(); //bad!
               toggle_hash();
               universeChanged = 0;
	    }
	    active.cellcount[1] = 0;
	    hashGenerate(&active, strBuf);
            redisplay(MESH);
	    displaystats();
	    continue;
	}
	if (state & RUN + HIDE) {
            generate(&active);
            if (hideperiod > 1) {
                for (i = 1; i < hideperiod; i++)
                   if (state != STOP)
                      generate(&active);
                if (state != HIDE)
                   redisplay(0);
            }
            else
	        redisplay(0);
            if (speed < 10000*hideperiod || ((active.generations/hideperiod)&0x3f) == 0)
                displaystats();
	}
	else
            make_delay(0, 100000);
	if (state == RUN)
            make_delay(timeout.tv_sec, timeout.tv_usec);
    }
}
/* main.c ends here */
