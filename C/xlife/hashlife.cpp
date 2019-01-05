/*
 * XLife Copyright 2013,14 Yaroslav Zotov zotovyaa@mail.ru
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
 * TOneIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

/*
$Id: hashlife.cpp 351 2014-06-02 15:21:12Z qiray $
*/

#include "hashlife.h"

coord_t initXRight, initXLeft, initYBottom, initYTop;
void nodeToPattern(int, pattern*, bigNumber, const bigNumber&, const bigNumber&, int);
void nodeToVideo(int, bigNumber, const bigNumber&, const bigNumber&, int*);
int changeCellNode2(coord_t, coord_t, cell_t, int, int, int, int, int, int);
int getResult(int, int);
void getPopulation(int, bigNumber&, int);
int expand(int, int);
int eventHandlerCounter = 0, expandCounter = 1, prevRoot = 0, incStep = 1;
bigNumber cellsCount[256] = {0};
unsigned hashRatio = 1;
unsigned long numOfNewNodes = 5000000;

vector<bigNumber> node::populationVector; 
int *node::hashTable;
node *node::hashVector;
int node::maxSize, node::curSize, node::prevMaxSize, node::curPop;
bigNumber rootX, rootY, rootN, maxCoord = 4294967295lu, population(0), generations(0), hashSpeed(0), currentStep(1);
int rootDepth = 0, minSaveDepth = 10, depthStep = 2, stepPower = 0, prevMode = ev_mode;
const u32bits live1 = -1428684584, live2 = -2599464;//these 2 numbers show state of cells in 16 2x2 nodes (8 nodes in the first number and 8 in the second one)
u32bits hashLive = live, hashBorn = born;
bigNumber tentativeX(0), tentativeY(0), tentativeN(8), rightX(0), bottomY(0), tentativePopulation(0), tentativeGenerations(0);
int tentativeRoot = 0, tentativeDepth = 3;
void tentativeNodeToVideo(int, bigNumber, const bigNumber&, const bigNumber&, int*);
//16 nodes 2x2 (all states):
/*
| 0 0 | | 0 1 | | 1 0 | | 1 1 | | 0 0 | | 0 1 | | 1 0 | | 1 1 |
| 0 0 | | 0 0 | | 0 0 | | 0 0 | | 0 1 | | 0 1 | | 0 1 | | 0 1 |

| 0 0 | | 0 1 | | 1 0 | | 1 1 | | 0 0 | | 0 1 | | 1 0 | | 1 1 |
| 1 0 | | 1 0 | | 1 0 | | 1 0 | | 1 1 | | 1 1 | | 1 1 | | 1 1 |
*/

int zero = 0;//zero is magic node all of which parts, result and this node itself are the same:
		//obviously this node is empty and all cells in this node are "dead"	
int root = 0;//main node of the universe	

u32bits maskNW = 771, maskNE = 3084, maskSW = 50528256, maskSE = 202113024;
	/* maskNW:	//other masks are similar
	1 1 0 0 0 0 0 0
	1 1 0 0 0 0 0 0
	0 0 0 0 0 0 0 0
	0 0 0 0 0 0 0 0
	*/
int pos[8] = {0, 2, 4, 6, 16, 18, 20, 22}, posSW = 16, posSE = 18, posNW = 0, posNE = 2;
u32bits mask0 = 460039, mask1 = 920078, mask2 = 117769984, mask3 = 235539968;
	/* mask0:	//other masks are similar
	1 1 1 0 0 0 0 0
	1 0 1 0 0 0 0 0
	1 1 1 0 0 0 0 0
	0 0 0 0 0 0 0 0
	*/	

inline void findResult(node *temp, u32bits result, u32bits live1, u32bits live2, u32bits maskNW, int *pos)
{//find 2x2 result node for temp node and set temp->result
	result &= maskNW;
	for (int i = 0; i < 8; i++)
		if (result == (maskNW & (live1 >> pos[i])))
		{
			temp->result = i;
			return;
		}	
	for (int i = 8; i < 16; i++)
		if (result == (maskNW & (live2 >> (pos[i - 8]))))
		{
			temp->result = i;
			return;
		}
}

inline void find2x2node (int &temp, u32bits result, u32bits live1, u32bits live2, u32bits maskNW, int *pos)
{//find 2x2 result according to u32bits result parameter and make int &temp equal to this result
	result &= maskNW;
	for (int i = 0; i < 8; i++)
		if (result == (maskNW & (live1 >> pos[i])))
		{
			temp = i;
			return;
		}	
	for (int i = 8; i < 16; i++)
		if (result == (maskNW & (live2 >> (pos[i - 8]))))
		{
			temp = i;
			return;
		}		
}

inline void setCell(u32bits &result, u32bits mask, int delta, u32bits live1, u32bits live2, int i)
{//move 2x2 node to chosen position (nw, ne, sw or se) in 4x4 node
	if (i < 8)
	{
		if (delta < 0)
			result |= mask & (live1 << (-delta));
		else
			result |= mask & (live1 >> delta);
	}
	else
	{
		if (delta < 0)
			result |= mask & (live2 << (-delta));
		else
			result |= mask & (live2 >> delta);	
	}
}

inline void setOneBit(u32bits &result, u32bits mask, u32bits temp, u32bits bit)
{//used to turn 1 bit (defined by mask) ON or OFF according to number of "neighbour" bits and cellular automata rules
	u32bits i;
	for (i = 0; mask; i++)
		mask ^= mask & -mask;//i contains number of "ON" bits in mask
	if ((result & bit) != 0)
		for (int j = 0; j < 10; j++)
			if (i == j && (hashLive & (1 << j)) != 0)
			{
				result |= bit;	
				return;//cell remains alive
			}
	if ((result & bit) == 0)
		for (int j = 0; j < 10; j++)
			if (i == j && (hashBorn & (1 << j)) != 0)
			{
				result |= bit;	
				return;//new cell is born
			}
	result &= ~bit;//cell dies
}

void generateOneCell(u32bits &result, u32bits mask0, u32bits mask1, u32bits mask2, u32bits mask3)  
{ //this function makes one generation of 2x2 'result' square in 4x4 square
	u32bits temp = result, i = 0;
	mask0 &= temp;
	mask1 &= temp;	
	mask2 &= temp;
	mask3 &= temp;
	setOneBit(result, mask0, temp, 512);
	setOneBit(result, mask1, temp, 1024);
	setOneBit(result, mask2, temp, 131072);
	setOneBit(result, mask3, temp, 262144);
	result >>= 9; //move center result (512, 1024, 131072, 262144 bits) to the North-West corner
}

void initHashTable(int newSize)
{//init hash table: clear hashVector and hashTable
	node::prevMaxSize = 0;
	node::hashTable = new int [newSize];
	node::maxSize = newSize;
	memset(node::hashTable, 0, node::maxSize*sizeof(int));
	node::hashVector = new node [newSize];
	node::curSize = node::curPop = 0;
	memset(node::hashVector, 0, node::maxSize*sizeof(node));
	node temp(-2, -2, -2, -2);//this number was chosen randomly
	int length = ev_mode == VALENCE_DRIVEN? 16 : 256;//number of special never changable nodes (2x2 or 1x1)
	for(int i = 0; i < length; i++)//Very important! Add first 16 (or 256) nodes to hashVector (2x2 or 1x1 nodes)
	{
		node::hashVector[i] = temp;
		node::curSize++;
	}
}

void setZero(int z)
{//init "magic" zero node
	node::hashVector[z].nw = node::hashVector[z].ne = node::hashVector[z].sw = node::hashVector[z].se = node::hashVector[z].result = z;
}

inline u32bits reverse(u32bits b)
{//return bitwise reversed u32bits b
	u32bits tmp = b;
	b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
	b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
	b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
	b <<= 1;
	b |= tmp >> 8;	
	return b;
}

void initHashlife()
{//init hashlife: hashTable, rules, nodes and tentativeRoot
	u32bits shift = 0, result = 0;
	initHashTable(nextprime(131072));
	if ((born & 1) != 0) //for B0 rules
	{//init rules
		hashLive = reverse(511 - born);
		hashBorn = reverse(511 - live);
	}
	else
	{//init rules
		hashLive = live;
		hashBorn = born;
	}
	if (ev_mode == VALENCE_DRIVEN)//init all possible 4x4 nodes and their results
		for(int i = 0; i < 16; i++) //sw
			for(int j = 0; j < 16; j++) //se
				for(int k = 0; k < 16; k++) //nw
					for(int l = 0; l < 16; l++) //ne
					{
						result = 0;
						node temp(i, j, k ,l);
						setCell(result, maskSW, pos[i & 7] - posSW, live1, live2, i);//set sw cell
						setCell(result, maskSE, pos[j & 7] - posSE, live1, live2, j);//set se cell
						setCell(result, maskNW, pos[k & 7] - posNW, live1, live2, k);//set nw cell
						setCell(result, maskNE, pos[l & 7] - posNE, live1, live2, l);//set ne cell
						generateOneCell(result, mask0, mask1, mask2, mask3);//make 1 generation to find 1 step result of 4x4 node
						findResult (&temp, result, live1, live2, maskNW, pos);//used to find result 2x2 node of 4x4 node
						temp.addToDictionary(2, 0);//add new created node into dictionary
					}
	setZero(0);
	currentStep = 1;
	stepPower = 0;
	tentativeX = tentativeY = 0;//init tentative root and it's coordinates
	rightX = bottomY = tentativeN = 8;
	tentativeRoot = 0;
	tentativeDepth = 3;
	if (shortText)//init displayed text
		strcpy(strBuf, "G0 C0 S0");	
	else
		strcpy(strBuf, "Generations: 0, Cells: 0, Step: 0");
}

int log2 (coord_t n)
{//return int binary logarithm of coord_t n
	int i = 0;
	while (n != 1)
	{
		i++;
		n >>= 1;
	}
	return i;
}

int log2(bigNumber n)
{//return int binary logarithm of bigNumber n
	int i = 0;
	while (n >= 2) 
	{
		i++;
		n /= 2;
	}
	return i;
}

inline int node::hashF()
{//hash function for hashTable
	return (unsigned)(65537*nw + 257*ne + 17*sw + 5*se)%maxSize;
}

inline int hashF(int sw, int se, int nw, int ne)
{//hash function for hashTable
	return (unsigned)(65537*nw + 257*ne + 17*sw + 5*se)%node::maxSize;
}

