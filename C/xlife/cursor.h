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
 * Several modifications were added at 2012, 2013 
 * by Vladimir Lidovski vol.litwr@gmail.com
 * (C) This version of XLife may be used under the same conditions as mentioned above
 * $Id$
 */

#define cursor_data_width 16
#define cursor_data_height 16
#define cursor_data_x_hot 7
#define cursor_data_y_hot 7
static unsigned char cursor_data_bits[] = {
   0x60, 0x03, 0x60, 0x03, 0x60, 0x03, 0x60, 0x03, 0x60, 0x03, 0x7f, 0x7f,
   0x7f, 0x7f, 0x00, 0x00, 0x7f, 0x7f, 0x7f, 0x7f, 0x60, 0x03, 0x60, 0x03,
   0x60, 0x03, 0x60, 0x03, 0x60, 0x03, 0x00, 0x00};
#define cursor_mask_width 16
#define cursor_mask_height 16
static unsigned char cursor_mask_bits[] = {
   0xe0, 0x03, 0xe0, 0x03, 0xe0, 0x03, 0xe0, 0x03, 0xe0, 0x03, 0xff, 0x7f,
   0xff, 0x7f, 0xff, 0x7f, 0xff, 0x7f, 0xff, 0x7f, 0xe0, 0x03, 0xe0, 0x03,
   0xe0, 0x03, 0xe0, 0x03, 0xe0, 0x03, 0x00, 0x00};
