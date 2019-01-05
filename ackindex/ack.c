#include <stdio.h>

long ack(long n, long x, long y) {
   if (n == 0) return y + 1;
   if (y == 0)
      switch (n) {
         case 1: return x;
         case 2: return 0;
         default: return 1;
      }
   return ack(n - 1, x, ack(n, x, y - 1));
}

main() {
   printf("%ld\n", ack(5, 2, 3));
}

