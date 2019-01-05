'COMPILE IT AT GAMBAS3 ENVIRONMENT
'THEN RUN AT COMMAND LINE BY
'   time ./FN

Function ack(n As Integer, x As Integer, y As Integer) As Long
   If n = 0 Then 
      Return y + 1
   Else If y = 0 Then
      If n = 1 Then
         Return x
      Else If n = 2 Then
         Return 0
      Else
         Return 1 
      End If
   Else
      Return ack(n - 1, x, ack(n, x, y - 1))
   End If
End

Public Sub Main()
   Print ack(1, 1, 1000)
End