inline int findInDictionary(int n0, int n1, int n2, int n3)
{//find node (sw = n0, se = n1, nw = n2, ne = n3) in dictionary and return it's position, if search fails return error code (-3)
	if (n0 == zero && n1 == zero && n2 == zero && n3 == zero)
		return zero;
	int hash = hashF(n0, n1, n2, n3), i = node::hashTable[hash], prev = i;
	if (i == 0)
		return -3;//search failed: this node hasn't been hashed yet
	while (i != -1)//node with the same hash function result exists in dictionary
	{
		if (node::hashVector[i].sw == n0 && node::hashVector[i].se == n1 && node::hashVector[i].nw == n2 && node::hashVector[i].ne == n3)
		{//node found
			if (prev != i)//move found node to "top" to increase search speed
			{
				node::hashVector[prev].hashNext = node::hashVector[i].hashNext;
				node::hashVector[i].hashNext = node::hashTable[hash];
				node::hashTable[hash] = i;
			}
			return i;
		}	
		prev = i;	
		i = node::hashVector[i].hashNext;//go to next node with the same hash function result 
	}
	return -3;//search failed: this node hasn't been hashed yet
}

inline int unite2(int a0, int a1, int a2, int a3, int depth, int pop = 0)
{//if node already exists in dictionary then return it's position else add node to dictionaryand then return position
	int res = findInDictionary(a0, a1, a2, a3);
	if (res != -3)
		return res;//node found
	node tmp(a0, a1, a2, a3);
	tmp.result = -3;	
	return tmp.addToDictionary(depth, pop);
}

void tileToNode(tile *t, int *n, coord_t x, coord_t y, int N)
{//create nodes from tiles using "recursive descent" from 4x4 nodes to 2x2 nodes
	switch (N)
	{
	case 4:
		N = 2;
		int nw, ne, sw, se;
		tileToNode(t, &nw, x, y, N);
		tileToNode(t, &ne, x + N, y, N);
		tileToNode(t, &sw, x, y + N, N);
		tileToNode(t, &se, x + N, y + N, N);
		*n = unite2(sw, se, nw, ne, 2);
		break;
	case 2:
		if (ev_mode == VALENCE_DRIVEN)//generations rules
		{
			u32bits t_live1 = t->cells.twostate.live1, t_live2 = t->cells.twostate.live2;
			if (y < 4)
			{
				t_live1 >>= 8*y + x;
				find2x2node(*n, t_live1, live1, live2, maskNW, pos);
			}
			else
			{
				t_live2 >>= 8*(y - 4) + x;
				find2x2node(*n, t_live2, live1, live2, maskNW, pos);		
			}
			break;
		}
		else//other rules
		{
			cell_t (*p)[BOXSIZE] = t->cells.nstate.cell;
			*n = unite2(p[y + 1][x], p[y + 1][x + 1], p[y][x], p[y][x + 1], 1);				
			break;
		}		
	}
}

void patternToNode(pattern *context, int *n, coord_t x, coord_t y, int N)
{//create nodes from pattern using "recursive descent" from "root" node to 8x8 nodes
	if (x + N < initXLeft || y + N < initYTop || x > initXRight || y > initYBottom)
	{//x and y coordinates are outside root borders, there is no tile in these coordinates
		*n = zero;
		return;		
	}
	int nw, ne, sw, se;
	if (N == 8)
	{
		tile *t = fetchtile(context, x, y);//find tile
		if (t == NULL)//there is no tile
		{
			*n = zero;
			return;		
		}
		N = 4;
		tileToNode(t, &nw, 0, 0, N);
		tileToNode(t, &ne, N, 0, N);		
		tileToNode(t, &sw, 0, N, N);	
		tileToNode(t, &se, N, N, N);
	}
	else
	{
		N /= 2;
		patternToNode(context, &nw, x, y, N);
		patternToNode(context, &ne, x + N, y, N);
		patternToNode(context, &sw, x, y + N, N);		
		patternToNode(context, &se, x + N, y + N, N);
	}
	*n = unite2(sw, se, nw, ne, log2((coord_t)N) + 1);
}

inline void resizeRootCoordinates(coord_t &xRight, coord_t &xLeft, coord_t &yBottom, coord_t &yTop, coord_t &deltaX, coord_t &deltaY)
{//resize root coordinates to make them multiple of 8 and to make height = width
	while ((xLeft & 7) != 0)//all corners coordinates must be multiple of 8
		xLeft--;
	while ((xRight & 7) != 0)
		xRight++;
	while ((yTop & 7) != 0)
		yTop--;
	while ((yBottom & 7) != 0)
		yBottom++;
	deltaX = xRight - xLeft;
	deltaY = yBottom - yTop;
	int temp = 8;
	while (temp < deltaX)
		temp <<= 1;
	deltaX = temp;//now deltaX is 8^n
	temp = 8;
	while (temp < deltaY)
		temp <<= 1;
	deltaY = temp;//now deltaY is 8^n
	if (deltaY < deltaX)
		deltaY = deltaX;
	else
		deltaX = deltaY;
}

void initRootNode(pattern *universe, coord_t xRight, coord_t xLeft, coord_t yBottom, coord_t yTop)
{//make root node from the universe pattern
	coord_t deltaX = 0, deltaY = 0;
	if (universe->tilecount == 0)//for empty universe
	{
		xLeft = xRight = STARTX;
		yBottom = yTop = STARTY;
	}
	initXRight = xRight; initXLeft = xLeft; initYBottom = yBottom; initYTop = yTop;
	resizeRootCoordinates(xRight, xLeft, yBottom, yTop, deltaX, deltaY);//init root coordinates
	xLeft -= 8;//move borders from "life bounds"
	yTop -= 8;
	xRight += 8;
	yBottom += 8;	
	resizeRootCoordinates(xRight, xLeft, yBottom, yTop, deltaX, deltaY);//used to avoid not 8^n deltaX
	rootDepth = log2(deltaX);
	if (universe->tilecount > 1000)//if universe is big it's faster to use recursive function
		patternToNode(universe, &root, xLeft, yTop, deltaX);//recursive search of all subnodes of root
	else//else change each cell in our hashworld using changeCellNode2
	{
		root = 0;
		rootN = deltaX;
		rootX = xLeft;
		rootY = yTop;
		uInt X = xLeft, Y = yTop, N = deltaX, XN = X + N, YN = Y + N;
		for (tile *ptr = universe->tiles; ptr; ptr = ptr->next)
		{//for all tiles in the universe
			int state;
			coord_t dx, dy, x, y;  
			if (!ptr->dead)//if tile exist
				for (dx = 0; dx < BOXSIZE; dx++)//for all cells in the tile
					for (dy = 0; dy < BOXSIZE; dy++)
						if (state = getcell(&ptr->cells, dx, dy)) {//if current cell is not 'dead'
							x = ptr->x + dx;
							y = ptr->y + dy;
							changeCellNode2(x, y, state, X, Y, N, XN, YN, 0);//add cell to root node
						}		
		}
	}
	rootN = 0;
	generations = (uLong) active.generations;
	population = (uLong) cellcnt(&active);
	setStrBuf();//init displayed text
	rootX = xLeft;
	rootY = yTop;
	rootN = (bigNumber)deltaX;
	prevMode = ev_mode;//save previous mode
}

node::node(int a0, int a1, int a2, int a3)
{
	sw = a0;
	se = a1;
	nw = a2;
	ne = a3;
	result = 0;
	popPointer = hashNext = -1;//special initial non zero value
}

node::node()
{
	sw = nw = se = ne = result = 0;
	popPointer = hashNext = -1;//special initial non zero value
}

void rebuildHashTable()
{//recalculate all hash functions and rebuild table
	int length = ev_mode == VALENCE_DRIVEN ? 16 : 256;
	for (int i = length; i < node::curSize; i++)//first 16 (or 256) nodes are special ones(2x2 ot 1x1)
	{
		int tmp = node::hashVector[i].hashF();
		node::hashVector[i].hashNext = -1;
		if (node::hashTable[tmp] == 0)
		{//there are no nodes with the same hash function value
			node::hashTable[tmp] = i;
			continue;
		}
		int j = node::hashTable[tmp], p = node::hashVector[j].hashNext;
		while (p != -1)//go to the last node with hashF() == tmp
		{
			j = p;
			p = node::hashVector[p].hashNext;
		}
		node::hashVector[j].hashNext = i;		
	}
}

int markNodes(int n, int &numberOfNodes, int depth, vector<bigNumber> &popVect, node *newVector, int changeInc)//mark nodes which are parts of root
{
	int length = ev_mode == VALENCE_DRIVEN ? 65552 : 256;
	if (n < length)
		return n;
	if (node::hashVector[n].hashNext == -2)//if we saw this node earlier it has already been added to newVector
		return node::hashVector[n].nw;//so we know it's position
	int k = numberOfNodes++;//save numberOfNodes as position of node n in newVector
	newVector[k].nw = markNodes(node::hashVector[n].nw, numberOfNodes, depth - 1, popVect, newVector, changeInc);
	newVector[k].ne = markNodes(node::hashVector[n].ne, numberOfNodes, depth - 1, popVect, newVector, changeInc);
	newVector[k].sw = markNodes(node::hashVector[n].sw, numberOfNodes, depth - 1, popVect, newVector, changeInc);
	newVector[k].se = markNodes(node::hashVector[n].se, numberOfNodes, depth - 1, popVect, newVector, changeInc);
	if (changeInc && stepPower >= depth - 1)//we changed increment so we must delete out of date nodes' results
		newVector[k].result = markNodes(node::hashVector[n].result, numberOfNodes, depth - 1, popVect, newVector, changeInc);
	else
		newVector[k].result = -3;		
	node::hashVector[n].hashNext = -2;//marked
	node::hashVector[n].nw = k;//save new "address" of node n
	if (depth >= minSaveDepth && (depth - minSaveDepth) % depthStep == 0)//if this node is saved in populationVector
	{
		popVect.push_back(node::populationVector[node::hashVector[n].popPointer]);
		newVector[k].popPointer = node::curPop++;
	}
	return k;
}

void garbageCollector(int changeInc)//delete not used nodes and rehash. If changeInc == 1 then we don't save result fields for nodes with depth < stepPower
{//if changeInc == 2 then don't delete tentativeRoot and it's subnodes
	int length = ev_mode == VALENCE_DRIVEN ? 65552 : 256;
	int numberOfNodes = length;
	vector<bigNumber> popVect;//temporary vector for nodes' populations
	delete [] node::hashTable;//delete old hashTable
	node *newVector = new node [node::curSize];
	memset(newVector, 0, node::curSize*sizeof(node));
	node::curPop = 0;		
	root = markNodes(root, numberOfNodes, rootDepth, popVect, newVector, changeInc);
	if (changeInc == 2)//don't delete tentativeRoot and it's subnodes
		tentativeRoot = markNodes(tentativeRoot, numberOfNodes, tentativeDepth, popVect, newVector, changeInc);
	node::populationVector = popVect;
	node::curSize = numberOfNodes;
	node::maxSize = nextprime(2*numberOfNodes);
	node::hashTable = new int [node::maxSize];//create new hashTable
	memset(node::hashTable, 0, node::maxSize*sizeof(int));	
	memcpy(newVector, node::hashVector, length*sizeof(node));//save old hashVector
	delete [] node::hashVector;
	node *newVector2 = new node [node::maxSize];	
	memset(newVector2, 0, node::maxSize*sizeof(node));
	memcpy(newVector2, newVector, numberOfNodes*sizeof(node));
	delete [] newVector;
	newVector = 0;
	node::hashVector = newVector2;
	rebuildHashTable();//recalculate hashTable
}

