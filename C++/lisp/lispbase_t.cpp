#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
using namespace std;
#include <sys/types.h>
#include <sys/stat.h>
#include "lispbase.cpp"
#include "vplsupport.cpp"

main() {
  Lisp l;
  LispMain *t1, *t2, *t3, *cp = 0;
  struct stat st;
  char fn[] = "z.vpl";
  stat(fn, &st);
  char *sb = new char[st.st_size + 1];
  ifstream fi(fn);
  fi.get(sb, st.st_size + 1, 0);
  if (fi.gcount() != st.st_size)
     cerr << fi.gcount() << '-' << st.st_size << " read file error\n";
  string is = string("(") + sb + ")";
  is.insert(is.find("VTITLE") + 7, "Edited by afmtoenc ... ");
  delete sb;
  l.lispin(is);
  t1 = l.find("DESIGNUNITS");
  double designunits = 1000.1, designsize = 10.1;
  if (t1)
     if (t1->r->l->s == "R") 
       designunits = atof(t1->r->r->l->s.c_str());
     else
       exit(1);
  t1 = l.find("DESIGNSIZE");
  if (t1)
     if (t1->r->l->s == "R") 
       designsize = atof(t1->r->r->l->s.c_str());
     else
       exit(1);
  t1 = l.find("MAPFONT");
  t3 = l.pp;
  t2 = l.find("FONTNAME", t1);
  string fontname;
  if (t2)
     fontname = t2->r->l->s;
  else
     exit(1);
  l.insert("(FONTAT R 100.0)", t1->r->r->r);
  l.insert("(FONTDSIZE R 1000.0)", t1->r->r->r->r);
  l.insert("(MAPFONT D 1)", t3);
  t2 = t3->r->l->r->r;
  l.insert("(FONTNAME " + fontname + "-sc)", t2);
  l.insert("(FONTAT R 800.0)", t2->r);
  l.insert("(FONTDSIZE R 1000.0)", t2->r->r);
  t1 = l.find("CHARACTER");
  while (t1) {
     LispMain *x = l.pp;
     int n = vplint(t1->r);
     t2 = l.find("MAP", t1);
     if (t2) {
        t3 = l.find("SELECTFONT", t2);
        if (t3) {
           t3->r->r->l->s = "2";
        }
        else
           l.insert("(SELECTFONT D 1)", t2);
     }
     if (x->r)
        t1 = l.find("CHARACTER", x->r);
     else
        break;
  }
  cout << vplout(l.sp) << endl << l.q << endl;
  l.clear();
  cout << l.q << endl;
  l.lispin("(ok ok (x z (12) y) ((z)) 12)");
  cout << l.lispout() << endl;
}
