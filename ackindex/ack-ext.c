#include <stdio.h>

long count, nl, maxnl;

long ack(long n, long x, long y) {
   long t;
   ++count;
   if (++nl > maxnl) maxnl = nl;
   if (n == 0) return --nl, y + 1;
   if (y == 0) {
      --nl;
      switch (n) {
         case 1: return x;
         case 2: return 0;
         default: return 1;
      }
   }
   t = ack(n - 1, x, ack(n, x, y - 1));
   --nl;
   return t;
}

main(int argc, char *argv[]) {
    int p1 = atoi(argv[1]), p2 = atoi(argv[2]), p3 = atoi(argv[3]);
    printf("ack(%d,%d,%d) = %ld, ", p1, p2, p3, ack(p1, p2, p3));
    if (nl)
       puts("Error!");
    else
       printf("times=%ld depth=%ld\n", count, maxnl);
}