void rehash()
{//increase size of hashTable and hashVector, then rebuild
	int size = node::maxSize;
	node::maxSize = nextprime(node::maxSize*2);
	delete [] node::hashTable;
	node::hashTable = new int [node::maxSize];	
	node *newVector = new node [node::maxSize];	
	memset(node::hashTable, 0, node::maxSize*sizeof(int));
	memset(newVector, 0, node::maxSize*sizeof(node));
	memcpy(newVector, node::hashVector, size*sizeof(node));
	delete [] node::hashVector;
	node::hashVector = newVector;
	rebuildHashTable();
}

void calculatePopulation(int n, int &population)//calculate population of node (slow method)
{
	if (n == zero)
		return;
	if (n > 15)
	{//not 2x2 node
		calculatePopulation(node::hashVector[n].nw, population);
		calculatePopulation(node::hashVector[n].ne, population);	
		calculatePopulation(node::hashVector[n].sw, population);
		calculatePopulation(node::hashVector[n].se, population);				
	}
	else
	{
		if (ev_mode == VALENCE_DRIVEN)
		{//"generations" rules
			if (n == 0)
				return;
			if (n == 1 || n == 2 || n == 4 || n == 8)//these magic numbers are 16 2x2 nodes (all states):
				population += 1;
			if (n == 3 || n == 5 || n == 6 || n == 9 || n == 10 || n == 12)
				population += 2;
			if (n == 7 || n == 11 || n == 13 || n == 14)
				population += 3;
			if (n == 15)
				population += 4;
		}
		else
		{//other rules
			if (n != 0)//not "dead" cell
				population++;
		}
	}
}

int node::addToDictionary(int depth, int pop = 0)// if pop == 1 then don't calculate node's population
{//add node to dictionary and return node's position
	if ((float)curSize/maxSize >= 0.75)//there are too many nodes in hashTable, we need rehash
		rehash();
	int hash = this->hashF(), i = hashTable[hash];
	hashVector[curSize] = *this;
	if (depth > minSaveDepth && (depth - minSaveDepth) % depthStep == 0)//calculate population of node with depth = minSaveDepth + depthStep*N; //N > 0
	{
		bigNumber population(pop);
		getPopulation(hashVector[curSize].nw, population, depth - 1);
		getPopulation(hashVector[curSize].ne, population, depth - 1);
		getPopulation(hashVector[curSize].sw, population, depth - 1);
		getPopulation(hashVector[curSize].se, population, depth - 1);
		hashVector[curSize].popPointer = curPop++;//save position in populationVector
		populationVector.push_back(population);//save population into populationVector
	}
	if (depth == minSaveDepth)
	{
		int population = pop;
		if(strcmp(active_rules, "wireworld") && !pop)//not wireworld rules or we already know node's population (pop)
			calculatePopulation(curSize, population);//slow recursive method
		hashVector[curSize].popPointer = curPop++;
		populationVector.push_back(population);
	}
	if (i == 0)
	{//this is first time when we find node with hasF() == hash
		hashTable[hash] = curSize;
		return curSize++;		
	}
	int p = hashVector[i].hashNext;
	while (p != -1)
	{//go to last node with hashF() == hash
		i = p;
		p = hashVector[p].hashNext;
	}
	hashVector[i].hashNext = curSize;
	return curSize++;
}

int expand(int thisNode, int depth) //used to double number of evolution steps: this function doubles node size
{
	int c0 = unite2(zero, zero, zero, node::hashVector[thisNode].sw, depth);
	int c1 = unite2(zero, zero, node::hashVector[thisNode].se, zero, depth);
	int c2 = unite2(zero, node::hashVector[thisNode].nw, zero, zero, depth);	
	int c3 = unite2(node::hashVector[thisNode].ne, zero, zero, zero, depth);
	return unite2(c0, c1, c2, c3, depth + 1);
}

int expandDouble(int k, int depth)
{//this function twice doubles node size (see expand())
	int n = expand(k, depth);
	return expand(n, depth + 1);
}

void getPopulation(int n, bigNumber &population, int depth)
{//recursively calculate total population of nodes
	if (n == zero)
		return;
	if (depth >= minSaveDepth && (depth - minSaveDepth) % depthStep == 0)
	{//if we know node's population we add it to total one
		population += node::populationVector[node::hashVector[n].popPointer];
	}
	else
	{//else we go to subnodes (while their population isn't known)
		getPopulation(node::hashVector[n].nw, population, depth - 1);
		getPopulation(node::hashVector[n].ne, population, depth - 1);	
		getPopulation(node::hashVector[n].sw, population, depth - 1);
		getPopulation(node::hashVector[n].se, population, depth - 1);				
	}
}

bool needGC()
{//return true if we need garbage collector
	if (node::maxSize - node::prevMaxSize >= numOfNewNodes)//3000000
	{//we have too many new nodes, need to clear smth. Maybe it needs to make more intellectual method.
		node::prevMaxSize = node::maxSize;
		return true;
	}
	else
	{
		return false;
	}
}

int nextprime(int i) {//from Golly src: return nearest prime number greater then i
   int j ;
   i |= 1 ;
   for (;; i+=2) {
      for (j=3; j*j<=i; j+=2)
         if (i % j == 0)
            break ;
      if (j*j > i)
         return i ;
   }
}

int getCell(int n, int shift)// shift = 256 - sw, 512 - se, 1 - nw, 2 - ne
{//return 1 cell from 2x2 node n (1 or 0)
	if (n > 7)
		return (shift & (live2 >> pos[n - 8])) ? 1 : 0;
	else
		return (shift & (live1 >> pos[n])) ? 1 : 0;	
}

int recursiveUnite(int n, int depth)
{//recursively unite node n from it's subnodes
	int a0 = node::hashVector[n].sw, a1 = node::hashVector[n].se, a2 = node::hashVector[n].nw, a3 = node::hashVector[n].ne;//4 subnodes
	int a0se = node::hashVector[a0].se, a1sw = node::hashVector[a1].sw, a0ne = node::hashVector[a0].ne, a1nw = node::hashVector[a1].nw;//12 subnodes' subnodes
	int a0nw = node::hashVector[a0].nw, a2sw = node::hashVector[a2].sw, a2se = node::hashVector[a2].se, a3sw = node::hashVector[a3].sw;
	int a1ne = node::hashVector[a1].ne, a3se = node::hashVector[a3].se, a2ne = node::hashVector[a2].ne, a3nw = node::hashVector[a3].nw;
	int b00 = getResult(a0, --depth);//find result of sw subnode
	int b01 = getResult(unite2(a0se, a1sw, a0ne, a1nw, depth), depth);//it can be easier to understand if you make a picture to illustrate these nodes
	int b02 = getResult(a1, depth);
	int b10 = getResult(unite2(a0nw, a0ne, a2sw, a2se, depth), depth);
	int b11 = getResult(unite2(a0ne, a1nw, a2se, a3sw, depth), depth);
	int b12 = getResult(unite2(a1nw, a1ne, a3sw, a3se, depth), depth);
	int b20 = getResult(a2, depth);
	int b21 = getResult(unite2(a2se, a3sw, a2ne, a3nw, depth), depth);
	int b22 = getResult(a3, depth);
	int c00 = 0, c01 = 0, c10 = 0, c11 = 0;
	if (currentHashMode == 1 || stepPower >= depth - 1)//normal recursive conditions
	{
		c00 = getResult(unite2(b00, b01, b10, b11, depth), depth);
		c01 = getResult(unite2(b01, b02, b11, b12, depth), depth);
		c10 = getResult(unite2(b10, b11, b20, b21, depth), depth);
		c11 = getResult(unite2(b11, b12, b21, b22, depth), depth); 
	}
	else
	{//used in constant step mode and in fasthash mode when step is less then N
		if (depth == 2 && ev_mode == VALENCE_DRIVEN)//n is 4x4 node and "generations" rule is active
		{
			int b00ne = getCell(b00, 2), b01nw = getCell(b01, 1), b10se = getCell(b10, 512), b11sw = getCell(b11, 256);
			int b01ne = getCell(b01, 2), b02nw = getCell(b02, 1), b11se = getCell(b11, 512), b12sw = getCell(b12, 256);
			int b10ne = getCell(b10, 2), b11nw = getCell(b11, 1), b20se = getCell(b20, 512), b21sw = getCell(b21, 256);
			int b11ne = getCell(b11, 2), b12nw = getCell(b12, 1), b21se = getCell(b21, 512), b22sw = getCell(b22, 256);
			find2x2node(c00, b10se + (b11sw << 1) + (b00ne << 8) + (b01nw << 9), live1, live2, maskNW, pos);//find correct 2x2 c00 node
			find2x2node(c01, b11se + (b12sw << 1) + (b01ne << 8) + (b02nw << 9), live1, live2, maskNW, pos);
			find2x2node(c10, b20se + (b21sw << 1) + (b10ne << 8) + (b11nw << 9), live1, live2, maskNW, pos);
			find2x2node(c11, b21se + (b22sw << 1) + (b11ne << 8) + (b12nw << 9), live1, live2, maskNW, pos);
		}
		else
		{
			c00 = unite2(node::hashVector[b00].ne, node::hashVector[b01].nw, node::hashVector[b10].se, node::hashVector[b11].sw, depth - 1);
			c01 = unite2(node::hashVector[b01].ne, node::hashVector[b02].nw, node::hashVector[b11].se, node::hashVector[b12].sw, depth - 1);
			c10 = unite2(node::hashVector[b10].ne, node::hashVector[b11].nw, node::hashVector[b20].se, node::hashVector[b21].sw, depth - 1);
			c11 = unite2(node::hashVector[b11].ne, node::hashVector[b12].nw, node::hashVector[b21].se, node::hashVector[b22].sw, depth - 1);
		}
	}
	int result = unite2(c00, c01, c10, c11, depth);	//calculate result of node n
	node::hashVector[n].result = result;
	return n;
}

