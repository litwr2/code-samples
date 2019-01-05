'RUN IT AT LIBRE OFFICE, OPEN OFFICE, OR MICROSOFT OFFICE

function fib(n as long)
  if n < 3 then
    fib = 1
  else
    fib = fib(n-2)+fib(n-1)
  end if
end function

sub Main
  dim t1 as long, f as long
  t1 = timer
  f = fib(32)
  print timer-t1
  print f
end sub

