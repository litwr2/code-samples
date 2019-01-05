ack 0 x y = y + 1
ack 1 x 0 = x
ack 2 x 0 = 0
ack n x 0 = 1
ack n x y = ack (n - 1) x (ack n x (y - 1))

main = print (ack 5 2 3)