inline cell_t returnOneCell (cell_t (&p)[4][4], coord_t y, coord_t x)//used for n-state cellular automata
{//return state of one cell after making one step according to rules and neighbours cells
	int neighbours = 0;
	if (ev_mode == TABLE_DRIVEN)
	{//table rules with Von Neumann neighborhood (4 neighbors)
		int newval, oldval;
		int u = p[y-1][x], r = p[y][x+1], d = p[y+1][x], l = p[y][x-1];		
		newval = ztrans[oldval = p[y][x]][u][r][d][l];//use standard xlife methods
		if (newval == BADSTATE)
			make_transition(0, newval = oldval, u, r, d, l, oldval);
		return ztrans[p[y][x]][u][r][d][l];
	}
	if (ev_mode == TAB8_DRIVEN)
	{//table rules with Moore neighborhood (8 neighbors)
		int newval;
		int u = p[y-1][x], r = p[y][x+1], d = p[y+1][x], l = p[y][x-1];
		int ur = p[y-1][x+1], dr = p[y+1][x+1], dl = p[y+1][x-1], ul = p[y-1][x-1];	
		newval = btrans[p[y][x]][u][ur][r][dr][d][dl][l][ul];//use standard xlife methods
		if (newval == BADSTATE)
			make_transition8(0, newval = p[y][x], u, ur, r, dr, d, dl, l, ul, newval);
		return btrans[p[y][x]][u][ur][r][dr][d][dl][l][ul];
	}	
	if (!strchr(active_rules, '+'))
	{//not history rules
		for (int i = x - 1; i < x + 2; i++) {
			if (p[y - 1][i] == 1) neighbours++;
			if (p[y + 1][i] == 1) neighbours++;		
		}
		if (p[y][x - 1] == 1) neighbours++;
		if (p[y][x + 1] == 1) neighbours++;
	}
	else //history mode neighbours count
	{
		for (int i = x - 1; i < x + 2; i++) {
			if (p[y - 1][i] == 1 || p[y - 1][i] == 3 || p[y - 1][i] == 5) neighbours++;
			if (p[y + 1][i] == 1 || p[y + 1][i] == 3 || p[y + 1][i] == 5) neighbours++;		
		}
		if (p[y][x - 1] == 1 || p[y][x - 1] == 3 || p[y][x - 1] == 5) neighbours++;
		if (p[y][x + 1] == 1 || p[y][x + 1] == 3 || p[y][x + 1] == 5) neighbours++;	
	}
	neighbours = 1 << neighbours;
	if(strcmp(active_rules, "wireworld") == 0) //wireworld rules
	{
		if (p[y][x] == 1 || p[y][x] == 2)
			return p[y][x] + 1;
		else if (p[y][x] == 3 && neighbours & 6)
			return 1;
		else
			return p[y][x];
	}
	if (strchr(active_rules, '+'))//history mode rules
	{	
		if ((p[y][x] == 1 || p[y][x] == 3 || p[y][x] == 5) && neighbours & hashLive)
			return p[y][x];	
		if ((p[y][x] == 0 || p[y][x] == 2) && neighbours & hashBorn)
			return 1;	
		if (p[y][x] == 4 && neighbours & hashBorn)
			return 3;
		if (p[y][x] == 1)
			return 2;
		if (p[y][x] == 5 || p[y][x] == 3)
			return 4;
		if (p[y][x] == 6 || p[y][x] == 2 || p[y][x] == 4)
			return p[y][x];			
		return 0;				
	}
	if (p[y][x] > 1)//generations rules with N states
		return (p[y][x] + 1) % maxstates;
	if (p[y][x] == 0 && neighbours & hashBorn)
		return 1;
	if (p[y][x] == 0 && !(neighbours & hashBorn))
		return 0;
	if (p[y][x] == 1 && neighbours & hashLive)
		return 1;
	if (p[y][x] == 1 && !(neighbours & hashLive))
		return (p[y][x] + 1) % maxstates;
}

int getResult(int n, int depth)//depth is depth of n node
{//return result of node n
	if (n == zero)
		return zero;
	if (node::hashVector[n].result != -3)//we know the result so return it
		return node::hashVector[n].result;
	if(eventHandlerCounter++ == 50000)
	{//handle user events to avoid long "not interrupted" actions
		eventHandler();
		eventHandlerCounter = 0;
		if (insideHashAlgorithm == 2)//user pressed 'C' (clear root node)
			throw 1;
	}
	if (depth == 2)//Situation when depth = 2 and node hasn't been hashed yet (N state rules)
	{
		cell_t p[4][4], nw = 0, ne = 0, sw = 0, se = 0;
		p[0][0] = node::hashVector[node::hashVector[n].nw].nw;
		p[0][1] = node::hashVector[node::hashVector[n].nw].ne;
		p[0][2] = node::hashVector[node::hashVector[n].ne].nw;
		p[0][3] = node::hashVector[node::hashVector[n].ne].ne;
		p[1][0] = node::hashVector[node::hashVector[n].nw].sw;
		p[1][1] = node::hashVector[node::hashVector[n].nw].se;
		p[1][2] = node::hashVector[node::hashVector[n].ne].sw;
		p[1][3] = node::hashVector[node::hashVector[n].ne].se;
		p[2][0] = node::hashVector[node::hashVector[n].sw].nw;
		p[2][1] = node::hashVector[node::hashVector[n].sw].ne;
		p[2][2] = node::hashVector[node::hashVector[n].se].nw;
		p[2][3] = node::hashVector[node::hashVector[n].se].ne;
		p[3][0] = node::hashVector[node::hashVector[n].sw].sw;
		p[3][1] = node::hashVector[node::hashVector[n].sw].se;
		p[3][2] = node::hashVector[node::hashVector[n].se].sw;
		p[3][3] = node::hashVector[node::hashVector[n].se].se;
		nw = returnOneCell(p, 1, 1);//get new state for nw cell
		ne = returnOneCell(p, 1, 2);
		sw = returnOneCell(p, 2, 1);
		se = returnOneCell(p, 2, 2);
		return unite2(sw, se, nw, ne, 1);//make new 2x2 node
	}			
	int z = recursiveUnite(n, depth);//calculate result
	return node::hashVector[n].result;//return it	
}

void setStrBuf()
{//init displayed text
	strcpy(strBuf, "G");
        if (shortText) {//short text mode (display only first letters)
		strcat(strBuf, generations.toString(18, 0).c_str());
		strcat(strBuf, " C");
		strcat(strBuf, population.toString(18, 0).c_str());
		strcat(strBuf, " S");
		strcat(strBuf, 	currentStep.toString(18, 0).c_str());
		if (dispspeed)
		{
			strcat(strBuf, " S");
			strcat(strBuf, hashSpeed.toString(18, 0).c_str());
		}
	}
        else {//normal mode (display whole words)
		strcat(strBuf, "enerations: ");
		strcat(strBuf, generations.toString(18, 0).c_str());
		strcat(strBuf, ", Cells: ");
		strcat(strBuf, population.toString(18, 0).c_str());
		strcat(strBuf, ", Step: ");
		strcat(strBuf, 	currentStep.toString(18, 0).c_str());
		if (dispspeed)
		{
			strcat(strBuf, ", Speed: ");
			strcat(strBuf, hashSpeed.toString(18, 0).c_str());
		}
	}
}

void extraNodes(int &newN, int &newNE, int &newE, int &newSE, int &newS, int &newSW, int &newW, int &newNW)
{//calculate 8 neighbours of root node (used to find if there are alive cells outside root node after some generations)
	int bigNW = unite2(zero, node::hashVector[root].nw, zero, zero, rootDepth);//looks difficult but if you draw it...
	int bigNE = unite2(node::hashVector[root].ne, zero, zero, zero, rootDepth);
	int bigSW = unite2(zero, zero, zero, node::hashVector[root].sw, rootDepth);
	int bigSE = unite2(zero, zero, node::hashVector[root].se, zero, rootDepth);
	newN = getResult(unite2(bigNW, bigNE, zero, zero, rootDepth + 1), rootDepth + 1);//looks not easy but ther is an advice some lines above
	newNE = getResult(unite2(bigNE, zero, zero, zero, rootDepth + 1), rootDepth + 1);
	newE = getResult(unite2(bigSE, zero, bigNE, zero, rootDepth + 1), rootDepth + 1);
	newSE = getResult(unite2(zero, zero, bigSE, zero, rootDepth + 1), rootDepth + 1);
	newS = getResult(unite2(zero, zero, bigSW, bigSE, rootDepth + 1), rootDepth + 1);
	newSW = getResult(unite2(zero, zero, zero, bigSW, rootDepth + 1), rootDepth + 1);
	newW = getResult(unite2(zero, bigSW, zero, bigNW, rootDepth + 1), rootDepth + 1);
	newNW = getResult(unite2(zero, bigNW, zero, zero, rootDepth + 1), rootDepth + 1);			
}

void hashGenerate (pattern *context, char *strBuf)
{//main hash function: make some generations according to mode and root's size
	try
	{
		if (generations == 0 && (born & 1) != 0 && (live & 256) != 0)
		{//cheat: make first step for B0 rule using tile algorithm (for further correct evolution)
			hash2tile(ev_mode);//from hash to tile
			generate(&active);//one step
			bounding_box(&active);
			initRootNode(&active, active.xmax, active.xmin, active.ymax, active.ymin);//from tile to hash
		}
		insideHashAlgorithm = 1;//we are inside hash algorithm (used for event handler)
		if (needGC())//there are too many not needed nodes
			garbageCollector(0);
		struct timeval start, end;//for speed calculation
		gettimeofday(&start, 0);
		int tempRoot = root, tempDepth = rootDepth;//copy of root and depth
		bigNumber rootNCopy(rootN);//copy of rootN
		int newN = 0,newS = 0, newW = 0, newE = 0, newNW = 0, newSW = 0, newSE = 0, newNE = 0;
		tempRoot = expand(tempRoot, tempDepth);//double root size
		tempRoot = getResult(tempRoot, tempDepth + 1);//The main thing: find result for expanded root, it will be the new one
		extraNodes(newN, newNE, newE, newSE, newS, newSW, newW, newNW);//Are there any alive cells outside root's borders?
		if(newN != zero || newNE != zero || newE != zero || newSE != zero || newS != zero || newSW != zero || newW != zero || newNW != zero)
		{//yes, there are, so we must expand root again to avoid loss of new alive cells
			int nw = unite2(newW, tempRoot, newNW, newN, tempDepth + 1), ne = unite2(newE, zero, newNE, zero, tempDepth + 1);//don't forget to draw it :-)
			int sw = unite2(zero, zero, newSW, newS, tempDepth + 1), se = unite2(zero, zero, newSE, zero, tempDepth + 1);
			tempRoot = unite2(sw, se, nw, ne, tempDepth + 2);
			rootX -= rootNCopy;//calculate new root size and coordinates
			rootY -= rootNCopy;
			rootNCopy *= 4;
			tempDepth += 2;
			expandCounter = 1;
		}
		else 
		{//no, there aren't
			if (currentHashMode != 3 && !(expandCounter++%20) && hashRatio < 3000000)
			{//not constStep mode and time has come to increase stem and root size
				tempRoot = expand(tempRoot, tempDepth++);
				rootX -= rootNCopy/2;//calculate new root size and coordinates
				rootY -= rootNCopy/2;
				rootNCopy *= 2;
				expandCounter = 1;
			}
		}
		root = tempRoot;//copy temp back to root
		rootDepth = tempDepth;
		rootN /= 2;
		if (currentHashMode == 1)//"classical" hash mode
		{
			currentStep = rootN;
			stepPower = log2(rootN);
		}
		generations += currentStep;//calculate generations
		if(strcmp(active_rules, "wireworld") != 0)
		{//not wireworld rules: calculate new population
			population = 0;
			if (rootDepth >= minSaveDepth)
				getPopulation(root, population, rootDepth);
			else
			{
				int tempPop = 0;
				calculatePopulation(root, tempPop);
				population = tempPop;
			}
		}
		gettimeofday(&end, 0);
		hashRatio = (end.tv_sec - start.tv_sec)*1000000 + end.tv_usec - start.tv_usec + 1;		
		hashSpeed = currentStep*1000000/hashRatio;//speed calculation
		setStrBuf();//text
		rootN = rootNCopy;
		clear_pattern(context);
		insideHashAlgorithm = 0;//end of hash algorithm
		if (currentHashMode == 2 && !(incStep++%3) && hashRatio < 3000000 && stepPower < rootDepth - 1)
		{//fastHash mode: increase step size
			INCSTEP;
			garbageCollector(1);
		}
	}
	catch (std::bad_alloc& badAlloc)
	{
		ERROR("Out of memory: hash algorithm stopped.");
	}
	catch (int k)
	{
		state = STOP;
		insideHashAlgorithm = 0;
	}
}

