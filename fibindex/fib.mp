%RUN IT BY
%   time mpost fib.mp
%FOR METAPOST
%OR
%   time mf fib.mp
%FOR METAFONT

def fib(expr n) =
  if n < 3:
    1
  else:
    fib(n-1) + fib(n-2)
  fi
enddef;
show fib(23)
end