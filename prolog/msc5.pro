% (C) Copyright V.Lidovski, 2002
%     Use this program freely

p(1,2,4).
p(1,3,11).
p(2,3,5).
p(2,4,2).
p(2,5,8).
p(2,6,7).
p(3,5,5).
p(3,6,3).
p(4,5,1).
p(4,6,10).
p(5,6,3).

:- dynamic(s/1, d/2, min_d/2, pd/3).

% Dijkstra's algorithm: print the shortest path between two nodes (16 LOC)

w(A,B) :- 
  retractall(pd(_,_,_)), retractall(d(_,_)), fail;
  p(X,Y,L), assertz(pd(X,Y,L)), assertz(pd(Y,X,L)), fail;
  pd(X,_,_), retractall(s(X)), assertz(s(X)), fail;
  assertz(d(A,0)), retractall(s(A)), fail;
  s(X), assertz(d(X,32000)), fail;           %32000 regarded as max integer
  pd(A,X,L), retractall(d(X,_)), assertz(d(X,L)), fail;
  w1(B).

w1(B) :- 
  retractall(min_d(_,_)), assertz(min_d(0,32000)), fail;
  s(I), d(I,L), min_d(I1,L1), L < L1, retractall(min_d(I1,L1)), 
    assertz(min_d(I,L)), fail;
  min_d(J,L), retractall(s(J)), J = B, write(L), nl, !;
  min_d(J,L), s(I), pd(J,I,L0), d(I,L1), L2 is L + L0, L2 < L1,
    retractall(d(I,L1)), assertz(d(I,L2)), fail;
  w1(B).
