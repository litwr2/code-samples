/******************************************************************************\
    malt_proc.h

    Header file for processor-related definitions and functions
\******************************************************************************/

#ifndef _MALT_PROC_H
#define _MALT_PROC_H

/* ######################### *
 * ## MicroBlaze-specific ##
 * ######################### */

#define PROC_START_ADDR     0x00000000
#define EXCEPTION_ENT_ADDR  0x00000008
#define INTERRUPT_ENT_ADDR  0x00000010
#define BREAK_ENT_ADDR      0x00000018
#define THREAD_END_ADDR     0x00000020  // MALT slave specific

#define MSW_IE              0x00000002


/* ####################### *
 * ## Macro definitions ##
 * ####################### */

#ifndef __ASSEMBLER__   // this allows including to assembler sources

/* General */
#define malt_read_reg(a)    (*(volatile u32_t*)(a))
#define malt_write_reg(a,v) (*(volatile u32_t*)(a) = v)

#define asmi(...) __asm__ __volatile__(__VA_ARGS__)


/* Coprocessors */
#define malt_get_num_coproc_units()  malt_read_reg(SYSINFO_COPROC_ADDR)

/* Interrupts */
#if TASIC
// workaround TASIC IE bug (DO NOT OPTIMIZE THIS MACRO)
#define malt_enable_interrupts() _FL(\
    asmi("  addi    r11, r0, 2\n\
            swi     r11, r0, %1\n\
            nop; nop; nop;\n\
            ori     r11, r0, %0\n\t\
            mts     rmsr, r11"::"i"(MSW_IE),"i"(TASIC_IE_ADDR):"r11"))
#else
#define malt_enable_interrupts() \
    asmi("  mfs     r11, rmsr\n\t\
            ori     r11, r11, %0\n\t\
            mts     rmsr, r11"::"i"(MSW_IE):"r11")
#endif

#if TASIC
// workaround TASIC IE bug (DO NOT OPTIMIZE THIS MACRO)
#define malt_disable_interrupts() _FL(\
    asmi("  nop\n\
            mts     rmsr, r0\n\t\
            nop; nop; nop \n\
            swi     r0, r0, %0"::"i"(TASIC_IE_ADDR):"r11"))
#else
#define malt_disable_interrupts() \
    asmi("  mfs     r11, rmsr\n\t\
            andi    r11, r11, %0\n\t\
            mts     rmsr, r11\n\t\
            nop"::"i"(~MSW_IE):"r11")
#endif

#if TASIC
// workaround TASIC IE bug (DO NOT OPTIMIZE THIS MACRO)
#define malt_save_interrupt_state(ADDR) \
    asmi("  lwi    %0, r0, %1":"=r"(ADDR):"i"(TASIC_IE_ADDR):)
#else
#define malt_save_interrupt_state(ADDR) \
    asmi("  nop\n\t\
            mfs     %0, rmsr\n\t\
            nop\n\t\
            andi    %0, %0, 2":"=r"(ADDR)::)
#endif

#if TASIC
// workaround TASIC IE bug (DO NOT OPTIMIZE THIS MACRO)
#define malt_restore_interrupt_state(ADDR) \
    asmi("  nop\n\t\
            addi    r11, r0, 2\n\
            mts     rmsr, %0\n\t\
            nop\n\
            swi     %0, r0, %1"::"r"(ADDR),"i"(TASIC_IE_ADDR):"r11")
#else
#define malt_restore_interrupt_state(ADDR) \
    asmi("  nop\n\t\
            mfs     r11, rmsr\n\t\
            andi    r11, r11, -3\n\t\
            or      r11, r11, %0\n\t\
            mts     rmsr, r11\n\t\
            nop"::"r"(ADDR):"r11")
#endif

/* Instruction cache */
#define malt_invalidate_icache()    malt_bc_cmd_reg(BCCMDR_INV_ICACHE)

extern void* local_init_sp; 

/* Stack */
#define malt_local_stack_call(f)  _FL(asmi(" \
    add     r28, r15, r0\n\t\
    add     r27, r20, r0\n\t\
    addik   r15, r0, %0\n\t\
    lw      r15, r0, r15\n\t\
    add     r29, r1, r0\n\t\
    add     r1, r0, r15\n\t\
    brlid   r15, %1\n\t\
    nop\n\t\
    add     r15, r28, r0\n\t\
    add     r1, r29, r0"::"i"(&local_init_sp), "i"(f): \
      "r3", "r4", "r5", "r6", "r7", "r8", \
      "r9", "r10", "r11", "r12", "r27", "r28", "r29"))

#if TASIC == 1
#define tasic_slave_stack_fix()    _FL(asmi(" \
    add     r29, r1, r0\n\t\
    addi    r11, r0, %0\n\t\
    add     r1, r0, r11\n\t\
    swi     r15, r1, 4"::"i"(SPB_MEM_BASE_ADDR-8+4096-512): \
      "r3", "r4", "r5", "r6", "r7", "r8", \
      "r9", "r10", "r11", "r12", "r29"))
      
#define tasic_return()  _FL(asmi("lwi r15, r1, 4; rtsd  r15, 8"))

#else
#define tasic_slave_stack_fix()
#define tasic_return()
#endif

/* ############### *
 * ## Functions ##
 * ############### */

#endif  // __ASSEMBLER__

#endif
