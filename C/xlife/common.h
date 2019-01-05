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
 * A lot of modifications were added at 2011, 2012 by Vladimir Lidovski vol.litwr@gmail.com
 * (C) This version of XLife may be used under the same conditions as mentioned above
 * $Id: common.h 364 2018-03-31 08:57:04Z litwr $
 */

/* someday, on a 64-bit machine, this might be unsigned int rather than long */
/* someday was years ago, catch up */
typedef unsigned int    u32bits;
typedef unsigned short  u16bits;
typedef unsigned char   u8bits;

/*
 * These typedefs might change on 64-bit machines like the DEC Alpha to
 * support *really large* universes.  Ideally, cellcount_t should be
 * twice the length of coord_t, since it might hold coord_t's max squared.
 * It's hardly likely it will ever go that high, though...
 */
typedef u32bits coord_t;        /* hold world coordinates */
typedef u32bits cellcount_t;    /* hold a cell count */

#define MAX_THREADS 16

#if !defined(HASHARRAY) && !defined(BTREE) && !defined(UNORDEREDMAP)
//#define HASHARRAY
#define BTREE
//#define UNORDEREDMAP
#endif
