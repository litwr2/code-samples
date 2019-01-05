#RUN IT BY
#   time tclsh fib.tcl

proc fib n {
  if $n<3 {return 1}
  expr [fib [expr $n-1]]+[fib [expr $n-2]]
}

puts [fib 25]