template<class T>
inline bool nodeOnScreen(const T &N, const T &x, const T &y) 
{//returns true if node with coords x, y and size N is seen on the screen
	if (x + N < bigXPos || y + N < bigYPos || x > bigScreenWidth || y > bigScreenHeight)
		return false;
	return true;
}

void nodeToTile(int n, tile *t, coord_t N, coord_t x, coord_t y, int mode)
{//convert node to tile
	if (N == 4)
	{//4x4 nodes: we need to make one step down
		N /= 2;
		nodeToTile(node::hashVector[n].nw, t, N, x, y, mode);
		nodeToTile(node::hashVector[n].ne, t, N, x + N, y, mode);	
		nodeToTile(node::hashVector[n].sw, t, N, x, y + N, mode);
		nodeToTile(node::hashVector[n].se, t, N, x + N, y + N, mode);	
		return;	
	}
	if (N == 2)
	{//2x2 nodes
		if (mode == VALENCE_DRIVEN)
		{//"generations" rules	
			u32bits mask = maskNW;
			if (prevMode == mode)
			{
				if (n < 8)
					mask &= live1 >> pos[n];
				else
					mask &= live2 >> pos[n - 8];
			}
			else
			{//rules' type changed
				if(strchr(saved_rules, '+'))//history mode rules
				{//if current cell in node is one of "not alive" cells we move 0 to it's position in tile else move 1
					mask &= (node::hashVector[n].nw != 1 && node::hashVector[n].nw != 3 && node::hashVector[n].nw != 5) ? 770 : 771;//these magic numbers are bitwise masks
					mask &= (node::hashVector[n].ne != 1 && node::hashVector[n].ne != 3 && node::hashVector[n].ne != 5) ? 769 : 771;
					mask &= (node::hashVector[n].sw != 1 && node::hashVector[n].sw != 3 && node::hashVector[n].sw != 5) ? 515 : 771;
					mask &= (node::hashVector[n].se != 1 && node::hashVector[n].se != 3 && node::hashVector[n].se != 5) ? 259 : 771;
				}
				else
				{//dead cell remains dead, alive remains alive
					mask &= node::hashVector[n].nw == 0 ? 770 : 771;
					mask &= node::hashVector[n].ne == 0 ? 769 : 771;
					mask &= node::hashVector[n].sw == 0 ? 515 : 771;
					mask &= node::hashVector[n].se == 0 ? 259 : 771;
				}				
			}
			if (y < 4)//move mask to correct position in tile
				t->cells.twostate.live1 |= mask << (x + 8*y);
			else		
				t->cells.twostate.live2 |= mask << (x + 8*(y - 4));
		}
		else
		{//N state rules
			if (prevMode == mode)
			{//direct conversion from node to cell
				t->cells.nstate.cell[y][x] = node::hashVector[n].nw;
				t->cells.nstate.cell[y][x + 1] = node::hashVector[n].ne;
				t->cells.nstate.cell[y + 1][x] = node::hashVector[n].sw;
				t->cells.nstate.cell[y + 1][x + 1] = node::hashVector[n].se;
			}
			else
			{//rules' type changed
				u32bits mask = maskNW;
				if (n < 8)
					mask &= live1 >> pos[n];
				else
					mask &= live2 >> pos[n - 8];
				t->cells.nstate.cell[y][x] = (mask & 1) ? 1 : 0;//dead cell remains dead, alive remains alive
				t->cells.nstate.cell[y][x + 1] = (mask & 2) ? 1 : 0;
				t->cells.nstate.cell[y + 1][x] = (mask & 256) ? 1 : 0;
				t->cells.nstate.cell[y + 1][x + 1] = (mask & 512) ? 1 : 0;	
			}		
		}	
	}	
}

void hash2tile(int mode) {//convert root node to active pattern
   clear_pattern(&active);
   nodeToPattern(root, &active, rootN, rootX, rootY, mode);//conversion
   if (bigXPos <= 42949617295 && bigYPos <= 42949617295 && bigXPos >= 0 && bigYPos >= 0)
   {//convert screen coordinates
      xpos = bigXPos.toUint();
      ypos = bigYPos.toUint(); 
   }
   else//current big coordinates are too big so we have to reduce screen coordinates to START values
   {
      bigXPos = xpos = STARTX;
      bigYPos = ypos = STARTY;
   }
   prevMode = mode;//save previous mode (rules' type)
   std::stringstream oss;//convert big generations to small active.generations
   oss << generations % (1L << (sizeof(unsigned long)*8 - 1));
   oss >> active.generations;
   bounding_box(&active);
}

void nodeToPattern(int n, pattern *context, bigNumber N, const bigNumber &x, const bigNumber &y, int mode)
{//convert node to pattern
	if (n == zero)
		return;
	if (x > maxCoord || y > maxCoord || x + rootN < 0 || y + rootN < 0)//node is outside of the Universe
		return;
	if (N == 8)
	{//8x8 node, then we use nodeToTile() instead of nodeToPattern()
		tile *t = maketile(context, x.toUint(), y.toUint());
		t->cells.twostate.live1 = t->cells.twostate.live2 = 0;
		N /= 2;
		coord_t shortN = N.toUint();
		nodeToTile(node::hashVector[n].nw, t, shortN, 0, 0, mode);
		nodeToTile(node::hashVector[n].ne, t, shortN, shortN, 0, mode);	
		nodeToTile(node::hashVector[n].sw, t, shortN, 0, shortN, mode);
		nodeToTile(node::hashVector[n].se, t, shortN, shortN, shortN, mode);	
	}
	else
	{//node is bigger then tile, use this recursive method
		N /= 2;
		nodeToPattern(node::hashVector[n].nw, context, N, x, y, mode);
		nodeToPattern(node::hashVector[n].ne, context, N, x + N, y, mode);	
		nodeToPattern(node::hashVector[n].sw, context, N, x, y + N, mode);
		nodeToPattern(node::hashVector[n].se, context, N, x + N, y + N, mode);
	}
}

void colorFromNode(int n, bigNumber N, unsigned &color)
{//used to get color from N state node (we find first available non zero cell and return it's color)
	if (n == zero)
		return;
	if (N == 1)
	{
		color = n;//color found
		return;
	}
	N /= 2;
	colorFromNode(node::hashVector[n].nw, N, color);
	if (color)
		return;
	colorFromNode(node::hashVector[n].sw, N, color);
	if (color)
		return;	
	colorFromNode(node::hashVector[n].se, N, color);
	if (color)
		return;	
	colorFromNode(node::hashVector[n].sw, N, color);
	if (color)
		return;		
}

void nodeToVideoTileShort(int n, coord_t ix, coord_t iy, coord_t N, coord_t x, coord_t y)
{//display small node on screen (coord_t used instead of bigNumber to increase speed)
	if (N == 4)
	{
		N /= 2;
		nodeToVideoTileShort(node::hashVector[n].nw, ix, iy, N, x, y);
		nodeToVideoTileShort(node::hashVector[n].ne, ix, iy, N, x + N, y);	
		nodeToVideoTileShort(node::hashVector[n].sw, ix, iy, N, x, y + N);
		nodeToVideoTileShort(node::hashVector[n].se, ix, iy, N, x + N, y + N);	
		return;	
	}
	if (N == 2)
	{//2x2 node
		if (ev_mode == VALENCE_DRIVEN)
		{
			if (getCell(n, 2))//if cell is alive
				fb_ins(ix + x + 1, iy + y, 1);//display it
			if (getCell(n, 1))
				fb_ins(ix + x, iy + y, 1);
			if (getCell(n, 512))
				fb_ins(ix + x + 1, iy + y + 1, 1);
			if (getCell(n, 256))
				fb_ins(ix + x, iy + y + 1, 1);
                }
                else
                {//N state rules
                	if (node::hashVector[n].nw != 0)//if cell is alive
				fb_ins(ix + x, iy + y, node::hashVector[n].nw);//display it
			if (node::hashVector[n].sw != 0)			
				fb_ins(ix + x, iy + y + 1, node::hashVector[n].sw);
			if (node::hashVector[n].ne != 0)		
				fb_ins(ix + x + 1, iy + y, node::hashVector[n].ne);
			if (node::hashVector[n].se != 0)
				fb_ins(ix + x + 1, iy + y + 1, node::hashVector[n].se);
		}
	}
}


