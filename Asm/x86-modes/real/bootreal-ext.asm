;redefines INT 8 and 9 handlers
;makes kbd buffer, shows kbd scancodes and timer interrupt activity
;prints values of DS and SR at the startup, makes a beep at the every kbd scancode
;reboots after ESC key pressed
        ORG	0x7c00

	USE16

        pushf                           ;will be printed
        pop bp
        mov cx,ds

        cld
	cli				; disable the interrupts
        xor    ax,ax
        mov    ds,ax
        mov    ss,ax
        mov    sp,0x7c00
        mov    dword [8*4],clock        ;set INT 8 handler, timer
        mov    dword [9*4],keyboard     ;set INT 9 handler, keyboard

        mov     ax,0xb800               ;set the screen segment
        mov     es,ax

	mov	dword [es:0],'R E '
	mov	dword [es:4],'A L '

        mov     di,16
        mov     eax,'d s '
        stosd
        call    showcx
        add     di,2
        mov     eax,'s r '
        stosd
        mov     cx,bp
        call    showcx    ;show SR
        mov     di,320    ;3rd line
        sti

main_loop:
        call    getkey
        mov     cl,al
	call	showcl         ; show the scan key
        add     di,2           ;space between keycodes
        push    main_loop
beep:   in	al,61h			; turn on the speaker
	or	al,3
	out	61h,al

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

getkey: mov     al,[keyboard.start]   ;*OUT: AL - keyboard scancode
        cmp     al,[keyboard.end]
        jz      getkey

        mov     bl,al
        and     bx,0xf
        inc     [keyboard.start]
        mov     al,[keyboard.buf+bx]
        ret

clock:  push	ax
	inc	word [es:2*80]
	mov	al,0x20
	out	0x20,al
	pop	ax
	iret

keyboard:
	push	ax
	in	al,0x60
	cmp	al,1			; check for Esc key
	je	reboot

        push    bx                      ;fill the kbd queue
        movzx   bx,[.end]
        and     bl,0xf
        mov     [.buf+bx],al
        inc     [.end]
        pop     bx

	mov	al,0x20
	out	0x20,al
	pop	ax
	iret

.buf    rb 16
.start  db 0
.end    db 0

reboot:	mov	al,0xFE
	out	0x64,al			; reboot computer
	jmp	reboot

showcx: mov     al,ch                   ;*at DI, di/160 = y, (di%160)/2 = x
        shr     al,4
        call    makedigit
        mov     al,ch
        and     al,0xf
        call    makedigit

showcl: mov     al,cl
        shr     al,4
        call    makedigit
        mov     al,cl
        and     al,0xf

makedigit:
        add    al,'0'
        cmp    al,'9'
        jbe    .exit

        add    al,'A'-'0'-10
.exit:  mov    ah,32    ;color green
        stosw
        ret

       rb 510 - $ mod 512
       db 0x55,0xaa

