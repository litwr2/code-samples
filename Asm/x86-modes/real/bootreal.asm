;redefines INT 8 and 9 handlers
;shows timer and keyboard interrupt activity
;makes a beep at the every kbd scancode, uses HLT to cool CPU
;reboots after ESC key pressed
        ORG	0x7c00

	USE16

	cld
	cli				; disable the interrupts
        xor    ax,ax
        mov    ds,ax
        mov    ss,ax
        mov    sp,0x7c00
        mov    dword [8*4],clock        ;set INT 8 handler, timer
        mov    dword [9*4],keyboard     ;set INT 9 handler, keyboard
        sti                             ;enable the interrupts

        mov     ax,0xb800               ;base of video memory
        mov     es,ax

	mov	dword [es:0],'R E '    ;32 bit op
	mov	dword [es:4],'A L '

main_loop:
        cmp     [keyboard.count],0
        jz      .wait

	inc	byte [es:2*(80+1)]      ;show key events
        call    beep
        dec     [keyboard.count]
        jnz     main_loop

.wait:  hlt                             ;cool the processor :-)
	jmp	main_loop

beep:   in	al,0x61			; turn on the speaker
	or	al,3                    ;and attach timer to it
	out	0x61,al

        mov al,0xb           ;setup beep sound frequency
        out 43h,al
        mov ax,1193               ;1193181/freq
        out 42h,al
        mov al,ah
        out 42h,al

        mov     cx,20                   ;200/182=100/91 approx = 1.1 sec
.wait1: mov     ah,[es:2*80]
.wait2: cmp     ah,[es:2*80]
        jz      .wait2
        loop    .wait1

        and     al,0xfc			; turn off the speaker
        out     0x61,al
	ret

clock:	push	ax
	inc	byte [es:2*80]		; make the ticks appear
	mov	al,0x20
	out	0x20,al
	pop	ax
	iret

keyboard:
	push	ax
	in	al,60h
	cmp	al,1			; check for Esc key
	je	reboot

        inc     [.count]
	mov	al,0x20                  ; give finishing information
	out	0x20,al			;to interrupt controller
	pop	ax
	iret

.count  db 0

reboot: mov	al,0xFE
	out	0x64,al			; reboot computer
	jmp	reboot

       rb 510 - $ mod 512
       db 0x55,0xaa

