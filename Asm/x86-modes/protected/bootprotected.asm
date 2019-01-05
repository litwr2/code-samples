;turns CPU to the protected mode, opens A20 line
;redefines via IDT INT 8 and 9 handlers
;shows kbd interrupt activity, makes a beep at the every kbd scancode
;shows the main loop activity, uses HLT to cool CPU
;uses redefined timer interrupt to make a delay
;defines 11 entries of IDT, shows these interrupts and exceptions activity
;reboots after ESC key pressed
        ORG	0x7c00
	USE16

        cld
        in al, 0x92         ;enable A20-line
        or al, 2
        out 0x92, al

	cli				; disable the interrupts
        lgdt    [cs:Protected_GDTR]
        mov eax, cr0                  ;switch to the protected mode
        or al, 1
        mov cr0, eax

        mov bx, 16                    ;to the 2nd descriptor of GDT
        mov ds, bx
        mov es, bx
        mov ss, bx
        ;mov fs, bx
        ;mov gs, bx
        jmp 8:pmode                 ;the selector to the 1st descriptior of GDT

pmode:  mov sp,0x7c00
        lidt    [IDTR]
        sti

        mov	dword [0xb8000],'P R '
	mov	dword [0xb8004],'O T '
        mov	dword [0xb8008],'E C '
        mov	dword [0xb800c],'T E '
        mov	dword [0xb8010],'D 1 '
        mov	word [0xb8014],'6 '

main_loop:
        cmp     [keyboard.count],0
        jz      .wait

	inc	byte [0xb8000 + 2*(80+1)]      ;show key events
        call    beep
        dec     [keyboard.count]
        jnz     main_loop

.wait:  hlt                             ;cool the processor :-)
        inc	byte [0xb8000 + 2*(80+2)]
	jmp	main_loop

Protected_GDTR:
         dw GDT_len - 1
         dd Protected_GDTR
         dw 0

;Code Selector Descriptor
         dw 0xFFFF      ;limit 0..15
         dw 0           ;base 0..15
         db 0           ;base 16..23
         db 10011010b   ;PRESENCE=1,PL=00,SYS=1,EXEC=1,0,RW=1,ACCESS=0
         db 0x80 or 0xf ;G=1, D=0 (USE16), limit 16..19
         db 0x00        ;base 24..31

;Data Selector Descriptor
         dw 0xFFFF      ;limit 0..15
         dw 0           ;base 0..15
         db 0           ;base 16..23
         db 10010010b   ;PRESENCE=1,PL=00,SYS=1,EXEC=0,0,RW=1,ACCESS=0
         db 0x80 or 0xf ;G=1, D=0 (USE16 - ignored), limit 16..19
         db 0           ;base 24..31
GDT_len = $ - Protected_GDTR

IDTR:					; Interrupt Descriptor Table Register
      dw EndOf_IDT - StartOf_IDT - 1	; limit of IDT (size minus one)
      dd StartOf_IDT           		; linear address of IDT

StartOf_IDT:
repeat 8                                ;8 exceptions
      dw exception and 0xFFFF           ;low 16 bits of offset, 'and 0xFFFF' maybe skipped
      dw 8                              ;the selector
      dw 0x8700                         ;P=1, PL=00, SYS=0, type = 7 = trap gate 16 bits
      dw exception shr 16               ;high 16 bits of offset, exception shr 16 = 0
end repeat

      dw clock and 0FFFFh               ;timer at INT 8
      dw 8
      dw 0x8600,clock shr 16            ;P=1, PL=00, SYS=0, type = 6 = interrupt gate 16 bits

      dw keyboard and 0FFFFh            ;keyboard at INT 9
      dw 8
      dw 0x8600,0

repeat 1
      dw interrupt and 0FFFFh,8         ; handler for all other interrupts
      dw 0x8600,interrupt shr 16
end repeat
EndOf_IDT:

timer_ticks = 0xb8000+2*80
beep:   in	al,61h			; turn on the speaker
	or	al,3                    ;and attach timer to it
	out	61h,al

        mov al,0xb           ;setup beep sound frequency
        out 43h,al
        mov ax,1193               ;1193181/freq
        out 42h,al
        mov al,ah
        out 42h,al

        mov     cx,10                   ;100/182=100/91 approx = 0.55 sec
.wait1: mov     ah,[timer_ticks]
.wait2: cmp     ah,[timer_ticks]
        jz      .wait2
        loop    .wait1

        and     al,0xfc			; turn off the speaker
        out     0x61,al
 	ret

clock:	push	ax
    	inc	byte [timer_ticks]	; make the ticks appear
	jmp     keyboard.eoi

keyboard:
	push	ax
	in	al,60h
        inc     [.count]
	cmp	al,1			; check for Esc key
	je	reboot

.eoi:   mov	al,20h
	out	20h,al
	pop	ax
	iret

.count  db 0

reboot: mov	al,0FEh
	out	64h,al			; reboot computer
	jmp	reboot

exception:				; exception handler
        inc	byte [0xb8000+9*160]	; make the ticks appear
	jmp	exception		; repeat it until reboot

interrupt:				; handler for all other interrupts
        push    ax
        inc	byte [0xb8000+20*160]	; make the ticks appear
        jmp     keyboard.eoi

       freemem = 510 - $ mod 512
       rb freemem
       db 0x55,0xaa

bits = 16
    display "0x"
    repeat bits/4
        d = '0' + freemem shr (bits-%*4) and 0Fh
        if d > '9'
            d = d + 'A'-'9'-1
        end if
        display d
    end repeat
    display " bytes free in this sector",10

