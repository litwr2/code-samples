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
$Id: file.h 311 2014-02-11 09:38:53Z litwr $
*/

extern char rfullpath[];
extern LoadReq *loadscript;

void add_loadreq(LoadReq**, unsigned long, char*, int, coord_t, coord_t, int, int, int, int);
char *add_opt_ext(char*, const char*);
char *checktilda(const char*);
void confirmload(void);
int do_loadreq(LoadReq*, pattern*);
void fileinit(void);
int find_file(char*, char**);
void fix_microsoft(char*);
void free_loadscript(void);
void genload(void);
void loadcolors(char*);
int loadfile_req(char*);
void loadrules(char*);
char* loadscriptfn(int);
int loadscriptstat(void);
void makefullpath(char*, char*, char**);
void parse_patname(char*, char*);
int savefile(char*);
int saveloadscript(char*);
char *seppatdir(char*, char*);
char* use_colors(char*, char**);
char* use_rules(char*, char**);

#define F_L             0
#define F_LIF           1
#define F_MCL           2
#define F_RLE           3
#define F_CELLS         4
#define F_ERR           100
