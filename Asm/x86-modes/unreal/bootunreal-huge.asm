;turns CPU to unreal mode, opens A20 line
;redefines INT 9 handler
;shows kbd interrupt activity, makes a beep at the every kbd scancode
;shows BIOS timer activity, uses HLT to cool CPU
;uses BIOS timer to make a delay, calls routine outside 1MB limit 
;reboots after ESC key pressed
        ORG	0x7c00
	USE16

        cld
        in al, 0x92         ;enable A20-line
        or al, 2
        out 0x92, al

	cli				; disable the interrupts
        mov    sp,0x7c00
        mov    dword [9*4],keyboard

        lgdt    [cs:FlatRM_GDTR]
        mov eax, cr0               ;switch to the protected mode
        or al, 1
        mov cr0, eax
        jmp 0x8:pmode              ;use the 1st selector of GDT

pmode:  mov bx, 16                 ;the 2nd selector in GTR
        mov ds, bx
        mov es, bx
        mov ss, bx
        ;mov fs, bx
        ;mov gs, bx

        and al, 0xfe               ;return back to the real mode
        mov cr0, eax
        jmp 0:to_unreal_huge

to_unreal_huge:
        xor ax, ax
        mov ds, ax
        mov es, ax
        mov ss, ax
        ;mov fs, ax
        ;mov gs, ax
        sti

farproc_addr = 0x180000
        mov     esi,farproc      ;test huge mode
        mov     edi,farproc_addr
        mov     ecx,farproc_len
        rep movs byte [edi],[esi]

        mov	dword [0xb8000],'U N '
	mov	dword [0xb8004],'R E '
        mov	dword [0xb8008],'A L '
        mov	dword [0xb800c],'H U '
        mov	dword [0xb8010],'G E '

main_loop:
        cmp     [keyboard.count],0
        jz      .wait

	inc	byte [0xb8000 + 2*(80+2)]      ;show key events
        call    beep
        dec     [keyboard.count]
        jnz     main_loop

.wait:  hlt                             ;cool the processor :-)
        mov     eax,[0x46c]             ;BIOS timer
        mov     [0xb8000 + 160],eax
        mov     eax,farproc_addr
        call    eax
	jmp	main_loop

FlatRM_GDTR: 
         dw GDT_len - 1
         dd FlatRM_GDTR
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
GDT_len = $ - FlatRM_GDTR 

beep:   in	al,0x61			; turn on the speaker
	or	al,3                    ;and attach timer to it
	out	0x61,al

        mov al,0xb           ;setup beep sound frequency
        out 43h,al
        mov ax,1193               ;1193181/freq
        out 42h,al
        mov al,ah
        out 42h,al

        mov     cx,10                   ;100/182=100/91 approx = 0.55 sec
.wait1: mov     ah,[0x46c]              ;BIOS timer low byte
.wait2: cmp     ah,[0x46c]
        jz      .wait2
        loop    .wait1

        and     al,0xfc			; turn off the speaker
        out     0x61,al
 	ret

keyboard:
	push	ax
	in	al,60h
	cmp	al,1			; check for Esc key
	je	reboot

        inc     [.count]
	mov	al,20h
	out	20h,al
	pop	ax
	iret

.count  db 0

reboot: mov	al,0FEh
	out	64h,al			; reboot computer
	jmp	reboot

farproc:
        inc  byte [0xb8000+320]        ;3rd line
        retd
farproc_len = $ - farproc

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

