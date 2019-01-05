/*
 * XLife Copyright 2013 Yaroslav Zotov zotovyaa@mail.ru
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
$Id$
*/

#undef __STRICT_ANSI__ //for i586-mingw32msvc-g++

#ifndef HASHLIFE_CPP_H

#define HASHLIFE_CPP_H

#define ERROR(s)\
	state = STOP,\
	strcpy(inpbuf, s),\
	insideHashAlgorithm = 0,\
	announce_and_wait(MAXSTATES + 3)//RED_EVER 

#define INCSTEP\
	stepPower++,\
	currentStep *= 2,\
	incStep = 1
	
#define CATCH_EXCEPTION(s)\
	char str[2], errorCode[15];\
	sprintf(str, "%d", i);\
	strcpy(errorCode, "Error code ");\
	strcat (errorCode, str);\
	if (i == 1)\
		ERROR(s);\
	else\
		ERROR(errorCode);
		
#define TOVECTOR(X, Y, N, XN, YN)\
	if (x < XN && y < YN)\
	{\
		n = node::hashVector[n].nw;\
		tempList[tempListCounter++] = NW;\
		continue;\
	}\
	if (x >= XN && y < YN)\
	{\
		n = node::hashVector[n].ne;\
		tempList[tempListCounter++] = NE;\
		X = XN;\
		continue;\
	}\
	if (x < XN && y >= YN)\
	{\
		n = node::hashVector[n].sw;\
		tempList[tempListCounter++] = SW;\
		Y = YN;\
		continue;\
	}\
	if (x >= XN && y >= YN)\
	{\
		n = node::hashVector[n].se;\
		tempList[tempListCounter++] = SE;\
		X = XN;\
		Y = YN;\
		continue;\
	}
	
#define BIGCODE(X, Y)\
	int nodeState = 0;\
	if (ev_mode == VALENCE_DRIVEN)/*twostate*/\
	{\
		if (x == X) /*here we determine location of 2x2 node with changeable cell (nw, sw, ne or se)*/\
			mask = (y == Y) ? NW : SW;\
		else\
			mask = (y == Y) ? NE : SE;\
		if (n < 8)\
			nodeState = (live1 >> pos[n]) & maskNW;\
		else \
			nodeState = (live2 >> pos[n - 8]) & maskNW;/*now nodeState is bit state of our current 2x2 node*/\
		result = (nodeState & mask) == 0 ? 0 : 1;\
		if(get)\
			return result;/*return cell color*/\
		if (newstate != 0)\
			nodeState |= mask;\
		else \
			nodeState &= ~mask;/*now it is changing	according to newState*/\
		for (int i = 0; i < 8; i++)\
		{\
			if (nodeState == (live1 >> pos[i] & maskNW))\
			{\
				nodeState = i;\
				break;\
			}\
			if (nodeState == (live2 >> pos[i] & maskNW))\
			{\
				nodeState = i + 8;\
				break;\
			}\
		}/*and now nodeState is this result 2x2 node's position in hashVector*/\
		if (nodeState == n)\
			return 0;/*nothing changed*/\
	}\
	else/*N-state*/\
	{\
		if(get)\
			return n;/*return cell color*/\
		result = n;\
		if (n == newstate)\
			return 0;/*nothing changed*/\
		nodeState = newstate;\
	}\
	if (newstate && (ev_mode != VALENCE_DRIVEN && n == 0 || ev_mode == VALENCE_DRIVEN))\
		population++;\
	else if (newstate == 0)\
		population--;\
	depth = minDepth;\
	while (nodeListCounter != 0)\
	{/*this is recursive like ascent from small changed node to root*/\
		depth++;\
		int tmp = tempList[--tempListCounter];\
		n = nodeList[--nodeListCounter];\
		if (tmp == NW)\
		{\
			nodeState = unite2(node::hashVector[n].sw, node::hashVector[n].se, nodeState, node::hashVector[n].ne, depth, 1);\
			continue;\
		}\
		if (tmp == NE)\
		{\
			nodeState = unite2(node::hashVector[n].sw, node::hashVector[n].se, node::hashVector[n].nw, nodeState, depth, 1);\
			continue;\
		}\
		if (tmp == SW)\
		{\
			nodeState = unite2(nodeState, node::hashVector[n].se, node::hashVector[n].nw, node::hashVector[n].ne, depth, 1);\
			continue;\
		}\
		if (tmp == SE)\
		{\
			nodeState = unite2(node::hashVector[n].sw, nodeState, node::hashVector[n].nw, node::hashVector[n].ne, depth, 1);\
			continue;\
		}\
	}\
	root = nodeState;/*final*/
	
