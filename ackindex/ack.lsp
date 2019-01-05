;RUN IT BY
;   time clisp ack.lsp
;OR
;   time sbcl --load ack.lsp

(defun ack (n x y)
  (cond
    ((= n 0) (+ y 1))
    ((= y 0)
       (cond
           ((= n 1) x)
           ((= n 2) 0)
           (1)))
    ((ack (- n 1) x (ack n x (- y 1))))))

(print (ack 1 1 5400))
(quit)



