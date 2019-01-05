(* RUN IT BY
    ack -mlinux386 -O4 -o fib fib.p; time ./fib
*)
program fibo(output);
function fib(n: integer): integer;
   begin
      if n < 3 then
         fib := 1
      else
         fib := fib(n - 1) + fib(n - 2)
   end;
begin
   writeln(fib(40))
end.
