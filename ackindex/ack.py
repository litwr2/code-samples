def ack(n, x, y):
  if n == 0: return y + 1
  if y == 0:
      if n == 1: return x
      if n == 2: return 0
      return 1
  return ack(n - 1, x, ack(n, x, y - 1))
print ack(1, 1, 1000)

