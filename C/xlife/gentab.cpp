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
A lot of modifications were added at 2011, 2012 by Vladimir Lidovski vol.litwr@gmail.com
(C) This version of XLife may be used under the same conditions as mentioned above
$Id: gentab.cpp 310 2014-02-11 07:40:09Z litwr $
*/

#include "defs.h"

void gentab(void) {
   register int i, t;
   for (i = 0; i < 0x40000; i++) {
      t = 1 << ((i & 0x4) != 0) + ((i & 1) != 0) + ((i & 2) != 0)
            + ((i & 0x40) != 0) + ((i & 0x100) != 0)
            + ((i & 0x1000) != 0) + ((i & 0x2000) != 0) + ((i & 0x4000) != 0);
      lookup[i] = i & 0x80 && live & t || ((i & 0x80) == 0) && born & t;
      t = 1 << ((i & 8) != 0) + ((i & 2) != 0) + ((i & 4) != 0)
            + ((i & 0x80) != 0) + ((i & 0x200) != 0)
            + ((i & 0x2000) != 0) + ((i & 0x4000) != 0) + ((i & 0x8000) != 0);
      lookup[i] |= (i & 0x100 && live & t || ((i & 0x100) == 0) && born & t) << 1;
      t = 1 << ((i & 8) != 0) + ((i & 0x10) != 0) + ((i & 4) != 0)
            + ((i & 0x100) != 0) + ((i & 0x400) != 0)
            + ((i & 0x10000) != 0) + ((i & 0x4000) != 0) + ((i & 0x8000) != 0);
      lookup[i] |= (i & 0x200 && live & t || ((i & 0x200) == 0) && born & t) << 2;
      t = 1 << ((i & 8) != 0) + ((i & 0x10) != 0) + ((i & 0x20) != 0)
            + ((i & 0x200) != 0) + ((i & 0x800) != 0)
            + ((i & 0x10000) != 0) + ((i & 0x20000) != 0) + ((i & 0x8000) != 0);
      lookup[i] |= (i & 0x400 && live & t || ((i & 0x400) == 0) && born & t) << 3;
   }
}

void gentab2(void) { /* generated, e.g., brian's brain */
    register int i, k, l;
    if (maxstates > 16)
      for (i = 0; i < 0x900; i++) {
       lookup[i] = 0;
       if ((i & 0xff) > 1)
           lookup[i] = ((i & 0xff) + 1)%maxstates;
       else if (i & 0xff)
           if (live & 1 << ((i & 0xf00) >> 8))
             lookup[i] |= 1;
           else
             lookup[i] |= 2;
       else
           if (born & 1 << ((i & 0xf00) >> 8))
             lookup[i] |= 1;
      }
    else if (maxstates > 4)
      for (i = 0; i < 0x8900; i++) {
       lookup2[i] = 0;
       if ((i & 0xf) > 1)
           lookup2[i] = ((i & 0xf) + 1)%maxstates;
       else if (i & 0xf)
           if (live & 1 << ((i & 0xf00) >> 8))
             lookup2[i] |= 1;
           else
             lookup2[i] |= 2;
       else
           if (born & 1 << ((i & 0xf00) >> 8))
             lookup2[i] |= 1;
       k  = i >> 4;
       if ((k & 0xf) > 1)
           lookup2[i] |= ((k & 0xf) + 1)%maxstates << 8;
       else if (k & 0xf)
           if (live & 1 << (k >> 8))
             lookup2[i] |= 0x100;
           else
             lookup2[i] |= 0x200;
       else if (born & 1 << (k >> 8))
             lookup2[i] |= 0x100;
      }
    else 
      for (i = 0; i < 0x888900; i++) {
         lookup4[i] = 0;
         for (l = 0; l < 4; l++) {
            k = i >> 2*l & 3;
            if (k > 1)
              lookup4[i] |= (k + 1)%maxstates << 8*l;
            else if (k)
              if (live & 1 << (i >> 4*l + 8 & 0xf))
                lookup4[i] |= 1 << 8*l;
              else
                lookup4[i] |= 2 << 8*l;
            else if (born & 1 << (i >> 4*l + 8 & 0xf))
              lookup4[i] |= 1 << 8*l;
         }
      }
}

