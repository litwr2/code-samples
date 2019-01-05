#include <iostream>
#include <map>
#include <cstdlib>
using namespace std;
#include "lisp4vpl.cpp"

class xstring: public string {
public:
  //xstring(char* s) {*this = s;}
  xstring(const string& s) {
    *this = s;
  }
  operator int () {
    return atoi(this->c_str());
  }
};

template<class TK, class TV> 
  class xmap: public map<TK, TV> {
  public:
    xmap() {}
    xmap(const xstring &);
    xmap<TK,TV>& operator=(const xstring&);
    xmap<TK,TV>& operator+=(const xstring&);
  };

template<class TK, class TV> 
  xmap<TK, TV>::xmap(const xstring &s) {
    LispMain *p, *sp = 0;
    lispin(s, p, sp);
    for (p = sp; p != 0; p = p->right)
      (*this)[xstring(TK(p->left->left->s))] = TV(xstring(p->left->right->left->s));
    //clearlisp(sp);
  }

template<class TK, class TV> 
  xmap<TK,TV>& xmap<TK, TV>::operator+=(const xstring &s) {
    LispMain *p, *sp = 0;
    lispin(s, p, sp);
    for (p = sp; p != 0; p = p->right)
      (*this)[atoi(p->left->left->s.c_str())] = atoi(p->left->right->left->s.c_str());
    //clearlisp(sp);
    return *this;
  }

template<class TK, class TV> 
  xmap<TK,TV>& xmap<TK, TV>::operator=(const xstring &s) {
    this->clear();
    return operator+=(s);
  }


xmap<int, int> t(xstring("((1 11) (2 4) (7 8) (711 114))")), x;

main() {
  t[1] = 2;
  t[3] = 5;
  for(xmap<int,int>::iterator i = t.begin(); i != t.end(); ++i)
     cout << i->first << ' ' << i->second << endl;
  x += string("((77 55))");
  t = x;
  for(xmap<int,int>::iterator i = t.begin(); i != t.end(); ++i)
     cout << i->first << ' ' << i->second << endl;
}



