'COMPILE IT AT GAMBAS3 ENVIRONMENT
'THEN RUN AT COMMAND LINE BY
'   time ./FN

Function fib(n As Integer) As Long
   If n < 3 Then
      Return 1
   Else
      Return fib(n - 2) + fib(n - 1)
   End If
End

Public Sub Main()
   Print fib(32)
End