#include "bignumber.h"
#include <vector>
#include <iostream>
#include <fstream>
#include <cmath>
using namespace std;

#include "defs.h"
#include "common.h"
#include "tile.h"
#include "framebuffer.h"

typedef unsigned long long uLong;
typedef unsigned int uInt;

class node
{
	static node *hashVector;
	static int *hashTable;
	static int maxSize, curSize, prevMaxSize, curPop;
	static vector<bigNumber> populationVector;
	int nw, ne, sw, se, result;
	int hashNext;//for dictionary
	int popPointer;//
public:
	node(int, int, int, int);
	node();
	inline int hashF();
	friend inline int hashF(int, int, int, int);
	friend inline int unite2(int, int, int, int, int, int);
	friend inline int findInDictionary(int, int, int, int);	
	int addToDictionary(int, int);
	friend int expand(int, int);
	friend int expandDouble(int, int);
	friend void setZero(int);//it becomes zero node
	friend inline void findResult(node*, u32bits, u32bits, u32bits, u32bits, int*);
	friend void patternToNode(pattern*, int*, coord_t, coord_t, int);
	friend void tileToNode(tile*, int*, coord_t, coord_t, int);
        friend void nodeToPattern(int, pattern*, bigNumber, const bigNumber&, const bigNumber&, int);
	friend void nodeToTile(int, tile*, coord_t, coord_t, coord_t, int);
	friend void nodeToVideo(int, bigNumber, const bigNumber&, const bigNumber&, int*);
	friend void nodeToVideoShort(int, coord_t, coord_t, coord_t, int*);
	friend void nodeToVideoTile(int, const bigNumber&, const bigNumber&, bigNumber, const bigNumber&, const bigNumber&);
	friend void nodeToVideoTileShort(int, coord_t, coord_t, coord_t, coord_t, coord_t);	
	friend void initHashTable(int);
	friend void rehash();
	friend void clearHash();
	friend void getPopulation(int, bigNumber&, int);
	friend void calculatePopulation(int, int&);
	friend void garbageCollector(int);
	friend int markNodes(int, int&, int, vector<bigNumber>&, node*, int);
	friend void rebuildHashTable();
	friend bool needGC();
	friend int getResult(int, int);
	friend int recursiveUnite(int, int);
	friend int changeCellNode (coord_t, coord_t, cell_t, int);
	friend void extraNodes(int&, int&, int&, int&, int&, int&, int&, int&);
	friend void calculateSlashInfo(int);
	friend void boundingBox(int, const bigNumber&, const bigNumber&, const bigNumber&, bigNumber&, bigNumber&, bigNumber&, bigNumber&);
	friend int changeCellNode2(coord_t, coord_t, cell_t, int, int, int, int, int, int);
	friend void colorFromNode(int, bigNumber, unsigned&);
	friend void saveHashFile(FILE*);
	friend int saveNodes(int, int&, FILE*);
	friend int loadHashFile(FILE*, char*);
	friend int changeCellNodeBig(const bigNumber&, const bigNumber&, char, cell_t, int);
	friend void make_tentative_hash(const bigNumber&, const bigNumber&, const bigNumber&, const bigNumber&);
	friend void tentativeNodeToVideoTile(int, const bigNumber&, const bigNumber&, bigNumber, const bigNumber&, const bigNumber&);
	friend void tentativeNodeToVideo(int, bigNumber, const bigNumber&, const bigNumber&, int*);
	friend int addTentativeNodes(node*, int, int);
};

inline void setCell(u32bits&, u32bits, int, u32bits, u32bits, int);
void generateOneCell(u32bits&, u32bits, u32bits, u32bits, u32bits);
inline void setOneBit(u32bits&, u32bits, u32bits, u32bits);
inline void find2x2node(int&, u32bits, u32bits, u32bits, u32bits, int*);
inline void resizeRootCoordinates(coord_t&, coord_t&, coord_t&, coord_t&, coord_t&, coord_t&);
int log2 (coord_t);
template<class T>
inline bool nodeOnScreen(const T&, const T&, const T&);
int nextprime(int);
inline u32bits reverse(u32bits);
inline cell_t returnOneCell (cell_t(&)[4][4], coord_t, coord_t);
int getCell(int, int);
int log2(bigNumber);

#endif