void nodeToVideoTile(int n, const bigNumber &ix, const bigNumber &iy, bigNumber N, const bigNumber &x, const bigNumber &y)
{//display small node on screen
	if (N == 4)
	{
		N /= 2;
		nodeToVideoTile(node::hashVector[n].nw, ix, iy, N, x, y);
		nodeToVideoTile(node::hashVector[n].ne, ix, iy, N, x + N, y);	
		nodeToVideoTile(node::hashVector[n].sw, ix, iy, N, x, y + N);
		nodeToVideoTile(node::hashVector[n].se, ix, iy, N, x + N, y + N);	
		return;	
	}
	if (N == 2)
	{
		if (ev_mode == VALENCE_DRIVEN)
		{
			if (getCell(n, 2))//if cell is alive
				fb_ins_big(ix + x + 1, iy + y, 1);//display it
			if (getCell(n, 1))
				fb_ins_big(ix + x, iy + y, 1);
			if (getCell(n, 512))
				fb_ins_big(ix + x + 1, iy + y + 1, 1);
			if (getCell(n, 256))
				fb_ins_big(ix + x, iy + y + 1, 1);
                }
                else
                {//N state rules
                	if (node::hashVector[n].nw != 0)//if cell is alive
				fb_ins_big(ix + x, iy + y, node::hashVector[n].nw);//display it
			if (node::hashVector[n].sw != 0)			
				fb_ins_big(ix + x, iy + y + 1, node::hashVector[n].sw);
			if (node::hashVector[n].ne != 0)		
				fb_ins_big(ix + x + 1, iy + y, node::hashVector[n].ne);
			if (node::hashVector[n].se != 0)
				fb_ins_big(ix + x + 1, iy + y + 1, node::hashVector[n].se);
		}
	}
}

void hash_fb_update(int *scale) {//update screen
	if (tentativeRoot)//if tentative root exist
		tentativeNodeToVideo(tentativeRoot, tentativeN, tentativeX, tentativeY, scale);//display it
	nodeToVideo(root, rootN, rootX, rootY, scale);//display root node
}

void nodeToVideoShort(int n, coord_t N, coord_t x, coord_t y, int *scale)
{//display node on screen (coord_t used instead of bigNumber to increase speed)
	if (x > maxCoord || y > maxCoord || x + N < 0 || y + N < 0)//node is outside of the Universe
		return;
	if (n == zero)
		return;		
	if (!nodeOnScreen(N, x, y))//node is outside of the screen
		return;
	if (*scale <= 0 && (1 << -*scale) >= N)
	{//improve video output at large scales (there is a node at this pixel)
		if (!multiColor || maxstates == 2)//2 state rules or monochrome mode
			fb_ins(x, y, 1);
		else
		{//multicolor mode
			unsigned color = 0;
			colorFromNode(n, N, color);//find color
			fb_ins(x, y, color);//draw it
		}			
		return;
	}	
	if (N == 8)
	{//8x8 node: use nodeToVideoTileShort() funciton
		N /= 2;
		nodeToVideoTileShort(node::hashVector[n].nw, x, y, N, 0, 0);		
		nodeToVideoTileShort(node::hashVector[n].ne, x, y, N, N, 0);
		nodeToVideoTileShort(node::hashVector[n].sw, x, y, N, 0, N);		
		nodeToVideoTileShort(node::hashVector[n].se, x, y, N, N, N);
	}
	else
	{
		N /= 2;
		nodeToVideoShort(node::hashVector[n].nw, N, x, y, scale);
		nodeToVideoShort(node::hashVector[n].ne, N, x + N, y, scale);	
		nodeToVideoShort(node::hashVector[n].sw, N, x, y + N, scale);
		nodeToVideoShort(node::hashVector[n].se, N, x + N, y + N, scale);
	}
}

void nodeToVideo(int n, bigNumber N, const bigNumber &x, const bigNumber &y, int *scale)
{//display node on scree
	if (n == zero)
		return;		
	if (!nodeOnScreen(N, x, y))//node is outside of the screen
		return;
	if (*scale < 0 && lrint(pow(2, -*scale)) >= N)
	{//improve video output at large scales (there is a node at this pixel)
		if (!multiColor || maxstates == 2)//2 state rules or monochrome mode
			fb_ins_big(x, y, 1);
		else
		{//multicolor mode
			unsigned color = 0;
			colorFromNode(n, N, color);
			fb_ins_big(x, y, color);
		}
		return;
	}
	if (*scale > MINSCALE && N > 8 && x > 0 && y > 0 && x + N < 4294967295lu && y + N < 4294967295lu)
	{//big coordinates are not so big, we can use short coords and nodeToVideoShort()
		N /= 2;
		coord_t shortX = x.toUint(), shortY = y.toUint(), shortN = N.toUint();
		nodeToVideoShort(node::hashVector[n].nw, shortN, shortX, shortY, scale);
		nodeToVideoShort(node::hashVector[n].ne, shortN, shortX + shortN, shortY, scale);	
		nodeToVideoShort(node::hashVector[n].sw, shortN, shortX, shortY + shortN, scale);
		nodeToVideoShort(node::hashVector[n].se, shortN, shortX + shortN, shortY + shortN, scale);
		return;	
	}
	if (N == 8)
	{//8x8 node: we can use nodeToVideoTile()
		N /= 2;
		nodeToVideoTile(node::hashVector[n].nw, x, y, N, 0, 0);		
		nodeToVideoTile(node::hashVector[n].ne, x, y, N, N, 0);
		nodeToVideoTile(node::hashVector[n].sw, x, y, N, 0, N);		
		nodeToVideoTile(node::hashVector[n].se, x, y, N, N, N);	
	}
	else
	{
		N /= 2;		
		nodeToVideo(node::hashVector[n].nw, N, x, y, scale);
		nodeToVideo(node::hashVector[n].ne, N, x + N, y, scale);	
		nodeToVideo(node::hashVector[n].sw, N, x, y + N, scale);
		nodeToVideo(node::hashVector[n].se, N, x + N, y + N, scale);
	}
}

void clearHash()
{//clear all memory
	delete [] node::hashTable;
	delete [] node::hashVector;
	node::populationVector.clear();
}

int changeCellNode(coord_t x, coord_t y, cell_t newstate, int get = 0)//if (get != 0) this function is used to return current cell color (not changing cell state)
{
	if ((x < rootX || y < rootY || x > rootX + rootN || y > rootY + rootN) && newstate == 0)
		return 0;//new cell is dead and out of hash universe border		
	while (x < rootX || y < rootY)
	{//while new cell is out of universe border we must expand our universe
		root = unite2(zero, root, zero, zero, ++rootDepth);
		rootX -= rootN;
		rootY -= rootN;
		rootN *= 2;
	}
	while (rootX + rootN < x || rootY + rootN < y)
	{//same as previous 'while'
		root = unite2(zero, zero, root, zero, ++rootDepth);
		rootN *= 2;
	}
	int n = root, NW = 1, NE = 2, SW = 256, SE = 512, depth = rootDepth, mask, minDepth = ev_mode == VALENCE_DRIVEN ? 1 : 0;
	uInt intX = 0, intY = 0, intN = 0, intXN = 0, intYN = 0, flag = 2, nodeListCounter = 0, tempListCounter = 0;
	cell_t result = 0;
	bigNumber X(rootX), Y(rootY), N(rootN), XN(X + N), YN(Y + N);
	vector<int> nodeList, tempList;//vectors for 2 stacks
	nodeList.reserve(depth);//reserve size to avoid resize()
	tempList.reserve(depth);	
	while (depth-- > minDepth)
	{//recursive like descent to 2x2 node where changeable cell is located
		nodeList[nodeListCounter++] = n;
		if (flag == 2 && X + N < 4296987295l && Y + N < 4296987295l && X > 0 && Y > 0)
		{
			flag = 1;//coordinates are not so big, we can use uInt instead of bigNumbers
			intX = X.toUint();
			intY = Y.toUint();
			intN = N.toUint();
		}
		if (flag == 1)
		{//use uInt
			intN /= 2;		
			intXN = intX + intN;
			intYN = intY + intN;		
			TOVECTOR(intX, intY, intN, intXN, intYN);//save subnode and it's position (sw, se, nw or ne)
		}
		else
		{//bigNumbers
			N /= 2;
			XN = X + N;
			YN = Y + N;			
			TOVECTOR(X, Y, N, XN, YN);//the same
		}								
	}
	BIGCODE(intX, intY);
	setStrBuf();//update number of cells
	return 1;
}

int changeCellNode2(coord_t x, coord_t y, cell_t newstate, int X, int Y, int N, int XN, int YN, int get = 0)
{//the same as changeCellNode() but faster (for short numbers only)
	int n = root, NW = 1, NE = 2, SW = 256, SE = 512, depth = rootDepth, mask, minDepth = ev_mode == VALENCE_DRIVEN ? 1 : 0, nodeListCounter = 0, tempListCounter = 0;
	vector<int> nodeList, tempList;
	nodeList.reserve(depth);
	tempList.reserve(depth);
	cell_t result = 0;	
	while (depth-- > minDepth)
	{//recursive like descent to 2x2 node where changeable cell is located
		nodeList[nodeListCounter++] = n;
		N /= 2;
		XN = X + N;
		YN = Y + N;
		TOVECTOR(X, Y, N, XN, YN);						
	}
	BIGCODE(X, Y);
	return 1;
}

void redisplayNode(int *scale)
{//redisplay
	nodeToVideo(root, rootN, rootX, rootY, scale);
}

void clearHashLife()
{//completely clear hashlife
	incStep = 1;
	stepPower = 0;
	currentStep = 1;
	root = prevRoot = zero;//clear root node
	clearHash();//clear memory
	initHashlife();//init table, rules etc.
	rootDepth = 4;
	rootX = STARTX;//clear coordinates
	rootY = STARTY;
	rootN = 16;
	population = generations = 0;
}

void calculateSlashInfo(int n)
{//calculate info: numbers of cells with different states
	if (n == zero)
		return;
	if (n < 256)
	{
		cellsCount[n]++;
		return;
	}
	calculateSlashInfo(node::hashVector[n].nw);
	calculateSlashInfo(node::hashVector[n].ne);
	calculateSlashInfo(node::hashVector[n].sw);
	calculateSlashInfo(node::hashVector[n].se);	
}

void redraw_hash_slashinfo()
{//display numbers of cells with different states
	int i, q = 0, h, l;
	char s[80];
	XClearWindow(disp, helpw);
	h = width/14/FONTWIDTH;
	if (root == zero)
		XDrawString(disp, helpw, itextgc, 10, FONTHEIGHT, "Life is extinct", 15);
	else {
		if (maxstates == 2)//2state rules
			cellsCount[1] = population;//don't calculate this info: we know it
		else if (root != prevRoot) {//if we have already calculated info for this root, it's not necessary to do it again
			for(int i = 0; i < 256; i++)
				cellsCount[i] = 0;
			calculateSlashInfo(root);
		}
		for (i = 1; i < maxstates; i++)//display
			if (cellsCount[i] != 0) {
				sprintf(s, "[%d] %s", i, cellsCount[i].toString(20, 0).c_str());
				XDrawString(disp, helpw, itextgc, 10 + (q%h)*14*FONTWIDTH, (q/h + 1)*FONTHEIGHT, s, strlen(s));
				q++;
			}
		if (maxstates > 2) {
			sprintf(s, "[*] %s", population.toString(20, 0).c_str());
			XDrawString(disp, helpw, itextgc, 10 + (q%h)*14*FONTWIDTH, (q/h + 1)*FONTHEIGHT, s, strlen(s));
			q++;
		}
	}
	prevRoot = root;
	XDrawString(disp, helpw, itextgc, 5, (q/h + 5)*FONTHEIGHT, "Press a key to continue", 23);
}

