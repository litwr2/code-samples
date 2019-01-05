%RUN IT BY
%    time gprolog --init-goal "['fib.pro'],fib"
%OR
%    time swipl -g "['fib.pro'],fib"

fib :- fib(30,R), write(R), nl, halt.
fib(N,M) :- N<3, M=1, !;
  N1 is N-1, fib(N1,M1), N2 is N-2, fib(N2,M2), M is M1+M2.
