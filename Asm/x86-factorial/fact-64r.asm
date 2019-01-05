;factorial calculator, it uses recursion
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

fact:   cmp     rax,1           ;in: rax;  out: rcx
        ja      .l1

        mov     eax,1
        ret

.l1:    push    rax
        dec     rax
        call    fact
        pop     rbx
        mul     rbx              ;rbx*rax -> rdx:rax
        ret

todec:  mov     rsi,msgend       ;IN: RAX; OUT: RSI - start of string, RDX - length of string
        mov     ecx,10
.l1:    xor     rdx,rdx
        div     rcx
        add     dl,48            ;48 = '0'
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

