format ELF64 executable 3
segment readable executable
entry $
      call  printstr
      db "Hello World",10,0

      xor edi,edi   ;код возврата
      mov eax,60    ;завершить процесс
      syscall

printstr:
      pop rsi
      mov edx,1
.startloop:
      cmp [rsi],dh
      jz .endloop

      mov edi,edx   ;STDOUT
      mov eax,edi   ;1 - write
      syscall
      inc rsi
      jmp .startloop
.endloop:
      inc rsi
      push rsi
      ret