void gentab3(void) { /* wireworld */
    register int i, k, l;
    for (i = 0; i < 0x888900; i++) {
       lookup4[i] = 0;
       for (l = 0; l < 4; l++) {
          k = i >> 2*l & 3;
          if (k == 3)
            if (born & 1 << (i >> 4*l + 8 & 0xf))
              lookup4[i] |= 1 << 8*l;
            else
              lookup4[i] |= 3 << 8*l;
          else if (k)
            lookup4[i] |= (k + 1)%maxstates << 8*l;
       }
    }
}

void gentab4(void) { /* 2-state with history -- add 5 states */
    register int i, k;
    for (i = 0; i < 0x8900; i++) {
       lookup2[i] = 0;
       k = 1 << ((i & 0xf00) >> 8);
       switch (i&0xf) {
       case 0:
       case 2:
          if (born & k) lookup2[i] |= 1;
            else lookup2[i] |= i&0xf;
          break;
       case 1:
          if (live & k) lookup2[i] |= 1;
            else lookup2[i] |= 2;
          break;
       case 3:
       case 5:
          if (live & k) lookup2[i] |= i&0xf;
             else lookup2[i] |= 4;
          break;
       case 4:
          if (born & k) lookup2[i] |= 3;
            else lookup2[i] |= 4;
          break;
       case 6:
          lookup2[i] |= 6;
       }
       k = 1 << ((i & 0xf000) >> 12);
       switch ((i&0xf0) >> 4) {
       case 0:
       case 2:
          if (born & k) lookup2[i] |= 0x100;
            else lookup2[i] |= (i&0xf0) << 4;
          break;
       case 1:
          if (live & k) lookup2[i] |= 0x100;
            else lookup2[i] |= 0x200;
          break;
       case 3:
       case 5:
          if (live & k) lookup2[i] |= (i&0xf0) << 4;
            else lookup2[i] |= 0x400;
          break;
       case 4:
          if (born & k) lookup2[i] |= 0x300;
            else lookup2[i] |= 0x400;
          break;
       case 6:
          lookup2[i] |= 0x600;
       }
    }
}

void genatab4(void) { //wireworld, generated
   int i = 0, k;

   if (maxstates <= 4)
     for (; i < 65536; i++) {
        k = 0;
        if ((i & 0x3) == 1) k = 1;
        if ((i >> 2 & 0x3) == 1) k |= 16;
        if ((i >> 4 & 0x3) == 1) k |= 256;
        if ((i >> 6 & 0x3) == 1) k |= 4096;
        if ((i >> 8 & 0x3) == 1) k |= 2;
        if ((i >> 10 & 0x3) == 1) k |= 32;
        if ((i >> 12 & 0x3) == 1) k |= 512;
        if ((i >> 14 & 0x3) == 1) k |= 8192;
        tab6[i] = k;
     }
   else if (maxstates <= 16)
     for (; i < 65536; i++) {
        k = 0;
        if ((i & 0xf) == 1) k = 1;
        if ((i >> 4 & 0xf) == 1) k |= 16;
        if ((i >> 8 & 0xf) == 1) k |= 2;
        if ((i >> 12 & 0xf) == 1) k |= 32;
        tab4[i] = k;
     }
   else
     for (; i < 65536; i++) {
        k = 0;
        if ((i & 0xff) == 1) k = 1;
        if ((i >> 8 & 0xff) == 1) k |= 2;
        tab4[i] = k;
     }
}

void genatab5(void) { /*history*/
   int i = 0, k;
   for (; i < 65536; i++) {
      k = 0;
      if ((i & 0xf)%2 == 1) k = 1;
      if ((i >> 4 & 0xf)%2 == 1) k |= 16;
      if ((i >> 8 & 0xf)%2 == 1) k |= 2;
      if ((i >> 12 & 0xf)%2 == 1) k |= 32;
      tab4[i] = k;
   }
}

