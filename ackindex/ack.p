program ack(output);
function ack (n: integer; x: integer; y: integer): integer;
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
  writeln(ack(1, 1, 10000));
end.
