function ack (n: byte; x: longint; y: longint): longint;
begin
  if n = 0 then
    ack := y + 1
  else if y = 0 then
    if n = 1 then
       ack := x
    else if n = 2 then
       ack := 0
    else
       ack := 1
  else
     ack := ack(n - 1, x, ack(n, x, y - 1));
end;

begin
  writeln(ack(5, 2, 3));
end.
