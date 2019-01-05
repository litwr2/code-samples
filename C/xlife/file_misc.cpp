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
 * A lot of modifications were added at 2001, 2011, 2012 by Vladimir Lidovski vol.litwr@gmail.com
 * (C) This version of XLife may be used under the same conditions as mentioned above
 * $Id: file_misc.cpp 310 2014-02-11 07:40:09Z litwr $
 */

#include "defs.h"
#include "tile.h"

void name_file(void) {
    if (fname[0])
	sprintf(inpbuf, "Old Name: %s, New Name: ", fname);
    else
	strcpy(inpbuf, "New Name: ");
    minbuflen = strlen(inpbuf);
    DoExpose(inputw);
    getxstring();
    strcpy(fname, inpbuf + minbuflen);
    displaystats();
}

void comment(void) {
    minbuflen = 9;
    for (;;) {
	announce("Comment: ");
	getxstring();
	strcpy(comments[numcomments], inpbuf + minbuflen);
	if (comments[numcomments][0] == 0 || ++numcomments > MAXCOMMENTS - 2) {
	    displaystats();
	    return;
	}
    }
}
