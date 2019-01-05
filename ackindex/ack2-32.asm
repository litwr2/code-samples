;RUN IT BY
;   fasm FN.asm; time ./FN

format ELF executable 3

segment readable executable

entry $
        mov     eax,5          ;n
        mov     ebx,2          ;x
        mov     ecx,3          ;y
        call    ack
        mov     eax,edx
        call    todec          ;form decimal string for output

	mov	edx,msgend-msg+1
        mov	ecx,msg
.l2:    cmp     byte [ecx],0  ;skip leading zeros
        jnz     .l1

        dec     edx
        inc     ecx
        jmp     .l2

.l1:    mov	ebx,1		;STDOUT
	mov	eax,4		;sys_write
	int     0x80

	xor	ebx,ebx 	;exit code 0
	mov	eax,1		;sys_exit
	int     0x80

ack:    or      eax,eax         ;in: eax;  out: ecx
        jne     .l1

        mov     edx,ecx
        inc     edx
        ret

.l1:    or      ecx,ecx
        jne     .l2

        dec     eax
        jne     .l3

        mov     edx,ebx
        ret

.l3:    xor     edx,edx
        dec     eax
        jne     .l4
        ret

.l4:    inc     edx
        ret

.l2:    push    eax
        push    ebx
        dec     ecx
        call    ack
        pop     ebx
        pop     eax

        dec     eax
        mov     ecx,edx
        jmp     ack

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

msg     db      0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
msgend  db      10

