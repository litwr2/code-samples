;Ackermann function calculation
;RUN IT BY
;   fasm FN.asm; time ./FN

format ELF64 executable 3

segment readable executable

entry $
        mov     eax,5          ;n
        mov     ebx,2          ;x
        mov     ecx,3          ;y
        call    ack
        mov     rax,rdx
        call    todec          ;form decimal string for output

	mov	rdx,msgend-msg+1
        mov	rsi,msg
.l2:    cmp     byte [rsi],0  ;skip leading zeros
        jnz     .l1

        dec     rdx
        inc     rsi
        jmp     .l2

.l1:	mov	edi,1		;STDOUT
	mov	eax,1		;sys_write
	syscall

	xor	edi,edi 	;exit code 0
	mov	eax,60		;sys_exit
	syscall

ack:    or      rax,rax         ;in: rax;  out: rcx
        jne     .l1

        mov     rdx,rcx
        inc     rdx
        ret

.l1:    or      rcx,rcx
        jne     .l2

        dec     rax
        jne     .l3

        mov     rdx,rbx
        ret

.l3:    xor     rdx,rdx
        dec     rax
        jne     .l4
        ret

.l4:    inc     rdx
        ret

.l2:    push    rax
        push    rbx
        dec     rcx
        call    ack
        pop     rbx
        pop     rax

        dec     rax
        mov     rcx,rdx
        jmp     ack

todec:  mov     rbx,msgend-1
        mov     ecx,10
.l1:    xor     rdx,rdx
        div     rcx
        add     dl,48
        mov     [rbx],dl
        dec     rbx
        or      rax,rax
        jnz     .l1
        ret

segment readable writeable

msg     db      0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
msgend  db      10

