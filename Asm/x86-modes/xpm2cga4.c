/* XPM 4 colors to CGA mode 4 direct to video memory converter */
/* USAGE: xpm2cga4 <INFILE */
#include <stdio.h>
#include <string.h>
int main() {
   char s[400], colors[256];
   int i, c = 0, p, k, x, y, n;
   FILE *f = fopen("pic.cga4", "w");
   while ((c = getc(stdin)) != '"');
   ungetc(c, stdin);
   p = scanf("\"%d %d %d %d%*s\n", &x, &y, &n, &k);
   if (p != 4 || x != 320 || y != 200 || n != 5 || k != 1) return 1;
   gets(s);
   for (i = 1; i < n; i++) {
      gets(s);
      colors[i - 1] = s[1];
   }
   for (i = 0; i < y; i++) {
      p = scanf("\"%s\n", s);
      s[x] = 0;
      if (p == 0 || strlen(s) < x) return 2;
      if (i%2)
         fseek(f, 7960 + i*40, SEEK_SET);
      else
         fseek(f, i*40, SEEK_SET);
      for (k = 0; k < x; k += 4) {
         c = 0;
         for (p = 0; p < 4; p++)
            if (s[k + p] == colors[2])
               c |= 2 << (3 - p)*2;
            else if (s[k + p] == colors[0])
               c |= 1 << (3 - p)*2;
            else if (s[k + p] == colors[1])
               c |= 3 << (3 - p)*2;
         fwrite(&c, 1, 1, f);
      }
   }
   fclose(f);
   return 0;
}