void boundingBox(int n, const bigNumber &x, const bigNumber &y, const bigNumber &nodeSize, bigNumber &xLeft, bigNumber &yTop, bigNumber &xRight, bigNumber &yBottom)
{//calculate "life" bounds with the required precision (error is less then rootN/128 for big nodes)
	if (n == zero)
		return;
	bigNumber minNodeSize(rootN > 256? rootN/128 : 4);
	if (nodeSize < minNodeSize)
	{//node is too small, error is not permitted
		if (x < xLeft)
			xLeft = x;
		if (x > xRight)
			xRight = x;
		if (y < yTop)
			yTop = y;
		if (y > yBottom)
			yBottom = y;				
		return;
	}
	bigNumber N(nodeSize/2);
	boundingBox(node::hashVector[n].nw, x, y, N, xLeft, yTop, xRight, yBottom);
	boundingBox(node::hashVector[n].ne, x + N, y, N, xLeft, yTop, xRight, yBottom);
	boundingBox(node::hashVector[n].sw, x, y + N, N, xLeft, yTop, xRight, yBottom);
	boundingBox(node::hashVector[n].se, x + N, y + N, N, xLeft, yTop, xRight, yBottom);
}

void hashBoundingBox() 
{//calculate and display "life" bounds
	if (root == zero)
		sprintf(inpbuf, "Life is extinct");
	else
	{
		bigNumber xLeft(rootX + rootN), xRight(rootX), yTop(rootY + rootN), yBottom(rootY);
		boundingBox(root, rootX, rootY, rootN, xLeft, yTop, xRight, yBottom);//calculate "life" bounds
		if (xRight <= xLeft)//correct errors
			xRight = rootX + rootN/2;
		if (yBottom <= yTop)
			yBottom = rootY + rootN/2;	
		sprintf(inpbuf, "Life bounds: %s <= x <= %s  %s <= y <= %s", (xLeft - 2147483647).toString(20, 0).c_str(),
			(xRight - 2147483647).toString(20, 0).c_str(), (yTop - 2147483647).toString(20, 0).c_str(), (yBottom - 2147483647).toString(20, 0).c_str());	
	}
        announce_and_wait(0);
        displaystats();
}

void hashMiddleScr()
{//move screen to the center of root node
	bigNumber xLeft(rootX + rootN), xRight(rootX), yTop(rootY + rootN), yBottom(rootY);
	boundingBox(root, rootX, rootY, rootN, xLeft, yTop, xRight, yBottom);//calculate "life" bounds
	if (xRight <= xLeft)
		xRight = rootX + rootN/2;
	if (yBottom <= yTop)
		yBottom = rootY + rootN/2;
	bigNumber oldX = bigXPos, oldY = bigYPos;
	bigXPos = (xLeft + xRight)/2 - SCALE((bigNumber)width/2);//change big screen coordinates
	bigYPos = (yTop + yBottom)/2 - SCALE((bigNumber)height/2);
	xpos = ((xLeft + xRight)/2).toUint() - SCALE(width/2);//change normal screen coordinates
	ypos = ((yTop + yBottom)/2).toUint() - SCALE(height/2);	
	SETBIGSCREENWIDTH;//change screen width
	SETBIGSCREENHEIGHT;
	display_move(0, -xpos + oldX.toUint(), -ypos + oldY.toUint());//move display
}

void gc(int m)
{//garbage collector
	garbageCollector(m);
}

void doSomeHashSteps(char* buf, int mode)
{//make a certain number of generations
	try
	{
		struct timeval start, end;
		double tm;
		gettimeofday(&start, 0);
		bigNumber newGenerations(buf);//number of generations
		digit r = 0;
		stepPower = 0;
		currentStep = 1;
		currentHashMode = 2;//choose Golly like mode to perform necessary number of generations
		garbageCollector(1);
		while (newGenerations > 0)
		{
			tinyDivide(newGenerations, newGenerations, 2, r);
			if (r)
			{//to perform necessary number of generations we must hashGenerate() with this step size 
				hashGenerate(&active, strBuf);
				garbageCollector(1);
			}
			INCSTEP;//increase step
			if (stepPower >= rootDepth - 1)
			{//step is too big, expand root node
				root = expand(root, rootDepth++);
				rootX -= rootN/2;
				rootY -= rootN/2;			
				rootN *= 2;
			}
		}
		gettimeofday(&end, 0);
		hashRatio = (end.tv_sec - start.tv_sec)*1000000 + end.tv_usec - start.tv_usec + 1;		
		hashSpeed = currentStep*1000000/hashRatio;//calculate speed
		redraw_lifew();//redisplay
		if (mode)//'B' command
		{
			tm = end.tv_sec - start.tv_sec + (end.tv_usec - start.tv_usec)/1000000.0;
			sprintf(strBuf, "Time: %fs, Speed: %s", tm, hashSpeed.toString(20, 0).c_str());
			displaystats();
			announce_and_wait(0);
		}
		setStrBuf();//display text
	}
	catch (int i)
	{
		CATCH_EXCEPTION("Wrong number of generations.");	
	}
}

void setStep (char *buf)
{//set step from C string
	try
	{
		bigNumber newStep(buf);
		int log2step = log2(newStep);
		if (newStep != lrint(pow(2, log2step)))
			ERROR("Step must be power of 2.");
		else
		{
			stepPower = log2step;
			currentStep = newStep;
			currentHashMode = 3;//choose constant step mode
			garbageCollector(1);
			while (stepPower >= rootDepth - 1)
			{//step is too big, expand root node
				root = expand(root, rootDepth++);
				rootX -= rootN/2;
				rootY -= rootN/2;			
				rootN *= 2;
			}
			sprintf(inpbuf, "Step set to %s", currentStep.toString(20, 0).c_str());
		}
	}
	catch (int i)
	{
		CATCH_EXCEPTION("Wrong step.");	
	}
}

int addTentativeNodes(node *nodesVector, int n, int depth)
{//add node n from nodesVector to dictionary and return it's position
	if (n == zero)
		return zero;
	int minDepth = ev_mode == VALENCE_DRIVEN ? 3 : 1;
	if(nodesVector[n].hashNext == -10)//we have already added this node, return it's position
		return unite2(nodesVector[n].sw, nodesVector[n].se, nodesVector[n].nw, nodesVector[n].ne, depth);
	if (depth > minDepth)
	{//add all subnodes
		nodesVector[n].nw = addTentativeNodes(nodesVector, nodesVector[n].nw, depth - 1);
		nodesVector[n].ne = addTentativeNodes(nodesVector, nodesVector[n].ne, depth - 1);	
		nodesVector[n].sw = addTentativeNodes(nodesVector, nodesVector[n].sw, depth - 1);
		nodesVector[n].se = addTentativeNodes(nodesVector, nodesVector[n].se, depth - 1);
		nodesVector[n].hashNext = -10;//to avoid several searches for one node
	}	
	return unite2(nodesVector[n].sw, nodesVector[n].se, nodesVector[n].nw, nodesVector[n].ne, depth);
}

int loadHashFile(FILE *loadfl, char *buf)
{//load file with hash pattern, return 1 if succesful, 0 else
	try
	{
		char bignumbers[5][255] = {0}, temp[355] = {0};
		int tempdepth, temproot;
                if (topology != 'Q' || oscillators || truehistory || runcounter || (born & 1) != 0 && (live & 256) == 0)
			throw "wrong rule";
		if (sscanf(buf, "#H %d p=%s g=%s x=%s y=%s n=%s d=%d r=%d", &currentHashMode, bignumbers[0], bignumbers[1], bignumbers[2], bignumbers[3], bignumbers[4], &tempdepth, &temproot) != 8)
			throw "Bad parameters";
		if (aloadmode)//direct load to root node
			clearHashLife();
		garbageCollector(0);//clear all unnecessary nodes
		fb_flush();
		redisplay(0);
		if (aloadmode)
		{//direct load mode
			population = bignumbers[0];
			generations = bignumbers[1];
			rootX = bignumbers[2];	
			rootY = bignumbers[3];	
			rootN = bignumbers[4];
			root = temproot;
			rootDepth = tempdepth;
		}
		else {//load to tentative node
			tentativePopulation = bignumbers[0];
			tentativeGenerations = bignumbers[1];
			tentativeX = bignumbers[2];	
			tentativeY = bignumbers[3];	
			tentativeN = bignumbers[4];
			tentativeRoot = temproot;
			tentativeDepth = tempdepth;
		}
		int count = ev_mode == VALENCE_DRIVEN ? 65557 : 257;//number of never changable nodes
		node *nodesVector = new node [count];
		while (1)
		{//read all nodes from file
			fgets(temp, 355, loadfl);
			if (feof(loadfl))//end of file
				break;
			int nw, ne, sw, se, n;
			if (sscanf(temp, "%d %d %d %d %d", &n, &sw, &se, &nw, &ne) != 5)
				throw "Something is going wrong";//wrong parameters
			while (n > count - 1)//increase nodesVector size
			{
				node *temp = new node [count*2];
				memcpy(temp, nodesVector, count*sizeof(node));
				count *= 2;
				delete [] nodesVector;
				nodesVector = temp;
			}
			node newNode(sw, se, nw, ne);
			newNode.result = -3;
			nodesVector[n] = newNode;//add to nodesVector	
		}//we added all nodes from file into nodesVector
		if (aloadmode)
			root = addTentativeNodes(nodesVector, root, rootDepth);//here we recursively add all nodes from nodesVector into hashVector
		else
			tentativeRoot = addTentativeNodes(nodesVector, tentativeRoot, tentativeDepth);//the same for tentative root
		delete [] node::hashTable;
		delete [] nodesVector;
		node::hashTable = new int [node::maxSize];
		memset(node::hashTable, 0, node::maxSize*sizeof(int));
		node::curSize = node::maxSize;
		garbageCollector(aloadmode ? 0 : 2);//in tentative mode don't clear tentative root else make "normal" garbageCollector(0)
		setStrBuf();
		displaystats();
		redisplay(0);
	}
	catch (const char *s)
	{
		ERROR(s);
		return 0;
	}
	catch (std::bad_alloc& badAlloc)
	{
		ERROR("Out of memory: loading stopped.");
		return 0;
	}
	return 1;
} 

