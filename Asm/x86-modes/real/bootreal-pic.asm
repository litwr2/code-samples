;adds INT 8 handler routine to the standard INT 8 handler
;shows timer interrupt activity, prints 320x200x4 picture
;makes beep at the every completed INT 16h call, reboots after ESC key pressed
;changes bg color after the space key pressed
;uses BIOS timer to make a delay
;uses INT 10h to change CGA palette and bg color and to set video mode
;uses INT 13h to read a picture, uses INT 16h to access to the kbd

        ORG	0x7c00
	USE16
loadbuf = 0x8000

        cld
        cli				; disable the interrupts
        xor    ax,ax
        mov    ds,ax
        mov    ss,ax
        mov    es,ax
        mov    sp,0x7c00

        mov    eax,[8*4]
        mov    [clock.irq + 1],eax
        mov    dword [8*4],clock         ;set INT 8 handler, timer
        ;sti

        mov     al,32                   ;32 sectors = 16 KB
        or      dl,dl                   ;floppy?
        jnz     hdd

        mov     al,17
hdd:    mov     bx,loadbuf              ;read sectors
        mov     ah,2                    ;function #2
        mov     cx,2                    ;start from sector 2 (6 bits), track 0 (10 bits)
        xor     dh,dh
        int     0x13                    ;may set IF=0 with some BIOS! :-(
        sti
        jc      main_loop               ;check disk error

        or      dl,dl
        jnz     showpic

        mov     al,15                   ;17+15=32
        mov     bx,loadbuf+17*512       ;read sectors
        mov     ah,2                    ;function #2
        mov     cl,1                    ;start from sector 1 (6 bits), track 0 (10 bits)
        mov     dh,cl
        int     0x13                    ;may set IF=0 with some BIOS! :-(
        sti
        jc      main_loop

showpic:
        mov     ax,4                    ;set video mode to 320x200 4 colors
        int     0x10

        mov     ax,0xb800               ;set the screen segment
        mov     es,ax

        xor     di,di                   ;copy picture to screen
        mov     si,loadbuf
        mov     cx,2000
        rep movsd

        mov     di,0x2000
        mov     cx,2000
        rep movsd

main_loop:
        call    getkey
        cmp     al,27          ;the ESC key
        jz      reboot

        cmp     al,' '          ;the space key
        jnz     .skip

	movzx   bx,[.bg]           ;set bg color
        mov     ah,0xb
        int     0x10
	inc	[.bg]

.skip:  call    beep
	jmp	main_loop

.bg     db 0

getkey: xor     ah,ah
        int     0x16
        ret

clock:  push	ax
        push    bx
        mov     bl,[.pal]
        mov     bh,1
        and     bl,bh
        mov     ah,0xb           ;change palette
        int     0x10
	inc	[.pal]
        pop     bx
	pop	ax
        inc     byte [es:13734]
.irq:   jmp     0:0             ;to the standard INT 8 handler

.pal    db 0

reboot:	mov	al,0xFE
	out	0x64,al			; reboot computer
	jmp	reboot

beep:   in	al,0x61			; turn on the speaker
	or	al,3
	out	0x61,al

        mov al,0xb           ;setup beep sound frequency
        out 43h,al
        mov ax,1193               ;1193181/freq
        out 42h,al
        mov al,ah
        out 42h,al

        mov     cx,5                   ;50/182=100/91 approx = 0.27 sec
.wait1: mov     ah,[0x46c]             ;BIOS timer
.wait2: cmp     ah,[0x46c]
        jz      .wait2
        loop    .wait1

        and     al,0xfc			; turn off the speaker
        out     0x61,al
	ret

showcx:                  ;at the cursor position
       mov     al,ch
       shr     al,4
       call    makedigit
       mov     al,ch
       and     al,0xf
       call    makedigit

showcl:
       mov     al,cl
       shr     al,4
       call    makedigit
       mov     al,cl
       and     al,0xf

makedigit:
       add    al,'0'
       cmp    al,'9'
       jbe    .exit

       add    al,'A'-'0'-10
if 0
.exit:  mov ah,32       ;put AL-char at DI
       stosw
       ret
else
.exit: mov    ah,0xe  ;BIOS print AL-char at tty, does not work with some BIOS :-(
       mov    bx,1
       int    0x10
       ret
end if

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

