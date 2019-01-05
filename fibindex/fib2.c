/*RUN IT BY (GNU compiler)
   gcc -O5  FN.c; time ./a.out
OR (it is without optimisation)
   gcc FN.c; time ./a.out
OR (Intel compiler)
   icc -fast FN.c; time ./a.out
OR (LLVM compiler)
   clang -O3 FN.c; time ./a.out
OR (Amsterdam Compiler Kit)
   ack -mlinux386 -O4 -o fib fib.c; time ./fib

This is an equivalent to fib.c
*/

#include <stdio.h>
#define bestint long  //64 bits

bestint fib (bestint n) {
  if (n < 3) return 1;
  return fib(n - 1) + fib(n - 2);
}

main() {
  int k = 41;
  printf("%d %ld\n", k, fib(k));
}
