%RUN IT BY
%   erlc ack.erl; time echo 'ack:start().'|erl

-module(ack).
-export([start/0]).

ack(0, _, Y) -> Y + 1;
ack(1, X, 0) -> X;
ack(2, _, 0) -> 0;
ack(_, _, 0) -> 1;
ack(N, X, Y) -> ack(N - 1, X, ack(N, X, Y - 1)).

start() -> ack(5, 2, 3).
