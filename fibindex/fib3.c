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

it is slightly (about 10%) faster than fib.c but it is not exactly matched to fibindex rules
*/

#include <stdio.h>
#define bestint long  //64 bits

bestint r;
void fib (bestint n) {
  if (n < 3)
     ++r;
  else {
     fib(n - 1);
     fib(n - 2);
  }
}

main() {
  int k = 41;
  fib(k);
  printf("%d %ld\n", k, r);
}
