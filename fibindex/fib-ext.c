#include <stdio.h>
#include <stdlib.h>

long count, nl, maxnl;

long fib(int n) {
   long t;
   ++count;
   if (++nl > maxnl) maxnl = nl;
   if (n < 3) return --nl, 1;
   t = fib(n - 1) +  fib(n - 2);
   --nl;
   return t;
}

main(int argc, char *argv[]) {
    int p = atoi(argv[1]);
    printf("fib(%d) = %ld, ", p, fib(p));
    if (nl)
       puts("Error!");
    else
       printf("times=%ld depth=%ld\n", count, maxnl);
}

