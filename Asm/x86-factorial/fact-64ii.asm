format ELF64 executable 3       ;3 - Linux

segment readable executable

entry $
        mov     rsi,prompt
        mov     rdx,msg-prompt
        call    outmsg
        call    inmsg
        dec     rax             ;ignore the last char
        call    tobin
        call    fact            ;returns RAX
        call    todec           ;form string for output
        call    outmsg

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

outmsg: mov     edi,1     ;IN: RSI - buffer, RDX - length
        mov     rax,rdi	  ;sys fn #1, stdout = 1
        syscall
        ret

inmsg:  xor    rax,rax  ;OUT: RSI - buffer, RAX - length
        xor    rdi,rdi  ;sysfn #0, stdin = 0
        mov    rsi,msg
        mov    rdx,16    ;max len
        syscall
        ret

tobin:  mov rdi,rax       ;IN: RSI - buffer, RAX - length
        xor rax,rax       ;OUT: RAX
        mov ecx,10
.l1:    mul rcx
        movzx rbx,byte [rsi]
        xor bl,0x30
        add rax,rbx
        inc rsi
        dec rdi
        jnz .l1

        ret

segment readable writeable

prompt  db      'Enter number (0..20): '    ;to previous segment?
msg     db      0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
msgend  db      10

