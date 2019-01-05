#RUN IT BY
#   time perl ack.pl

sub ack {
  my $n = shift;
  my $x = shift;
  my $y = shift;
  if ($n == 0) {return $y + 1;}
  if ($y == 0) {
     if ($n == 1) {return $x;}
     if ($n == 2) {return 0;}
     return 1;
  }
  return ack($n-1, $x, ack($n, $x, $y - 1));
}

print ack(5, 2, 3), "\n"
