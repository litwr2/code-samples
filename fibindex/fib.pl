#RUN IT BY
#   time perl fib.pl

sub fib {
  my $n = shift;
  if ($n < 3) {return 1;}
  return fib($n-1) + fib($n-2);
}

print fib(30), "\n"