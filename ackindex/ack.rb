def ack(n,x,y)
  if n == 0
      y + 1
  elsif y == 0
      if n == 1
          x
      elsif n == 2
          0
      else
          1
      end
  else
      ack(n - 1, x, ack(n, x, y - 1))
  end
end
print ack(1, 1, 6600), "\n"

