#include "malt_asm.h"
#include "malt_periph.h"
#include "malt_spbus.h"

#define ENABLE_DMM          1 
#define ENABLE_THRCTL       (!SMASTER_IROM)
#define ENABLE_PROG_THREAD  0
#define ENABLE_BARRIER      0
#define ENABLE_GDB          1

#define SPB_SOFT_FIXED_DATA 0x40000100 //__soft_spb_fixed_data
#define SPB_STACKCOLLISON_CHECK 0

#if ENABLE_GDB
    #define SAVE_REGS   7
#elif !LEGACY_SPB
#if SPB_STACKCOLLISON_CHECK
    #define SAVE_REGS   5
#endif
#else
    #define SAVE_REGS   4
#endif

interrupt:
    // SPbus interrupt handler - thread control and other SPbus parsing is done here

    // save CPU state
    _STACK_GROW(SAVE_REGS*4)
    _SAVE_GPR(r11, 0)
    _SAVE_GPR(r12, 4)
    _SAVE_GPR(r13, 8)
    //_SAVE_SPR(rmsr, r11, 12)  // !! using keep carry, so actually not needed
    
    #if SMASTER_IROM
    _ACK_INTERRUPT
    
    // check if this is an interrupt from SPbus
    _GET_INT_NUM(r11)
    addik   r11, r11, -INTNUM_INTERCON
    bnei    r11, return_from_int
    #endif

    // get message pointer
    lwi     r11, r0, BUSC_SPB_HCNT_ADDR
    // get spb packet type
    lbui    r12, r11, SPB_PACK_TYPE_OFF
    
    #if ENABLE_GDB
    addik   r13, r12, -IC_SPB_STYPE_GDB
    beqi    r13, call_gdb_stub
    gdb_ret:
    #endif
    
#if !LEGACY_SPB
    // process everything with IC packet type, but without bit 0x08 set
    andi    r13, r12, ~IC_SPB_STYPE_SWMSK
    addik   r13, r13, -IC_SPB_STYPE_DATA
    beqi    r13, process_soft_packet
#endif

    // Select handler based on request type
    #if ENABLE_DMM
    // DMM
    addik   r13, r12, -SPB_TYPE_DMM_READ
    beqi    r13, process_dmm
    #endif
    
    #if ENABLE_THRCTL
    // start thread request
    addik   r13, r12, -IC_SPB_STYPE_TSTART
    beqi    r13, start
    // stop thread request
    addik   r13, r12, -IC_SPB_STYPE_TSTOP
    beqi    r13, stop
    #endif
    
    #if ENABLE_BARRIER
    addik   r13, r12, -IC_SPB_STYPE_BARR
    beqi    r13, return_from_int_norestore
    #endif

return_from_int:
    // for other messages just return
    //_RESTORE_SPR(rmsr, r11, 12) // !! using keep carry, so actually not needed
    #if !SMASTER_IROM
    _RESTORE_GPR(r11, 0)
    #endif
    _RESTORE_GPR(r12, 4)
    _RESTORE_GPR(r13, 8)  
return_from_int_norestore:
    _STACK_SHRINK(SAVE_REGS*4)
    
    #if SMASTER_IROM
    // on smaster check if handler in shared mem is present & call it
    lwi     r11, r0, SPBMEM_SM_GLB_IHNDL
    beqi    r11, smaster_restore_r11
    brad    r11 // with delay!
smaster_restore_r11:
    _RESTORE_GPR(r11, -(SAVE_REGS*4 - 0))
    #endif

    // on slave just return from int
    rtid    r14, 0
    nop

#if ENABLE_DMM
process_dmm:
#if LEGACY_SPB
    swi     r0, r0, BUSC_SPB_SCNT_ADDR  // !! increment software pointer if no unparsed soft msgs
#endif
    swi     r0, r0, BUSC_SPB_RPEND_ADDR // decrement pending read counter
    bri     return_from_int
#endif

#if !LEGACY_SPB
process_soft_packet:
#if SPB_STACKCOLLISON_CHECK
    _SAVE_GPR(r15, 16)
#endif
    addik   r13, r0, SPBMEM_FIXED_ICDATA
    lw      r12, r13, r0    //incoming packet pointer
    addik   r12, r12, 16

#if SPB_STACKCOLLISON_CHECK
    cmpu    r15, r1, r12    //clash with stack pointer check
    blti    r15, soft_packet_copy
    brid    soft_packet_end
    swi     r11, r13, 8    //keep a pointer to a packet in hw circular buffer
#endif

soft_packet_copy:
    sw      r12, r13, r0    //incoming packet pointer increment
    lwi     r13, r11, 0
    swi     r13, r12, -16
    lwi     r13, r11, 4
    swi     r13, r12, -12
    lwi     r13, r11, 8
    swi     r13, r12, -8
    lwi     r13, r11, 12
    swi     r13, r12, -4

soft_packet_end:
#if SPB_STACKCOLLISON_CHECK
    _RESTORE_GPR(r15, 16)    //move to delay slot?
#endif
    bri     return_from_int
#endif


#if ENABLE_GDB
call_gdb_stub:
    #if !SMASTER_IROM
    // just exit to route it on masters
    _MALT_CUR_CORE_NUM(r12)
    andi    r12, r12, SPB_CORENUM_MASK
    beqi    r12, gdb_ret
    #endif
    
    // calls GDB stub from main memory
    lwi     r12, r0, SPBMEM_GDBHNDL
    beqi    r12, return_from_int
    _SAVE_GPR(r15, 16)
    _SAVE_GPR(r5, 20)
    _SAVE_GPR(r6, 24)
    lhui    r5, r11, SPB_PACK_SRC_OFF
    lwi     r6, r11, SPB_PACK_DAT0_OFF
    brald   r15, r12
    nop
    _RESTORE_GPR(r15, 16)
    _RESTORE_GPR(r5, 20)
    _RESTORE_GPR(r6, 24)
    bri     return_from_int
#endif
