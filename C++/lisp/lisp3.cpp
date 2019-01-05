#include <iostream>
#include <fstream>
#include <string>
#include <cmath>
using namespace std;

struct LispMain {
  LispMain *left, *right;
  string s;
};

inline LispMain* car(LispMain *p) {
  if (p == 0)
    throw "impossible car";
  else
    return p->left;
}

inline LispMain* cdr(LispMain *p) {
  if (p == 0)
    throw "impossible cdr";
  else
    return p->right;
}

inline LispMain* cons(LispMain *p1, LispMain *p2) {
  LispMain *t = new LispMain;
  t->left = p1;
  t->right = p2;
  return t;
}

inline int atom(LispMain *p) {
  return  p->s != "";
}

inline int eql(LispMain *p1, LispMain *p2) {
  return p1 == p2 || p1->s == p2->s && p1->s != "";
}

inline LispMain* last(LispMain *p) {
  while (p->right != 0) 
    p = p->right;
  return p;
}

LispMain* nth(int n, LispMain *l) {
  if (n == 0)
    return car(l);
  return nth(n - 1, cdr(l));
}

LispMain *append(LispMain *p1, LispMain *p2) {
  if (p1 == 0)
    return p2;
  return cons(car(p1), append(cdr(p1), p2));
}

inline LispMain* fappend(LispMain *p1, LispMain *p2) {
  if (p1 == 0)
    return p2;
  last(p1)->right = p2;
  return p1;
}

int length(LispMain *l) {
  int n = 0;
  while (l != 0) {
    n++;
    l = cdr(l);
  }
  return n;
}

LispMain *globx;

LispMain* mapcar2(LispMain* (*f)(LispMain*), LispMain *l) {
  if (l == 0)
    return 0;
  return cons((*f)(car(l)), mapcar2(f, cdr(l)));
}

LispMain* mapcar2f(LispMain* (*f)(LispMain*), LispMain *l) {
  LispMain *r = 0, *t;
  if (l != 0) {
    t = r = cons((*f)(car(l)), 0);
    l = cdr(l);
  }
  while (l != 0) {
    t = t->right = cons((*f)(car(l)), 0);
    l = cdr(l);
  }
  return r;
}

inline LispMain* lambda1(LispMain* z) {
  return cons(globx, z);
}

inline LispMain* lambda2(LispMain* u) {
  return fappend(u, mapcar2f(lambda1, u));
}

LispMain* powerset(LispMain *l) {
  if (l == 0)
    return cons(0,0);
  globx = car(l);
  return lambda2(powerset(cdr(l)));
}

inline LispMain *lambda(LispMain *z) {
  return cons(globx, z);
}

LispMain *powerset3(LispMain *l) {
   LispMain *x = cons(0,0);
   while (l != 0) {
     globx = car(l);
     x = fappend(x, mapcar2f(lambda, x));
     l = cdr(l);
   }
   return x;
}

LispMain* powerset2(LispMain* l1) {
  LispMain *t, *l2 = 0, *l3;
  int p = rint(pow(2, length(l1))) - 1, t1, t2;
  while (p != 0) {
    t1 = 0;
    t2 = p;
    l3 = 0;
    while (t2 != 0) {
      if (t2%2 == 1)
        l3 = cons(nth(t1, l1), l3);
      t2 = t2/2;
      t1++;
    }
    l2 = cons(l3, l2);
    p--;
  }
  return cons(0, l2);
}

int lispin(string &, LispMain *&, LispMain *&, int);

int lispinr(string &s, LispMain *&iv, LispMain *&sp, int cp) {
  int n;
  if (s[n = s.find_first_not_of(" \n\t", cp)] == ')') {
    iv = 0;
    return n + 1;
  }
  iv = new LispMain;
  cp = lispinr(s, iv->right, sp, lispin(s, iv->left, sp, n));
  return cp;
}

int lispin(string &s, LispMain *&iv, LispMain *&sp, int cp = 0) {
  LispMain *t;
  int n;
  while (cp < s.length()) {
    if (s[cp] == '(') {
       if (s[n = s.find_first_not_of(" \n\t", cp + 1)] == ')') {
          iv = 0;
          return n + 1;
       }
       iv = new LispMain;
       if (!sp)
          sp = iv;
       cp = lispinr(s, iv->right, sp, lispin(s, iv->left, sp, cp + 1));
       return cp;
    }
    else if (s[cp] == ')') {
       iv = 0;
       return cp + 1;
    }
    else if (s[cp] == ' ' || s[cp] == '\n' || s[cp] == '\t')
       cp++;
    else {
       iv = new LispMain;
       if (!sp)
          sp = iv;
       if ((n = s.find_first_of(" ()\n\t", cp)) == string::npos)
         n = s.length();
       iv->s = s.substr(cp, n - cp);
       return n;
    }
  }
  return cp;
}

string lispout(LispMain *iv, string f = "") {
  if (iv == 0)
    return "()";
  else if (iv->s != "")
    return f + iv->s;
  else {
    string s = f + "(" + lispout(iv->left);
    while (1) {
      iv = iv->right;
      if (iv)
        s += lispout(iv->left, " ");
      else {
        s += ")";
        return s;
      }
    }
  }
}


main() {
  LispMain *p, *sp1 = 0, *sp2 = 0, *sp3 = 0, *sp4 = 0;
  string s1 = "(a ((())) (b) (c d) (f (() e h ()) (xx 8)))", s2 = "(w x y z)",
    s3 = "(1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24)",
    s4 = "(1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22)";
  /*
  char *sb new char[100000];
  ifstream fi("x.vpl");
  fi.get(sb, sizeof(sb), 0);
  cout << fi.gcount() << endl;
  string is = string("(") + sb + ")";
  delete sb;
  lispin(is, p, sp4);
  */
  //lispin(s1, p, sp1);
  //lispin(s2, p, sp2);
  lispin(s3, p, sp3);
  //cout << sp1->right->right->right->left->right->left->left->s << endl;
  //cout << sp3->right->right << endl;
  //cout << lispout(sp3) << endl;
  //sp4 = powerset3(sp3);
  //cout << length(sp3) << ' ' << length(sp4) << endl;
  cout << length(powerset3(sp3)) << endl;
  //cout << lispout(sp4) << endl;
}
