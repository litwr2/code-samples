;RUN IT BY
;   fasm FN.asm; time ./FN

format ELF executable 3

segment readable executable

entry $
        mov     eax,41          ;an argument
        push    eax
        call    fib
        mov     eax,ecx
        call    todec           ;form string for output

	mov	edx,msgend-msg+1
        mov	ecx,msg
.l2:    cmp     byte [ecx],0    ;skip leading zeros
        jnz     .l1

        dec     edx
        inc     ecx
        jmp     .l2

.l1:	mov	ebx,1		;STDOUT
	mov	eax,4		;sys_write
	int     0x80

	xor	ebx,ebx 	;exit code 0
	mov	eax,1		;sys_exit
	int     0x80

fib:    cmp     eax,2           ;in: eax;  out: ecx
        ja      .l1

        mov     ecx,1
        ret

.l1:    dec     eax
        push    eax
        call    fib
        pop     eax
        push    ecx
        dec     eax
        call    fib
        pop     eax
        add     ecx,eax
        ret

todec:  mov     ebx,msgend-1
        mov     ecx,10
.l1:    xor     edx,edx
        div     ecx
        add     dl,48
        mov     [ebx],dl
        dec     ebx
        or      eax,eax
        jnz     .l1
        ret

segment readable writeable

msg     db      0,0,0,0,0,0,0,0,0,0
msgend  db     10

