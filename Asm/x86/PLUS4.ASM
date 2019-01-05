format MZ

                INCLUDE 'FASM.MAC'
                INCLUDE 'CONST.MAC'
                INCLUDE '8501.MAC'
                INCLUDE 'ADLIB.MAC'
                INCLUDE 'IEC.MAC'
                INCLUDE 'HARDWARE.MAC'

ENTRY SEG_CODE:START                    ;program entry point
STACK 2000H                             ;stack size
HEAP 0

SEGMENT         SEG_DATA USE16
        db 'xxxx'
                INCLUDE 'DOS.DAT'
                INCLUDE 'HARDWARE.DAT'
                INCLUDE 'MDAC.DAT'
                INCLUDE 'ROMFN.DAT'
                INCLUDE '8501.DAT'
                INCLUDE 'CDS.DAT'
                INCLUDE 'IEC.DAT'
                INCLUDE 'DEBUGGER.DAT'
                INCLUDE 'KBD.DAT'
                INCLUDE 'JOY.DAT'
                INCLUDE 'IO.DAT'
                INCLUDE 'TED.DAT'
                INCLUDE 'VIDEO.DAT'
                INCLUDE 'MENUS0.DAT'
                INCLUDE 'MENUS1.DAT'
                INCLUDE 'MENUS2.DAT'
                INCLUDE 'MENUS3.DAT'
                INCLUDE 'SOUND.DAT'
                INCLUDE 'ADLIB.DAT'
                INCLUDE 'TIMER.DAT'
                INCLUDE 'SB.DAT'
                ;INCLUDE 'LOG.DAT'

SEGMENT         SEG_CODE USE16          ;main program segment
        db 'xxxx'
START:          PUSH    DS              ;SET RETURN TO DOS ADDRESS
                PUSH    WORD 0
                MOV     AX,SEG_DATA     ;INIT DS
                MOV     DS,AX
                MOV     [RESET_SP],SP
                CALL    SEG_CODEX:INITIBM_F
RESET:          MOV     SP,[RESET_SP]
                CALL    SEG_CODEX:XRESET_F
MAIN_ENTRY:     MOV     [MAINJUMP],MAINLOOP
                ;MOV     [MAINJUMP],DEBUG_ENTRY

MAINLOOP:       CALL    TED0
                CALL    CPU8501
		CALL    TED1
                CALL    CPU8501
                JMP     [MAINJUMP]

END_EMU:        MOV     SP,[RESET_SP]
                JMP     SEG_CODEX:FINISHIBM_F

                INCLUDE 'KBD.COD'
                INCLUDE 'JOY.COD'
                INCLUDE 'TED.COD'
                INCLUDE 'MEMORY.COD'
                INCLUDE '8501.COD'
                INCLUDE 'CPU_IO.COD'
                INCLUDE 'CDS.COD'
                INCLUDE 'CSB.COD'
                INCLUDE 'VIDEO.COD'
                INCLUDE 'SOUND.COD'
                INCLUDE 'ADLIB.COD'
                INCLUDE 'TIMER.COD'
                INCLUDE 'SB.COD'
                INCLUDE 'C2X.COD'

SEGMENT         SEG_CODEX USE16          ;extra program segment
        db 'xxxx'
                INCLUDE 'CONV.CDX'
                INCLUDE 'INTR8.CDX'
                INCLUDE 'INTR9.CDX'
                INCLUDE 'X2C.CDX'
                INCLUDE 'IEC.CDX'
                INCLUDE 'DEBUGGER.CDX'
                INCLUDE 'MENUS0.CDX'
                INCLUDE 'MENUS1.CDX'
                INCLUDE 'MENUS2.CDX'
                INCLUDE 'MENUS3.CDX'
                INCLUDE 'HARDWARE.CDX'
                ;INCLUDE 'LOG.CDX'
        db 'xxxx'

SEGMENT         SEG_RAM USE16
TIMES   32*1024 DW      0FFH

SEGMENT         SEG_ROM USE16
                FILE    '../BASIC.ROM'
                FILE    '../KERNAL.ROM'
                FILE    '../3P1.ROM'

SEGMENT         SEG_CART USE16
TIMES   16*1024 DB      'F'             ;C1-LOW
TIMES   16*1024 DB      'G'             ;C1-HIGH
TIMES   16*1024 DB      'H'             ;C2-LOW
TIMES   16*1024 DB      'I'             ;C2-HIGH

SEGMENT         SEG_1551 USE16
                FILE    '../1551.ROM'

SEGMENT         SEG_VRAM1 USE16
TIMES   16*1024 DD      'VBE2'

SEGMENT         SEG_VRAM2 USE16 ;4K used for fonts transform 8->16
TIMES   79*512  DB      '2'

SEGMENT         SEG_DBG USE16
TIMES   4000    DB      4               ;DEBUGGER

SEGMENT         SEG_DS  USE16
TIMES   8000    DB      5               ;DATASETTE

