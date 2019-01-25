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
 * Several modifications were added at 2011, 2012 by Vladimir Lidovski vol.litwr@gmail.com
 * (C) This version of XLife may be used under the same conditions as mentioned above
 * $Id: tab.h 216 2013-07-28 18:06:27Z micro $
 */

int tab1[] = {
0x00000000, 0x00000011, 0x00000111, 0x00000122,
0x00001110, 0x00001121, 0x00001221, 0x00001232,
0x00011100, 0x00011111, 0x00011211, 0x00011222,
0x00012210, 0x00012221, 0x00012321, 0x00012332,
0x00111000, 0x00111011, 0x00111111, 0x00111122,
0x00112110, 0x00112121, 0x00112221, 0x00112232,
0x00122100, 0x00122111, 0x00122211, 0x00122222,
0x00123210, 0x00123221, 0x00123321, 0x00123332,
0x01110000, 0x01110011, 0x01110111, 0x01110122,
0x01111110, 0x01111121, 0x01111221, 0x01111232,
0x01121100, 0x01121111, 0x01121211, 0x01121222,
0x01122210, 0x01122221, 0x01122321, 0x01122332,
0x01221000, 0x01221011, 0x01221111, 0x01221122,
0x01222110, 0x01222121, 0x01222221, 0x01222232,
0x01232100, 0x01232111, 0x01232211, 0x01232222,
0x01233210, 0x01233221, 0x01233321, 0x01233332,
0x11100000, 0x11100011, 0x11100111, 0x11100122,
0x11101110, 0x11101121, 0x11101221, 0x11101232,
0x11111100, 0x11111111, 0x11111211, 0x11111222,
0x11112210, 0x11112221, 0x11112321, 0x11112332,
0x11211000, 0x11211011, 0x11211111, 0x11211122,
0x11212110, 0x11212121, 0x11212221, 0x11212232,
0x11222100, 0x11222111, 0x11222211, 0x11222222,
0x11223210, 0x11223221, 0x11223321, 0x11223332,
0x12210000, 0x12210011, 0x12210111, 0x12210122,
0x12211110, 0x12211121, 0x12211221, 0x12211232,
0x12221100, 0x12221111, 0x12221211, 0x12221222,
0x12222210, 0x12222221, 0x12222321, 0x12222332,
0x12321000, 0x12321011, 0x12321111, 0x12321122,
0x12322110, 0x12322121, 0x12322221, 0x12322232,
0x12332100, 0x12332111, 0x12332211, 0x12332222,
0x12333210, 0x12333221, 0x12333321, 0x12333332,
0x11000000, 0x11000011, 0x11000111, 0x11000122,
0x11001110, 0x11001121, 0x11001221, 0x11001232,
0x11011100, 0x11011111, 0x11011211, 0x11011222,
0x11012210, 0x11012221, 0x11012321, 0x11012332,
0x11111000, 0x11111011, 0x11111111, 0x11111122,
0x11112110, 0x11112121, 0x11112221, 0x11112232,
0x11122100, 0x11122111, 0x11122211, 0x11122222,
0x11123210, 0x11123221, 0x11123321, 0x11123332,
0x12110000, 0x12110011, 0x12110111, 0x12110122,
0x12111110, 0x12111121, 0x12111221, 0x12111232,
0x12121100, 0x12121111, 0x12121211, 0x12121222,
0x12122210, 0x12122221, 0x12122321, 0x12122332,
0x12221000, 0x12221011, 0x12221111, 0x12221122,
0x12222110, 0x12222121, 0x12222221, 0x12222232,
0x12232100, 0x12232111, 0x12232211, 0x12232222,
0x12233210, 0x12233221, 0x12233321, 0x12233332,
0x22100000, 0x22100011, 0x22100111, 0x22100122,
0x22101110, 0x22101121, 0x22101221, 0x22101232,
0x22111100, 0x22111111, 0x22111211, 0x22111222,
0x22112210, 0x22112221, 0x22112321, 0x22112332,
0x22211000, 0x22211011, 0x22211111, 0x22211122,
0x22212110, 0x22212121, 0x22212221, 0x22212232,
0x22222100, 0x22222111, 0x22222211, 0x22222222,
0x22223210, 0x22223221, 0x22223321, 0x22223332,
0x23210000, 0x23210011, 0x23210111, 0x23210122,
0x23211110, 0x23211121, 0x23211221, 0x23211232,
0x23221100, 0x23221111, 0x23221211, 0x23221222,
0x23222210, 0x23222221, 0x23222321, 0x23222332,
0x23321000, 0x23321011, 0x23321111, 0x23321122,
0x23322110, 0x23322121, 0x23322221, 0x23322232,
0x23332100, 0x23332111, 0x23332211, 0x23332222,
0x23333210, 0x23333221, 0x23333321, 0x23333332};

