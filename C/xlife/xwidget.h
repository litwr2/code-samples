/*
 * XLife Copyright 2011, 2012 Vladimir Lidovski vol.litwr@gmail.com
 * $Id: xwidget.h 311 2014-02-11 09:38:53Z litwr $
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

#define LOADW_WIDTH 282
#define LOADW_HEIGHT 457

#define WIDST_LOAD_PATTERN 0
#define WIDST_LOAD_PALETTE 1
#define WIDST_LOAD_RULE 2
#define WIDST_SAVE 3
#define WIDST_LOAD_FILE_PART 4
#define WIDST_SAVE_SCRIPT 5

void drawloadwidget(void), widget_main(int), setcurdir(const char*);
