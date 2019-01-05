def ack(expr n, x, y) =
  if n = 0:
     y + 1
  else: if y = 0:
     if n = 1:
        x
     else: if n = 2:
        0
     else:
        1
     fi fi
  else:
     ack(n - 1, x, ack(n, x, y - 1))
  fi fi
enddef;
show ack(1, 1, 30)
end
