--RUN IT BY
--   ghc -O fib.hs; time ./fib
--OR
--   time runhugs fib.hs

fib 1 = 1
fib 2 = 1
fib n = fib (n - 1) + fib (n - 2)

main = print (fib(25))
