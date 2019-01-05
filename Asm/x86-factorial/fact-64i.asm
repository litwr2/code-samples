format ELF64 executable 3       ;3 - Linux

segment readable executable

entry $
        mov     eax,0           ;an argument: 0..20
        call    fact            ;returns RAX
        call    todec           ;form string for output

        mov     edi,1		;STDOUT
        mov     rax,rdi		;sys_write
        syscall

        xor     edi,edi 	;exit code 0
        mov     eax,60		;sys_exit
        syscall

fact:   mov     ebx,eax         ;counter
        mov     eax,1
        or      rbx,rbx
        jnz     .l1
        ret

.l1:    mul     rbx             ;rbx*rax -> rdx:rax
        dec     rbx
        jnz     .l1
        ret

todec:  mov     rsi,msgend       ;IN: RAX; OUT: RSI - start of string, RDX - length of string
        mov     ecx,10
.l1:    xor     rdx,rdx
        div     rcx
        add     dl,'0'
        dec     rsi
        mov     [rsi],dl
        or      rax,rax
        jnz     .l1

        mov     rdx,msgend+1
        sub     rdx,rsi
        ret

segment readable writeable

msg     db      0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,'X'
msgend  db      10

