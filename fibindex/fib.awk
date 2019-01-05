#RUN IT BY
#   time gawk -f fib.awk
#OR
#   time mawk -f fib.awk
#OR
#   time busybox awk -f fib.awk

function fib (n) {
  if (n < 3) return 1;
  return fib(n - 1) + fib(n - 2);
}

BEGIN {print fib(31)}
