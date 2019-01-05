/*
 * Multirules Generator Copyright 2013 Vladimir Lidovski vol.litwr@gmail.com
 * $Id: gen-multirules.cpp 310 2014-02-11 07:40:09Z litwr $
 *
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

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#define RULESMAX 12
#define BADSTATE 0xff
#define btrans(n) ((unsigned char(*)[n][n][n][n][n][n][n][n])trans)

char *trans;

unsigned long power(int n, int k) {
   int f = 1;
   while (k--) f *= n;
   return f;
}

int set_sums(unsigned long n, int q, int *ra) {
   int i, sum = 0;
   for (i = 1; i <= q; i++)
     ra[i] = 0;
   for (i = 1; i <= 8; i++) {
     ra[n%(q + 1)]++;
     n /= q + 1;
   }
   return sum <= 8;
}

int find_nz(int q, int *ra) {
   int i = 1;
   for (; i <= q; i++)
      if (ra[i]) return i;
   return 0;
}

void convcoors(unsigned int m, int q, int *ra) {
   int i;
   for (i = 0; i < 8; i++) {
      ra[7 - i] = m%(q + 1);
      m /= q + 1;
   }
}

int addcoors(int s, int *r, int e, int q) {
   int i;
   if (btrans(q + 1)[s][r[0]][r[1]][r[2]][r[3]]
         [r[4]][r[5]][r[6]][r[7]] != BADSTATE) return 0;
   for (i = 0; i < 8; i++)
      btrans(q + 1)[s][r[i%8]] [r[(i + 1)%8]][r[(i + 2)%8]][r[(i + 3)%8]]
         [r[(i + 4)%8]][r[(i + 5)%8]][r[(i + 6)%8]][r[(i + 7)%8]] = e;
   return 1;
}

void rule2out(int s, unsigned long m, int e, int q) {
   int ra[8], i;
   char ro[11];
   convcoors(m, q, ra);
   if (!addcoors(s, ra, e, q)) return;
   ro[0] = s + '0';
   ro[9] = e + '0';
   ro[10] = 0;
   for (i = 0; i < 8; i++)
      ro[i + 1] = ra[i] <= 9 ? ra[i] + '0' : ra[i] + 'A';
   puts(ro);
}

int parse_rule(char *rule, int *rb, int *rs) {
   char *p;
   if (!strcasecmp(rule, "life"))
      *rb = 8, *rs = 12;
   else if (!strcasecmp(rule, "seeds"))
      *rb = 4, *rs = 0;
   else if ((p = strchr(rule, 's')) || (p = strchr(rule, 'S')) || strchr(rule, 'b') || strchr(rule, 'B')) {
      *rb = *rs = 0;
      if (p++)
         while (*p >= '0' && *p <= '9')
            *rs |= 1 << *p++ - '0';
      (p = strchr(rule, 'b')) || (p = strchr(rule, 'B'));
      if (p++)
         while (*p >= '0' && *p <= '9')
            *rb |= 1 << *p++ - '0';
   }
   else if (p = strchr(rule, '/')) {
      *rb = *rs = 0;
      while (*rule >= '0' && *rule <= '9')
         *rs |= 1 << *rule++ - '0';
      if (*rule == '/') {
         rule++;
         while (*rule >= '0' && *rule <= '9')
            *rb |= 1 << *rule++ - '0';
      }
   }
   else
      return 0;
   if (*rb & 1) return 0; /* b0 is not supported */
   return 1;
}

int parse_rules(char *rules, int *rb, int *rs) {
   char *p, *s = rules;
   int i = 0;
   while (p = strchr(rules, ',')) {
      rules = p + 1;
      *p = '\0';
      i++;
      if (!parse_rule(s, &rb[i], &rs[i])) return 0;
      s = rules;
   }
   i++;
   if (!parse_rule(s, &rb[i], &rs[i])) return 0;
   return i;
}

int main(int argc, char *argv[]) {
   int j, mode, rules_total, rules_born[RULESMAX], rules_stay[RULESMAX], 
      sum[RULESMAX], born[RULESMAX], stay[RULESMAX], qb, qs, k;
   unsigned long i, limit;
   if (argc != 3) {
      fputs("USAGE: gen-multirules COLLISION-ACTION RULES-LIST\n", stderr);
      fputs("       gen-multirules 2 life,life,seeds,23/36,B12S5\n", stderr);
      fputs("       gen-multirules 0 S012345678/B3\n", stderr);
      fputs("COLLISION-ACTION  values:\n", stderr);
      fputs("       0 - mutual destruction of newborn cells, old cell has priority\n", stderr);
      fputs("       1 - mutual destruction of newborn cells, new cell has priority\n", stderr);
      fputs("       2 - left rule has priority for new cell, old cell has priority\n", stderr);
      fputs("       3 - left rule has priority for new cell, new cell has priority\n", stderr);
      return 1;
   }
   printf("# %s\n", argv[2]);
   if ((rules_total = parse_rules(argv[2], rules_born, rules_stay)) == 0) {
      fputs("Wrong rules-list format\n", stderr);
      return 2;
   }
   if (rules_total >= RULESMAX) {
      fputs("Too many rules\n", stderr);
      return 3;
   }
   if ((mode = atoi(argv[1])) > 3) {
      fputs("Wrong mode\n", stderr);
      return 4;
   }
   limit = power(rules_total + 1, 8);
   trans = (char*)malloc(limit*(rules_total + 1));
   memset(trans, BADSTATE, limit*(rules_total + 1));
   printf("Moore\nstates %d\n", rules_total + 1);
   for (i = 0; i < limit; i++)
      if (set_sums(i, rules_total, sum)) {
         qb = qs = 0;
         for (j = 1; j <= rules_total; j++) {
            qb += born[j] = (rules_born[j] & (1 << sum[j])) != 0;
            qs += stay[j] = rules_stay[j] & (1 << sum[j]);
         }
         k = find_nz(rules_total, born);
         switch (mode) {
         case 0:  /* mutual destruction, keep previous */
            if (qb == 1 && k)
               rule2out(0, i, k, rules_total);
            for (j = 1; j <= rules_total; j++)
               if (!stay[j])
                  if (qb == 1 && k != j)
                     rule2out(j, i, k, rules_total);
                  else
                     rule2out(j, i, 0, rules_total);
            break;
         case 1:  /* mutual destruction, new cell replaces old */
            if (qb == 1 && k)
               rule2out(0, i, k, rules_total);
            for (j = 1; j <= rules_total; j++)
               if (!stay[j])
                  if (qb == 1 && k != j)
                     rule2out(j, i, k, rules_total);
                  else
                     rule2out(j, i, 0, rules_total);
               else
                  if (qb == 1 && k != j)
                     rule2out(j, i, k, rules_total);
            break;
         case 2:  /* first rule has priority, keep previous */
            if (qb >= 1 && k)
               rule2out(0, i, k, rules_total);
            for (j = 1; j <= rules_total; j++)
               if (!stay[j])
                  if (qb >= 1 && k != j)
                     rule2out(j, i, k, rules_total);
                  else
                     rule2out(j, i, 0, rules_total);
            break;
         case 3:  /* first rule has priority, new cell replaces old */
            if (qb >= 1 && k)
               rule2out(0, i, k, rules_total);
            for (j = 1; j <= rules_total; j++)
               if (!stay[j])
                  if (qb >= 1 && k != j)
                     rule2out(j, i, k, rules_total);
                  else
                     rule2out(j, i, 0, rules_total);
               else
                  if (qb >= 1 && k != j)
                     rule2out(j, i, k, rules_total);
            break;
         }
      }
   return 0;
}

