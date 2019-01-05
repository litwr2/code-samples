//RUN IT BY
//   time dino fib.d

func fib(n) {
   if (n < 3) return 1;
   return fib(n - 1) + fib(n - 2);
}
putln (fib(33));