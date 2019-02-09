#include <iostream>
#include <memory>
#include <map>
#include <bitset>
#include <vector>
using namespace std;
unsigned minLinkSize(unsigned sz) {
   return sz < sizeof(void*) ? sizeof(void*) : sz;
}
class Pool {
   struct Link {Link *next;};
   static const unsigned chunksz = 8*1024;
   struct Chunk {
      static const unsigned size = chunksz - sizeof(Chunk*); //page alignment, 256 bytes
      Chunk *next;
      char mem[size];
   } *chunks;
   Link *head;  //a pointer to the 1st free memory element
   void grow();    //pool increase
public:
   unsigned occupied; //should be private
   const unsigned esz;
   Pool(unsigned);
   ~Pool();
   void* alloc();  //allocate memory for an element
   void free(void*);  //to return element back to the free memory poo
   void print();  //prints pool
};
void* Pool::alloc() {
   occupied++;
   if (head == 0) grow();
   Link *p = head;
   head = p->next;
   return p;
}
void Pool::free(void *b) {
   occupied--;
   Link *p = static_cast<Link*>(b);
   p->next = head;
   head = p;
}
Pool::Pool(unsigned sz) : esz(sz) {
cout << sz << " Pool constructor\n";
   occupied = 0;
   head = 0;
   chunks = 0;
}
Pool::~Pool() {
cout << esz << " Pool destructor\n";
   if (occupied) { cout << occupied << " return\n";return;}
   Chunk *p = chunks;
   while (p) {
      Chunk *q = p;
      p = p->next;
      delete q;
   }
}
void Pool::grow() {
   Chunk *p = new Chunk;
   p->next = chunks;
   chunks = p;
   const unsigned noe = Chunk::size/esz; //number of elements in a chunk
   char *start = p->mem, *last = start + (noe - 1)*esz;
   for (char *p = start; p < last; p += esz)
      ((Link*)p)->next = (Link*)(p + esz);
   ((Link*)last)->next = 0;
   head = (Link*)start;
}
void Pool::print() {
   Chunk *p = chunks;
   vector<Link*> v;
   unsigned n = 0;
   cout << "\nThe pool map for the element size = " << esz << endl;
   if (p == 0) {
       cout << "0 bytes allocated\n\n";
       return;
   }
   else
      while (p) {
         v.push_back((Link*)p->mem);
         p = p->next;
         n++;
      }
   const unsigned bxs = Chunk::size/sizeof(Link*) - 1;
   bitset<bxs> bx[n];
   for (int i = 0; i < n; i++) bx[i].set();
   if (head == 0) {
      cout << "all (" << n << ") chunks are allocated\n\n";
      return;
   }
   else {
      Link *q = head;
      while (q) {
         unsigned long a = (unsigned long)q;
         Link* r = v[n - 1];
         for (unsigned i = 1; i < n; i++) 
            if (a > (unsigned long)(v[i - 1]) && a < (unsigned long)(v[i])) {
               r = v[i - 1];
               break;
            }
         unsigned xsz = esz/sizeof(Link*) + esz%sizeof(Link*);
         bx[(q - r)/bxs/xsz].reset((q - r)/xsz%bxs);
         q = q->next;
      }
   }
   for (unsigned i = 0; i < n; i++) {
      unsigned cft = 0, cbt = 0;
      cout << "Chunk #" << i << endl;
      for (int k = 0; k < Chunk::size/esz; k++) {
         if (bx[(k + i*(Chunk::size/esz))/bxs].test((k + i*(Chunk::size/esz))%bxs)) {
            if (cft) cout << cft << " link(s) free\n";
            cft = 0;
            cbt++;
         }
         else {
            if (cbt) cout << cbt << " link(s) occupied\n";
            cbt = 0;
            cft++;
         }
      }
      if (cft) cout << cft << " link(s) free\n";
      if (cbt) cout << cbt << " link(s) occupied\n";
   }
   cout << endl;
}
map<unsigned, Pool*> pools;
map<void*, unsigned> map_of_allocators;
template<class T> struct Pool_alloc : allocator<T> {
   template<class U> struct rebind {
      typedef Pool_alloc<U> other;
   };
   template<class U> Pool_alloc(const Pool_alloc<U>&) {}
   Pool_alloc() {
      unsigned sz = minLinkSize(sizeof(T));
      if (pools.find(sz) == pools.end()) {
         pools[sz] = new Pool(sz);
         map_of_allocators[this] = sz;
      }
   }
   ~Pool_alloc() {  //complete (?) vector-interface
      map<void*, unsigned>::iterator p;
      unsigned sz = 0;
      if ((p  = map_of_allocators.find(this)) == map_of_allocators.end())
         goto possibleclearall;
      else if (pools[sz = p->second]->occupied)
         return;
      delete pools[sz];
      pools.erase(sz);
      map_of_allocators.erase(this);
possibleclearall:
      for (map<unsigned, Pool*>::iterator i = pools.begin(); i != pools.end(); i++)
         if (i->second->occupied)
            return;
      for (map<unsigned, Pool*>::iterator i = pools.begin(); i != pools.end(); i++)
         delete pools[i->first];
      pools.clear();
      map_of_allocators.clear();
   }
   T* allocate(size_t, void*);
   void deallocate(T*, size_t);
};
template<class T> T* Pool_alloc<T>::allocate(size_t n, void* = 0) {
   T* p;
   if (n == 1)
      p = static_cast<T*>(pools[minLinkSize(sizeof(T))]->alloc());
   else
      p = static_cast<T*>(allocator<T>::allocate(n));    //STL level
      //p = static_cast<T*>(operator new (sizeof(T)*n));  //OS level
   return p;
}
template<class T> void Pool_alloc<T>::deallocate(T* p, size_t n) {
   if (n == 1)
      pools[minLinkSize(sizeof(T))]->free(p);
   else
      allocator<T>::deallocate(p, n);  //STL level
      //operator delete(p);  //OS level
}
main() {
   map<int, int, less<int>, Pool_alloc<pair<int, int> > > m;
   map<long, long, less<long>, Pool_alloc<pair<long, long> > > m2;
   map<long, pair<short, short>, less<long>, Pool_alloc<pair<long, pair<short,short> > > > m3;
   m.insert(pair<int,int>(7, 8));
   for (int i(0); i < 8; ++i)
      m[i*i] = 2*i;
   m.erase(7);
   m.erase(36);
   m[5] = 88;
   m2[77] = 1001;
   m3[5] = pair<short, short>(55, 11);
   vector<int, Pool_alloc<int> > v(1), v1(1), v2(100), v5;
   vector<pair<long,int>, Pool_alloc<pair<long,int> > > v3(1);
   v5.push_back(14);
   v2[7] = 2;
   cout << "Pools: ";
   for (map<unsigned, Pool*>::iterator i = pools.begin(); i != pools.end(); i++)
      cout << i->first << ' ';
   cout << endl;
   for (map<unsigned, Pool*>::iterator i = pools.begin(); i != pools.end(); i++)
      i->second->print();
}
