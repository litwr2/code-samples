struct LispMain {
   LispMain *l, *r;
   string s;
};

class Lisp {
   int lispin1(const string &, LispMain *&, int);
   void find1(const string &, LispMain *);
   void clear1(LispMain *);
   string lispout1(LispMain *, int);
public:
   static LispMain *p;
   LispMain *sp, *pp;
   int q;
   Lisp(): sp(0), q(0) {}
   int lispin(const string &, LispMain *&, int);
   LispMain* find(const string &, LispMain *);
   void insert(const string &, LispMain *);
   string lispout(LispMain *);
   void clear(LispMain *);
};

LispMain *Lisp::p;

int Lisp::lispin1(const string &s, LispMain *&iv, int cp) {
   int n;
   if (s[n = s.find_first_not_of(" \n\t", cp)] == ')') {
      iv = 0;
      return n + 1;
   }
   q++, iv = new LispMain;
   cp = lispin1(s, iv->r, lispin(s, iv->l, n));
   return cp;
}

int Lisp::lispin(const string &s, LispMain *&iv = p, int cp = 0) {
   LispMain *t;
   int n;
   while (cp < s.length()) {
      if (s[cp] == '(') {
         if (s[n = s.find_first_not_of(" \n\t", cp + 1)] == ')') {
            iv = 0;
            return n + 1;
         }
         q++, iv = new LispMain;
         if (!sp) sp = iv;
         cp = lispin1(s, iv->r, lispin(s, iv->l, cp + 1));
         return cp;
      }
      else if (s[cp] == ')') {
         iv = 0;
         return cp + 1;
      }
      else if (s[cp] == ' ' || s[cp] == '\n' || s[cp] == '\t')
         cp++;
      else {
         q++, iv = new LispMain;
         if (!sp) sp = iv;
         if ((n = s.find_first_of(" ()\n\t", cp)) == string::npos)
            n = s.length();
         iv->s = s.substr(cp, n - cp);
         return n;
      }
   }
   return cp;
}

void Lisp::find1(const string &s, LispMain *pos) {
   if (pos == 0)
      return;
   if (pos->s.empty()) {
      pp = p;
      p = pos;
      find1(s, pos->l);
      find1(s, pos->r);
   }
   else if (pos->s == s)
      throw p;
}

LispMain* Lisp::find(const string &s, LispMain *pos = 0) {
   if (!pos) pos = sp;
   try {
      find1(s, pos);
      return 0;
   }
   catch (LispMain *r) {
      return r;
   }
}

void Lisp::clear1(LispMain *pos) {
   if (pos == 0)
      return;
   q--;
   if (pos->s.empty()) {
      clear1(pos->l);
      clear1(pos->r);
   }
   delete pos;
}

void Lisp::clear(LispMain *pos = 0) {
   if (!pos) pos = sp;
   clear1(pos);
   sp = 0;
}

void Lisp::insert(const string &s, LispMain *pos) {
   Lisp l;
   l.lispin(s);
   q += l.q + 1;
   p = new LispMain;
   p->l = l.sp;
   p->r = pos->r;
   pos->r = p;
}

string Lisp::lispout1(LispMain *pos, int f = 1) {
   if (pos == 0)
      return "";
   if (pos->s.empty()) {
      string s = " ";
      if (f) s = "(";
      s += lispout1(pos->l);
      s += lispout1(pos->r, 0);
      if (f) s += ")";
      return s;
   }
   return pos->s;
}

string Lisp::lispout(LispMain *pos = 0) {
   if (!pos) pos = sp;
   return lispout1(pos);
}