int saveNodes(int n, int &numberOfNodes, FILE *savefl)//save nodes which are parts of root
{
	int length = ev_mode == VALENCE_DRIVEN ? 65552 : 256;
	if (n < length)
		return n;
	if (node::hashVector[n].hashNext < -2)//if we saw this node earlier it has already been saved
		return -node::hashVector[n].hashNext;//so we know its position
	int k = numberOfNodes++;//save numberOfNodes as position of node n
	int nw = saveNodes(node::hashVector[n].nw, numberOfNodes, savefl);//save subnodes
	int ne = saveNodes(node::hashVector[n].ne, numberOfNodes, savefl);
	int sw = saveNodes(node::hashVector[n].sw, numberOfNodes, savefl);
	int se = saveNodes(node::hashVector[n].se, numberOfNodes, savefl);	
	node::hashVector[n].hashNext = -k;
	fprintf(savefl, "%d %d %d %d %d\n", k, sw, se, nw, ne);
	return k;
}

void saveHashFile(FILE *savefl)
{//save hash pattern in file
	int numberOfNodes = ev_mode == VALENCE_DRIVEN ? 65552 : 256;
        fprintf(savefl, "#H %d p=%s g=%s x=%s y=%s n=%s d=%d r=%d\n", currentHashMode, /*save all information about rules, population, root node, coordinates etc.*/
        	population.toString(0, 1).c_str(), generations.toString(0, 1).c_str(), 
                rootX.toString(0, 1).c_str(), rootY.toString(0, 1).c_str(), 
                rootN.toString(0, 1).c_str(), rootDepth, numberOfNodes);
	saveNodes(root, numberOfNodes, savefl);//sae root node and it's subnodes
	memset(node::hashTable, 0, node::maxSize*sizeof(int));
	rebuildHashTable();//perhaps we made some changes into this table
	announce_and_delay("Saved");
	displaystats();
} 

int changeCellNodeBig(const bigNumber &x, const bigNumber &y, char c, cell_t newstate, int get = 0)//if (get != 0) this function is used to return current cell color (not changing cell state)
{//this function is like changeCellNode() but it can operate with tentativeRoot node
	int n = root, depth = rootDepth, tempRoot = root;
	bigNumber X(rootX), Y(rootY), N(rootN), XN(X + N), YN(Y + N);
	if (c == 'r')
		;//default
	else if (c == 't')
	{
		n = tentativeRoot;
		depth = tentativeDepth;
		X = tentativeX;
		Y = tentativeY;
		N = tentativeN;
		XN = X + N;
		YN = Y + N;
	}
	else
		return 0;//wrong char mode
	if ((x < X || y < Y || x > XN || y > YN) && newstate == 0)
		return 0;//new cell is dead and out of hash universe border		
	while (x < X || y < Y)
	{//while new cell is out of universe border we must expand our universe
		n = unite2(zero, n, zero, zero, ++depth);
		X -= N;
		Y -= N;
		N *= 2;
	}
	while (X + N <= x || Y + N <= y)
	{//same as previous 'while'
		n = unite2(zero, zero, n, zero, ++depth);
		N *= 2;
	}
	XN = X + N;
	YN = Y + N;
	int NW = 1, NE = 2, SW = 256, SE = 512, mask, minDepth = ev_mode == VALENCE_DRIVEN ? 1 : 0;
	uInt intX = 0, intY = 0, intN = 0, intXN = 0, intYN = 0, flag = 2, nodeListCounter = 0, tempListCounter = 0;
	vector<int> nodeList, tempList;
	nodeList.reserve(depth);
	tempList.reserve(depth);
	cell_t result = 0;
	while (depth-- > minDepth)
	{//recursive like descent to 2x2 node where changeable cell is located
		if (n == zero && newstate == 0)
			return 0;
		nodeList[nodeListCounter++] = n;
		if (flag == 2 && X + N < 4296987295l && Y + N < 4296987295l && X > 0 && Y > 0)
		{
			flag = 1;
			intX = X.toUint();
			intY = Y.toUint();
			intN = N.toUint();
		}
		if (flag == 1)
		{
			intN /= 2;		
			intXN = intX + intN;
			intYN = intY + intN;		
			TOVECTOR(intX, intY, intN, intXN, intYN);
		}
		else
		{
			N /= 2;
			XN = X + N;
			YN = Y + N;			
			TOVECTOR(X, Y, N, XN, YN);
		}								
	}
	BIGCODE(intX, intY);
	if (c == 't')
	{
		root = tempRoot;
		tentativeRoot = nodeState;
		tentativeDepth = depth;
		tentativeX = X;
		tentativeY = Y;
		tentativeN = N;
	}
	else//c == 'r'
	{
		rootDepth = depth;
		rootX = X;
		rootY = Y;
		rootN = N;
	}
	return result;//return cell state
}

void make_tentative_hash(const bigNumber &tentativeX, const bigNumber &tentativeY, const bigNumber &rightX, const bigNumber &bottomY)
{//move all cells in with tentativeX <= x <= rightX and tentativeY <= y <= bottomY from root node to tentativeRoot
	tentativeRoot = zero;
	tentativeN = 8;
	tentativeDepth = 3;
	for (bigNumber x = tentativeX; x <= rightX; x++)
		for (bigNumber y = tentativeY; y <= bottomY; y++)
		{		
			cell_t CurrentCell = changeCellNodeBig(x, y, 'r', 0, 0);//read and delete cell from root node
			if (CurrentCell)
				changeCellNodeBig(x, y, 't', CurrentCell, 0);//set cell in tentative node
		}
	txx = tyy = 1;//transformation variables
	tyx = txy = 0;
}

void hash_moveload(const bigNumber &x, const bigNumber &y)
{//move tentativeRoot
	tentativeX = x;
	tentativeY = y;
	redisplay(0);
}

void hash_confirmload()
{//confirm load: integrate tentativeRoot into root node
	if (root == zero && txx == tyy && txx == 1 && tyx == txy && txy == 0)//faster mode
	{//root == zero so we can directly move tentativeRoot to root
		rootX = tentativeX;
		rootY = tentativeY;
		rootN = tentativeN;
		rootDepth = tentativeDepth;
		root = tentativeRoot;
		tentativeRoot = zero;
		population = tentativePopulation;
		generations = tentativeGenerations;
		setStrBuf();
		return;
	}
	bigNumber right = tentativeX + tentativeN, bottom = tentativeY + tentativeN;
	for (bigNumber x = tentativeX; x <= right; x++)
		for (bigNumber y = tentativeY; y <= bottom; y++)
		{//for all cells from tentativeRoot: move them to root
			cell_t CurrentCell = changeCellNodeBig(x, y, 't', 0, 1);//read cell from tentative node
			if (CurrentCell)
				changeCellNodeBig(TxBIG(x, y), TyBIG(x, y), 'r', CurrentCell, 0);//TxBIG(x, y), TyBIG(x, y)
		}
	tentativeRoot = zero;//clear tentativeRoot
	setStrBuf();
}

void tentativeNodeToVideoTile(int n, const bigNumber &ix, const bigNumber &iy, bigNumber N, const bigNumber &x, const bigNumber &y)
{//display tentative node n (this function is like nodeToVideoTile() but transformation variables are also in use)
	if (N == 4)
	{
		N /= 2;
		tentativeNodeToVideoTile(node::hashVector[n].nw, ix, iy, N, x, y);
		tentativeNodeToVideoTile(node::hashVector[n].ne, ix, iy, N, x + N, y);
		tentativeNodeToVideoTile(node::hashVector[n].sw, ix, iy, N, x, y + N);
		tentativeNodeToVideoTile(node::hashVector[n].se, ix, iy, N, x + N, y + N);
		return;
	}
	if (N == 2)
	{
		if (ev_mode == VALENCE_DRIVEN)
		{
			if (getCell(n, 2))//TxBIG and TyBIG are transformation functions (like Tx and Ty)
				fb_ins_big(TxBIG(ix + x + 1, iy + y), TyBIG(ix + x + 1, iy + y), 1);
			if (getCell(n, 1))
				fb_ins_big(TxBIG(ix + x, iy + y), TyBIG(ix + x, iy + y), 1);
			if (getCell(n, 512))
				fb_ins_big(TxBIG(ix + x + 1, iy + y + 1), TyBIG(ix + x + 1, iy + y + 1),  1);
			if (getCell(n, 256))
				fb_ins_big(TxBIG(ix + x, iy + y + 1), TyBIG(ix + x, iy + y + 1), 1);
                }
                else
                {
                	if (node::hashVector[n].nw != 0)
				fb_ins_big(TxBIG(ix + x, iy + y), TyBIG(ix + x, iy + y), node::hashVector[n].nw);
			if (node::hashVector[n].sw != 0)			
				fb_ins_big(TxBIG(ix + x, iy + y + 1), TyBIG(ix + x, iy + y + 1), node::hashVector[n].sw);
			if (node::hashVector[n].ne != 0)		
				fb_ins_big(TxBIG(ix + x + 1, iy + y), TyBIG(ix + x + 1, iy + y), node::hashVector[n].ne);
			if (node::hashVector[n].se != 0)
				fb_ins_big(TxBIG(ix + x + 1, iy + y + 1), TyBIG(ix + x + 1, iy + y + 1), node::hashVector[n].se);
		}
	}
}

void tentativeNodeToVideo(int n, bigNumber N, const bigNumber &x, const bigNumber &y, int *scale)
{//display tentative node n (this function is like nodeToVideo() but transformation variables are also in use)
	if (n == zero)
		return;		
	if (!nodeOnScreen(N, TxBIG(x,y), TyBIG(x,y)))
		return;
	if (*scale < 0 && lrint(pow(2, -*scale)) >= N)
	{//improve video output at large scales
		if (!multiColor || maxstates == 2)
			fb_ins_big(TxBIG(x,y), TyBIG(x,y), 1);
		else
		{
			unsigned color = 0;
			colorFromNode(n, N, color);
			fb_ins_big(TxBIG(x,y), TyBIG(x,y), color);
		}
		return;
	}
	if (N == 8)
	{
		N /= 2;
		tentativeNodeToVideoTile(node::hashVector[n].nw, x, y, N, 0, 0);		
		tentativeNodeToVideoTile(node::hashVector[n].ne, x, y, N, N, 0);
		tentativeNodeToVideoTile(node::hashVector[n].sw, x, y, N, 0, N);		
		tentativeNodeToVideoTile(node::hashVector[n].se, x, y, N, N, N);	
	}
	else
	{
		N /= 2;		
		tentativeNodeToVideo(node::hashVector[n].nw, N, x, y, scale);
		tentativeNodeToVideo(node::hashVector[n].ne, N, x + N, y, scale);	
		tentativeNodeToVideo(node::hashVector[n].sw, N, x, y + N, scale);
		tentativeNodeToVideo(node::hashVector[n].se, N, x + N, y + N, scale);
	}
}

