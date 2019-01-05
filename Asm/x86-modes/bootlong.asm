;turns CPU to the long mode, maps 2 MB RAM to the page translation
;redefines via IDT INT 8 and 9 handlers, opens A20 line
;shows clock/kbd interrupt activity, uses HLT to cool CPU
;defines 2 entries of IDT, fakes 8 IDT entries
;reboots after ESC key pressed, shows the main loop activity
        ORG	0x7c00
	USE16

        cld
        in al, 0x92         ;enable A20-line
        or al, 2
        out 0x92, al

	cli			; disable the interrupts
	lgdt	[cs:GDTR]	; load GDT register

	mov	eax,cr0 		; switch to protected mode
	or	al,1
	mov	cr0,eax
	jmp	16:pm_start

GDTR:					; Global Descriptors Table Register
        dw 4*8 - 1			; limit of GDT (size minus one)
        dq GDT - 8			; linear address of GDT & null descriptor

GDT     dw 0FFFFh, 0, 9200h, 08Fh	; flat data descriptor
        dw 0FFFFh, 0, 9A00h, 0CFh	; 32-bit code descriptor
        dw 0, 0, 9800h, 0x20	        ; entrance to 64-bit code descriptor
;0x0020980000000000
;     .        ....    limit
;  ..    ......        base
;    * **              long, presence, system, code = 1

	USE32
pm_start:
        mov esp,0x7c00
	mov	eax,8	                ; load 4 GB data descriptor
	mov	ds,ax			; to all data segment registers
	mov	es,ax
	mov	ss,ax

	mov	eax,cr4
	or	eax,1 shl 5
	mov	cr4,eax 		; enable physical-address extensions

	mov	edi,70000h
	mov	ecx,4000h shr 2
	xor	eax,eax
	rep	stosd			; clear the page tables: set presence bit to 0

	mov	dword [70000h],71000h + 7 ; first PDP table, sets presence, write and user bit
	mov	dword [71000h],72000h + 7 ; first page directory
	mov	dword [72000h],73000h + 7 ; first page table

	mov	edi,73000h		; address of first page table
	mov	eax,7
	mov	ecx,512 		; number of pages to map (512*4 KB = 2 MB)

make_page_entries:
	stosd
	add	edi,4
	add	eax,4096                ;4 KB = 0x1000 B = 1 Page
	loop	make_page_entries

	mov	eax,70000h
	mov	cr3,eax 		; load page-map level-4 base

	mov	ecx,0C0000080h
	rdmsr
	or	eax,1 shl 8		; enable long mode
	wrmsr

	mov	eax,cr0
	or	eax,1 shl 31
	mov	cr0,eax 		; enable paging
	jmp	24:long_start

	USE64
long_start:
	lidt	[IDTR]			; load IDT register
	sti				; now we may enable the interrupts

        mov     rax,'L O N G '
	mov	[0xb8000],rax

main_loop:
        hlt
        inc	byte [0xB8000+2*(80+2)]
	jmp	main_loop

IDTR:     				;Interrupt Descriptor Table Register
      dw 10*16 - 1                      ;limit of IDT (size minus one)
      dq StartOf_IDT - 16*8     	;linear address of IDT, 8 entries are faked

StartOf_IDT:
      dw clock,24                       ;timer at INT 8, long selector
      dw 0x8e00,0                       ;P=1, PL=00, SYS=0
      dq 0

      dw keyboard,24                    ;keyboard at INT 9
      dw 0x8e00,0                       ;type = 0xe = 32/64 bit interrupt gate
      dq 0

clock:	inc	byte [0xB8000+2*80]	; make the ticks appear
	push	rax
        jmp     keyboard.eoi

keyboard:
	push	rax
	in	al,60h
	cmp	al,1			; check for Esc key
	je	reboot

	mov	[0B8000h+2*(80+1)],al	; show the scan key
.eoi:	mov	al,20h
	out	20h,al
	pop	rax
	iretq

reboot:
	mov	al,0FEh
	out	64h,al			; reboot computer
	jmp	reboot

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
