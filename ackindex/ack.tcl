proc ack {n x y} {
  if $n==0 {return [expr $y+1]}
  if $y==0 {
     if $n==1 {return $x}
     if $n==2 {return 0}
     return 1
  }
  ack [expr $n-1] $x [ack $n $x [expr $y-1]]
}

puts [ack 1 1 999]
