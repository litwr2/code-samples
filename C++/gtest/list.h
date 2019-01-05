#ifndef _LIST
#define _LIST

#include <iostream>
#include <cstdlib>
using namespace std;

template <class T>
struct ListElement {
  T data;
  ListElement *next;
};

struct Range { 
  int n;
  Range(int i) {n = i;}
};

template <class T>
class List {
  ListElement<T> *pcurrent, *pfirst;
public:
  List () : pfirst(0) {}
  List (const List&);
  ~List (); 
  T& operator [] (int);
  List& operator = (List<T>);
  T operator == (List<T>&);
  T operator != (List<T>& list) {return ! (*this == list);}
  List copy (int, int);
  void append (List<T>&);
  void insert (T);
  void del ();
  void tofirst () {pcurrent = pfirst;}
  void tonext () {if (pfirst != 0) pcurrent = pcurrent->next;}
  void toend ();
  void sort ();
  void reverse ();
  void rember (T);
  void remberall (T);
  int find (T);
  T& getdata () const {return pcurrent->data;}
  int null () const {return pfirst == 0;}
  int length ();
  int eol ();
};  

template <class T>
List<T> :: List (const List<T>& list) : pfirst(0)  {
  ListElement<T> *ptemp = list.pfirst, *ptemp2;
  if (ptemp != 0)
	 do {
		ptemp2 = new ListElement<T>;
		ptemp2->data = ptemp->data;
		if (pfirst == 0) {
		  pfirst = pcurrent = ptemp2;
	pfirst->next = pfirst;
		}
		else {
		  ptemp2->next = pcurrent->next;
		  pcurrent = pcurrent->next = ptemp2;
		}
		ptemp = ptemp->next;
	 }
	 while (ptemp != list.pfirst);
}

template <class T>
List<T> :: ~List () {
  ListElement<T> *ptemp = pfirst;
  if (pfirst != 0) {
	 pcurrent = pfirst->next;
	 while (pcurrent != ptemp) {
		delete pfirst;
		pfirst = pcurrent;
		pcurrent = pfirst->next;
	 }
	 delete pfirst;
	 pfirst = 0;
  }
}

template <class T>
void List<T> :: toend () {
  length();
}

template <class T>
List<T>& List<T> :: operator = (List<T> list) {
  this->~List();
  append(list);
  return *this;
}

template <class T>
T& List<T> :: operator [] (int n) {
  int count = 1;
  if (pfirst == 0 || n < 1)
	 throw Range (n);
  pcurrent = pfirst;
  while (count < n) {
	 pcurrent = pcurrent->next;
	 count++;
  }
  return pcurrent->data;
}

template <class T>
int List<T> :: length () {
  int count = 1;
  if (pfirst == 0)
	 return 0;
  pcurrent = pfirst;
  while (pcurrent->next != pfirst) {
	 pcurrent = pcurrent->next;
	 count++;
  }
  return count;
}

template <class T>
T List<T> :: operator == (List<T>& list) {
  if (pfirst == 0)
	 return 1;
  pcurrent = pfirst;
  list.pcurrent = list.pfirst;
  do {
	 if (pcurrent->data != list.pcurrent->data)
		return 0;
	 pcurrent = pcurrent->next;
	 list.pcurrent = list.pcurrent->next;
  }
  while (pcurrent != pfirst);
  return 1;
}

template <class T>
int List<T> :: eol () {
  if (pcurrent->next == pfirst)
	 return 1;
  return 0;
}

template <class T>
List<T> List<T> :: copy (int pos, int len) {
  List<T> A;
  (*this)[pos];
  for (int i = 0; i < len; i++) {
	 A.insert(pcurrent->data);
	 pcurrent = pcurrent->next;
  }
  return A;
}

template <class T>
void List<T> :: sort () {
  ListElement<T> *ptemp = pfirst, *ptemp2;
  int min, tpos = 1;
  if (pfirst == 0)
	 return;
  do {
	 pcurrent = ptemp;
	 min = pcurrent->data;
	 do {
		pcurrent = pcurrent->next;
		if (min > pcurrent->data) {
		  min = pcurrent->data;
		  ptemp2 = pcurrent;
		}
	 }
	 while (pcurrent->next != pfirst);
	 if (min != ptemp->data) {
		min = ptemp->data;
		ptemp->data = ptemp2->data;
		ptemp2->data = min;
	 }
	 ptemp = ptemp->next;
	 tpos++;
  }
  while (ptemp->next != pfirst);
}

template <class T>
void List<T> :: reverse () {
  if (pfirst != 0) {
	 List<T> A(*this);
	 this->~List ();
	 pfirst = 0;
	 while (A.pfirst != 0) {
		tofirst();
		insert(A.pfirst->data);
		A.tofirst();
		A.del();
	 }
	 pfirst = pfirst->next;
  }
}

template <class T>
void List<T> :: append (List<T>& A) {
  toend();
  if(A.pfirst != 0) {
	 A.tofirst();
	 while (A.pcurrent->next != A.pfirst) {
		insert(A.pcurrent->data);
		A.pcurrent = A.pcurrent->next;
	 }
	 insert(A.pcurrent->data);
  }
}

template <class T>
void List<T> :: insert (T n) {
  ListElement<T>* ptemp = new ListElement<T>;
  ptemp->data = n;
  if (pfirst == 0) {
	 pfirst = pcurrent = ptemp;
  }
  else
	 ptemp->next = pcurrent->next;
  pcurrent = pcurrent->next = ptemp;
}

template <class T>
void List<T> :: del () {
  ListElement<T> *ptemp = pfirst;
  if (pfirst == 0)
	 return;
  for(; ptemp->next != pcurrent; ptemp = ptemp->next);
  if (pfirst->next == pfirst)
	 pfirst = 0;
  if (pcurrent == pfirst)
	 if (pfirst != 0)
		pfirst = pcurrent->next;
  ptemp->next = pcurrent->next;
  delete pcurrent;
  pcurrent = ptemp;
}

template <class T>
int List<T> :: find (T n) {
  int count = 1;
  pcurrent = pfirst;
  if (pfirst == 0)
	 return 0;
  do {
	 if (pcurrent->data == n)
		return count;
	 count++;
	 pcurrent = pcurrent->next;
  }
  while (pcurrent != pfirst);
  return 0;
}

template <class T>
void List<T> :: rember (T n) {
  if (find(n))
	 del();
}

template <class T>
void List<T> :: remberall (T n) {
  while (find(n))
	 del();
}

template <class T>
ostream& operator << (ostream& out, List<T>& list) {
  if (! list.null ()) {
	 list.tofirst ();
	 while (! list.eol ()) {
		out << list.getdata () << ' ';
		list.tonext ();
	 }
	 out << list.getdata ();
  }
  cout << endl;
  return out;
}
#endif /*!_LIST*/
