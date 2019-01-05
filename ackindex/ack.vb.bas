Function ack(n As Integer, x As Integer, y As Integer) As Long
   If n = 0 Then 
      ack = y + 1
   Else If y = 0 Then
      If n = 1 Then
         ack = x
      Else If n = 2 Then
         ack = 0
      Else
         ack = 1 
      End If
   Else
      ack = ack(n - 1, x, ack(n, x, y - 1))
   End If
End Function

Public Sub Main()
   Print ack(1, 1, 1000)
End Sub
