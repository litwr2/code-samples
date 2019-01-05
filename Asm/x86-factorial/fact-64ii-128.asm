format ELF64 executable 3       ;3 - Linux

segment readable executable

todec:  mov     rsi,msgend       ;IN: RDX:RAX; OUT: RSI - start of string, RDX - length of string
        mov     ecx,10
.l1:    push    rax
        mov     rax,rdx
        xor     rdx,rdx
        div     rcx
        mov     rbx,rax
        pop     rax
        div     rcx
        add     dl,'0'
        dec     rsi
        mov     [rsi],dl
        mov     rdx,rbx
        or      rax,rax
        jnz     .l1

        mov     rdx,msgend+1
        sub     rdx,rsi
        ret

tobin:  mov rcx,rax       ;IN: RSI - buffer, RAX - length
        xor rax,rax       ;OUT: RDX:RAX
        xor rdx,rdx
        mov edi,10
.l1:    push rax
        mov rax,rdx
        mul rdi
        mov rbp,rax
        pop rax
        mul rdi
        movzx rbx,byte [rsi]
        xor bl,0x30
        add rax,rbx
        adc rdx,rbp
        inc rsi
        loop .l1

        ret

outmsg: mov     edi,1     ;IN: RSI - buffer, RDX - length
        mov     rax,rdi   ;sys fn #1, stdout = 1
        syscall
        ret

inmsg:  xor    rax,rax  ;OUT: RSI - buffer, RAX - length
        xor    rdi,rdi  ;sysfn #0, stdin = 0
        mov    rsi,msg
        mov    rdx,16    ;max len
        syscall
        ret

fact:   mov     ecx,eax         ;IN: RDX:RAX, OUT: RDX:RAX
        mov     eax,1
        jrcxz   .l2

.l1:    push    rax
        mov     rax,rdx
        mul     rcx
        mov     rbx,rax
        pop     rax
        mul     rcx             ;rcx*rax -> rdx:rax
        add     rdx,rbx
        loop    .l1
.l2:    ret

entry $
        mov     rsi,prompt
        mov     rdx,msg-prompt
        call    outmsg
        call    inmsg
        dec     rax             ;ignore the last char
        call    tobin
        call    fact            ;returns RDX:RAX
        call    todec           ;form string for output
        call    outmsg
        xor     edi,edi 	;exit code 0
        mov     eax,60		;sys_exit
        syscall

segment readable writeable

prompt  db      'Enter number (0..34): '
msg     rb      38
msgend  db      10

