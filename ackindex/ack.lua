--RUN IT BY
--   time lua ack.lua

function ack(n, x, y)
  if (n == 0) then return y + 1 end
  if (y == 0) then
    if (n == 1) then return x end
    if (n == 2) then return 0 end
    return 1
  end
  return ack(n - 1, x, ack(n, x, y - 1))
end

print(ack(5, 2, 3))