int tab2[] = {
0x00000000, 0x00000010, 0x00000101, 0x00000111,
0x00001010, 0x00001020, 0x00001111, 0x00001121,
0x00010100, 0x00010110, 0x00010201, 0x00010211,
0x00011110, 0x00011120, 0x00011211, 0x00011221,
0x00101000, 0x00101010, 0x00101101, 0x00101111,
0x00102010, 0x00102020, 0x00102111, 0x00102121,
0x00111100, 0x00111110, 0x00111201, 0x00111211,
0x00112110, 0x00112120, 0x00112211, 0x00112221,
0x01010000, 0x01010010, 0x01010101, 0x01010111,
0x01011010, 0x01011020, 0x01011111, 0x01011121,
0x01020100, 0x01020110, 0x01020201, 0x01020211,
0x01021110, 0x01021120, 0x01021211, 0x01021221,
0x01111000, 0x01111010, 0x01111101, 0x01111111,
0x01112010, 0x01112020, 0x01112111, 0x01112121,
0x01121100, 0x01121110, 0x01121201, 0x01121211,
0x01122110, 0x01122120, 0x01122211, 0x01122221,
0x10100000, 0x10100010, 0x10100101, 0x10100111,
0x10101010, 0x10101020, 0x10101111, 0x10101121,
0x10110100, 0x10110110, 0x10110201, 0x10110211,
0x10111110, 0x10111120, 0x10111211, 0x10111221,
0x10201000, 0x10201010, 0x10201101, 0x10201111,
0x10202010, 0x10202020, 0x10202111, 0x10202121,
0x10211100, 0x10211110, 0x10211201, 0x10211211,
0x10212110, 0x10212120, 0x10212211, 0x10212221,
0x11110000, 0x11110010, 0x11110101, 0x11110111,
0x11111010, 0x11111020, 0x11111111, 0x11111121,
0x11120100, 0x11120110, 0x11120201, 0x11120211,
0x11121110, 0x11121120, 0x11121211, 0x11121221,
0x11211000, 0x11211010, 0x11211101, 0x11211111,
0x11212010, 0x11212020, 0x11212111, 0x11212121,
0x11221100, 0x11221110, 0x11221201, 0x11221211,
0x11222110, 0x11222120, 0x11222211, 0x11222221,
0x01000000, 0x01000010, 0x01000101, 0x01000111,
0x01001010, 0x01001020, 0x01001111, 0x01001121,
0x01010100, 0x01010110, 0x01010201, 0x01010211,
0x01011110, 0x01011120, 0x01011211, 0x01011221,
0x01101000, 0x01101010, 0x01101101, 0x01101111,
0x01102010, 0x01102020, 0x01102111, 0x01102121,
0x01111100, 0x01111110, 0x01111201, 0x01111211,
0x01112110, 0x01112120, 0x01112211, 0x01112221,
0x02010000, 0x02010010, 0x02010101, 0x02010111,
0x02011010, 0x02011020, 0x02011111, 0x02011121,
0x02020100, 0x02020110, 0x02020201, 0x02020211,
0x02021110, 0x02021120, 0x02021211, 0x02021221,
0x02111000, 0x02111010, 0x02111101, 0x02111111,
0x02112010, 0x02112020, 0x02112111, 0x02112121,
0x02121100, 0x02121110, 0x02121201, 0x02121211,
0x02122110, 0x02122120, 0x02122211, 0x02122221,
0x11100000, 0x11100010, 0x11100101, 0x11100111,
0x11101010, 0x11101020, 0x11101111, 0x11101121,
0x11110100, 0x11110110, 0x11110201, 0x11110211,
0x11111110, 0x11111120, 0x11111211, 0x11111221,
0x11201000, 0x11201010, 0x11201101, 0x11201111,
0x11202010, 0x11202020, 0x11202111, 0x11202121,
0x11211100, 0x11211110, 0x11211201, 0x11211211,
0x11212110, 0x11212120, 0x11212211, 0x11212221,
0x12110000, 0x12110010, 0x12110101, 0x12110111,
0x12111010, 0x12111020, 0x12111111, 0x12111121,
0x12120100, 0x12120110, 0x12120201, 0x12120211,
0x12121110, 0x12121120, 0x12121211, 0x12121221,
0x12211000, 0x12211010, 0x12211101, 0x12211111,
0x12212010, 0x12212020, 0x12212111, 0x12212121,
0x12221100, 0x12221110, 0x12221201, 0x12221211,
0x12222110, 0x12222120, 0x12222211, 0x12222221};

unsigned char tab3[] = {
 0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4,
 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
 4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8};