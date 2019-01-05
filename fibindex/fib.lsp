;RUN IT BY
;   time clisp fib.lsp
;OR
;   time sbcl --load fib.lsp

(defun fib (n)
  (cond
    ((< n 3) 1)
    ((+ (fib (- n 1)) (fib (- n 2))))))

(print (fib 30))
(quit)



