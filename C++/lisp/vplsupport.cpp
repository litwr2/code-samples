string vplout1(LispMain *iv, int spc = 0, int commentline = 0, int f = 0) {
   static string spaces = "                ";
   if (iv == 0)
      return "()";
   else if (iv->s != "")
      return spaces.substr(0, f) + iv->s;
   else {
      string s = " ";
      if (iv->l->s == "VTITLE")
         s = "";
      else if (iv->l->s != "comment" && !commentline)
         s = "\n" + spaces.substr(0, spc);
      if (iv->l->s == "COMMENT")
         commentline = 1;
      s += "(" + vplout1(iv->l, spc);
      spc += 3;
      while (1) {
         iv = iv->r;
         if (iv)
            s += vplout1(iv->l, spc, commentline, 1);
         else {
            if (s[s.length() - 1] == ')') 
               s += "\n" + spaces.substr(0, spc);
               s += ")";
               return s;
         }
      }
   }
}

string vplout(LispMain *iv) {
   string s;
   LispMain *siv;
   for (; iv != 0; iv = iv->r) {
      siv = iv;
      s += vplout1(iv->l);
      iv = siv;
   }
   return s;
}

int vplint(LispMain *pos) {
   char *ep;
   if (pos->l->s == "O")
      return strtol(pos->r->l->s.c_str(), &ep, 8);
   else if (pos->l->s == "H")
      return strtol(pos->r->l->s.c_str(), &ep, 16);
   else if (pos->l->s == "D")
      return strtol(pos->r->l->s.c_str(), &ep, 10);
  else if (pos->l->s == "C")
      return pos->r->l->s.c_str()[0];
}
