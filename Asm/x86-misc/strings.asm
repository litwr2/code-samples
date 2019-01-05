format ELF64 executable 3       ;3 - Linux

segment readable executable

entry main

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

inmsg:  xor    rax,rax  ;OUT: RSI - buffer, RAX - length, IN:RSI - buffer ptr, RDX - buffer length
        xor    rdi,rdi  ;sysfn #0, stdin = 0
        syscall
        dec    rax             ;ignore the last char
        ret

macro instring s {
        mov     rsi,prompt
        mov     edx,string1-prompt
        call    outmsg
        mov     rsi,string#s
        mov     edx,str#s#maxlen
        call    inmsg
        mov     [strlens+s*8-8],rax
}

main:   instring 1
        inc     byte [numpos]
        instring 2
        inc     byte [numpos]
        instring 3

        mov     r8,string1
.loop2: xor     rax,rax
.loop:  lea     rsi,[r8 + rax]
        mov     rdi,string2
        mov     rdx,string1
        add     rdx,[strlens]
        sub     rdx,rax
        sub     rdx,r8
        jbe     .notfound

        mov     rcx,[strlens+8]
        cmp     rdx,rcx
        jb      .notfound

        repz    cmpsb
        jz      .found

        inc     rax
        jmp     .loop

.found: mov     rdi,string4
        add     rdi,[strlens+3*8]
        add     [strlens+3*8],rax
        xchg    r8,rsi
        mov     rcx,rax
        rep     movsb
        mov     rsi,string3
        mov     rcx,[strlens+2*8]
        add     [strlens+3*8],rcx
        rep     movsb
        inc     [counter]
        jmp     .loop2

.notfound:
        mov     rdi,string4
        add     rdi,[strlens+3*8]
        mov     rsi,r8
        mov     rcx,string1
        add     rcx,[strlens]
        sub     rcx,r8
        jz      .theend

        add     [strlens+3*8],rcx
        rep     movsb
.theend:
        mov     byte [rdi],10
        mov     rdx,[strlens+3*8]
        inc     rdx           ;eol
        mov     rsi,string4
        call    outmsg
        movzx   rax,[counter]
        call    todec
        call    outmsg
        xor     edi,edi 	;exit code 0
        mov     eax,60		;sys_exit
        syscall

segment readable writeable

strlens rq      3
        dq      0
counter db      0
prompt  db      'Enter number string #1: '
numpos  = $-3
string1 rb      256
str1maxlen = $-string1
string2 rb      128
str2maxlen = $-string2
string3 rb      128
str3maxlen = $-string3
string4 rb      512
msg     rb      15
msgend  db      10

