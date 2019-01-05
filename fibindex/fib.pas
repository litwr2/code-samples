(* RUN IT BY
     fpc -O3 fib.pas; time ./fib
*)

function fib (n:byte): longint;
begin
  if n < 3 then
    fib := 1
  else
    fib := fib(n - 1) + fib(n - 2);
end;

begin
  writeln(fib(40));
end.
