%RUN IT BY
%    time gprolog --init-goal "['ack.pro'],ack"
%OR
%    time swipl -g "['ack.pro'],ack"
ack :- ack(5,2,3,R), write(R), nl,!,halt.
ack(N,X,Y,R) :- N=0, R is Y+1,!;
                Y=0, (N=1,R=X,!;
                    N=2,R=0,!;
                     R=1,!);
Y1 is Y-1, ack(N,X,Y1,R1),
N1 is N-1, ack(N1,X,R1,R).
