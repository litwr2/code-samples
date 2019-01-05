%RUN IT BY
%   erlc fib.erl; time echo 'fib:start().'|erl

-module(fib).
-export([start/0]).

fib(0) -> 0;
fib(1) -> 1;
fib(2) -> 1;
fib(N) -> fib(N - 1) + fib(N - 2).

start() -> fib(36).