/******************************************************************************\
    MALT extended memory controller functions
\******************************************************************************/


#include <libmalt.h>

/* 
* it allows to use a limited number of MC operations when interrupts are disabled
*/
void malt_mc_do_if_nointerrupts() {
     if (!malt_get_interrupts_state()) {
         u32_t hwcnt = malt_read_reg(BUSC_SPB_HCNT2_ADDR)%malt_get_sys_stat(BCCMDR_GET_RBUFSZ);
         while ((malt_read_reg(BUSC_SPB_RCNT_ADDR) << 4)%malt_get_sys_stat(BCCMDR_GET_RBUFSZ) != hwcnt) {
             if (((spbus_packet*)(hwcnt + malt_get_spb_rcvbuf()))->type == SPB_TYPE_DMM_READ) {
                 ((spbus_packet*)(hwcnt + malt_get_spb_rcvbuf()))->type = SPB_TYPE_INVALID;
                 malt_write_reg(BUSC_SPB_RPEND_ADDR, 0);
             }
             else
                 hwcnt = (hwcnt + 16)%malt_get_sys_stat(BCCMDR_GET_RBUFSZ);
         }
     }
}

/*
 * Read contents of memory cell at address addr
 * and block this cell with EMPTY-bit
 */
u32_t malt_read_modify(void* addr)
{
    malt_extended_mem_op(SMC_READ_MODIFY, 4, (u32_t)(addr), 0, SPBMEM_SPB_DAT1);
    
    // wait for reply packet
    //spbus_packet ic_pack;
    //while (malt_spbus_recv(&ic_pack, SPB_TYPE_DMM_READ) < 0);
    malt_wait_breads_ready();
    
    return malt_read_reg(SPBMEM_SPB_DAT1);
}


/*
 * Write data to memory cell at address addr
 * and reset EMPTY-bit
 */
void malt_fill_write(register volatile void* addr,register volatile u32_t data)
{
    malt_extended_mem_op(SMC_WRITE_UNBLOCK, 0, (u32_t)(addr), (u32_t)(data), 0);
}


/*
 * Atomic increment
 */
u32_t malt_ext_memop_inc(void* addr, i16_t data)
{
//    malt_write_reg(SPBMEM_SPB_DAT1+4, 0xaab);
    malt_extended_mem_op(SMC_INCREMENT, data, (u32_t)(addr), 0, SPBMEM_SPB_DAT1);
    
    // wait for reply packet
    //Required? spbus_packet ic_pack;
    malt_wait_breads_ready();
    
//    printf("%X %X %x %x\n", malt_read_reg(SPBMEM_SPB_DAT1), malt_read_reg(SPBMEM_SPB_DAT1+4), malt_read_reg(SPBMEM_SPB_DAT1+8), malt_read_reg(SPBMEM_SPB_DAT1+12));
//static char db[10000];db[0]=0;malt_hard_ic_buffer_show(db);puts(db);
    
    return malt_read_reg(SPBMEM_SPB_DAT1);
}


/*
 * Perform extended operation of MALT memory controller
 */
void malt_extended_mem_op(u32_t type, u32_t size, u32_t glob_addr, u32_t wrd1, u32_t laddr)
{
    //printf("type %04X size %08X w0 %08X w1 %08X la %08X\n", type, size, glob_addr, wrd1, laddr);
    spbus_packet pack;
    
    pack.type = type;
    pack.data0 = glob_addr & SMEM_ADDR_MASK;
    if (glob_addr & 0x40000000)
    {
	    pack.target = (malt_cur_core_num() & 0xFF00) + 0xFF;
    }
	else
	    pack.target = SPB_ADDR_MCTRL;
        
    // Size
    malt_write_reg(BUSC_SPB_SRC_ADDR, (size << 16) | malt_cur_core_num());
    
    if (type & SPB_REG_RE_MSK)
    {
        malt_spbus_read(&pack, laddr);
    }
    else
    {
        pack.data1 = wrd1;
        malt_spbus_send(&pack);
    }
        
}

#if 0
void malt_ext_memop_blockread(u32_t laddr, u32_t glob_addr, u32_t size)
{
    malt_write_reg(BUSC_SPB_PTYPE_ADDR, SMC_BLOCK_READ);
    malt_write_reg(BUSC_SPB_DAT0_ADDR, glob_addr & SMEM_ADDR_MASK);
    malt_write_reg(BUSC_SPB_SRC_ADDR, (size*4 * 0x10000) | malt_cur_core_num()); // same as (size << (SPB_TARGET_BITS+2)), but uses HW-mul
    malt_write_reg(BUSC_SPB_LADDR_ADDR, laddr);
    malt_write_reg(BUSC_SPB_TRGT_ADDR, SPB_ADDR_MCTRL);
}
#endif


// Zero bss using extended memory controller op
void malt_bss_zero()
{
    extern u32_t __bss_start;
    extern u32_t __bss_end;
    malt_ext_memop_memset(&__bss_start, 0, ((u32_t)&__bss_end - (u32_t)&__bss_start));
}


void *malt_memcpy_aligned(void *s1, const void *s2, register size_t n)
{
    register const u32_t *src = s2;
    register u32_t *dst = s1;
    
    // Copy large chunks
    for (; n >= SMC_OP_MAX_BYTES; n -= SMC_OP_MAX_BYTES)
    {
        malt_ext_memop_memcpy(((u32_t)dst)+malt_read_reg(BUSC_DMEMSEG_ADDR), ((u32_t)src)+malt_read_reg(BUSC_DMEMSEG_ADDR), SMC_OP_MAX_BYTES);
        dst += (SMC_OP_MAX_WORDS);
        src += (SMC_OP_MAX_WORDS);
    }
    
    // Copy small chunks
    for (; n >= SMC_OP_MIN_BYTES; n -= SMC_OP_MIN_BYTES)
    {
        malt_ext_memop_memcpy(((u32_t)dst)+malt_read_reg(BUSC_DMEMSEG_ADDR), ((u32_t)src)+malt_read_reg(BUSC_DMEMSEG_ADDR), SMC_OP_MIN_BYTES);
        dst += (SMC_OP_MIN_WORDS);
        src += (SMC_OP_MIN_WORDS);
    }
    
    for (; n >= 4; n -= 4)
        *dst++ = *src++;
    
    return s1;
}


void *malt_memcpy(void *s1, const void *s2, register size_t n)
{
    const char *src = s2;
    char *dst = s1;

    const u32_t *i_src;
    u32_t *i_dst;
    size_t rem_bytes;
    
    if (((u32_t)s1 & (SMC_OP_ALIGNMENT-1)) || ((u32_t)s2 & (SMC_OP_ALIGNMENT-1))) {
        void *memcpy(void *dest, const void *src, size_t n);
        return memcpy(s1, s2, n);   // call simple memcopy in case of non alligned request
    }

    if (/*likely*/(n >= 4)) {
        unsigned  value, buf_hold;

        /* Align the destination to a word boundary. */
        switch ((unsigned long)dst & 3) {
        case 1:
            *dst++ = *src++;
            --n;
        case 2:
            *dst++ = *src++;
            --n;
        case 3:
            *dst++ = *src++;
            --n;
        }

        i_dst = (void *)dst;

        /* Choose a copy scheme based on the source */
        /* alignment relative to destination. */
        switch ((unsigned long)src & 3) {
        case 0x0:    /* Both byte offsets are aligned - use malt_ext_memop_memcpy*/
            i_src  = (const void *)src;

            // Copy large chunks
            for (; n >= SMC_OP_MAX_BYTES; n -= SMC_OP_MAX_BYTES)
            {
                //printf("mcpy %08X %08X %08X %X\n", i_src, i_dst, malt_read_reg(BUSC_DMEMSEG_ADDR), SMC_OP_MAX_BYTES);
                malt_ext_memop_memcpy(((u32_t)i_dst)+malt_read_reg(BUSC_DMEMSEG_ADDR), ((u32_t)i_src)+malt_read_reg(BUSC_DMEMSEG_ADDR), SMC_OP_MAX_BYTES);
                i_dst += (SMC_OP_MAX_BYTES / 4);
                i_src += (SMC_OP_MAX_BYTES / 4);
            }
            
            rem_bytes = n - (n & (SMC_OP_MIN_BYTES - 1));
            
            // Copy remaining words    
            if (rem_bytes >= SMC_OP_MIN_BYTES)
            {
                //printf("mcpy %08X %08X %X\n", i_src, i_dst, rem_bytes);
                malt_ext_memop_memcpy(((u32_t)i_dst)+malt_read_reg(BUSC_DMEMSEG_ADDR), ((u32_t)i_src)+malt_read_reg(BUSC_DMEMSEG_ADDR), rem_bytes);
                i_dst += (rem_bytes >> 2);
                i_src += (rem_bytes >> 2);
            }

            n = n - rem_bytes;
            for (; n >= 4; n -= 4)
                *i_dst++ = *i_src++;
            
            src  = (const void *)i_src;
            break;
        case 0x1:    /* Unaligned - Off by 1 */
            /* Word align the source */
            i_src = (const void *) ((unsigned)src & ~3);

            /* Load the holding buffer */
            buf_hold = *i_src++ << 8;

            for (; n >= 4; n -= 4) {
                value = *i_src++;
                *i_dst++ = buf_hold | value >> 24;
                buf_hold = value << 8;
            }

            /* Realign the source */
            src = (const void *)i_src;
            src -= 3;
            break;
        case 0x2:    /* Unaligned - Off by 2 */
            /* Word align the source */
            i_src = (const void *) ((unsigned)src & ~3);

            /* Load the holding buffer */
            buf_hold = *i_src++ << 16;

            for (; n >= 4; n -= 4) {
                value = *i_src++;
                *i_dst++ = buf_hold | value >> 16;
                buf_hold = value << 16;
            }

            /* Realign the source */
            src = (const void *)i_src;
            src -= 2;
            break;
        case 0x3:    /* Unaligned - Off by 3 */
            /* Word align the source */
            i_src = (const void *) ((unsigned)src & ~3);

            /* Load the holding buffer */
            buf_hold = *i_src++ << 24;

            for (; n >= 4; n -= 4) {
                value = *i_src++;
                *i_dst++ = buf_hold | value >> 8;
                buf_hold = value << 24;
            }

            /* Realign the source */
            src = (const void *)i_src;
            src -= 1;
            break;
        }
        dst = (void *)i_dst;
    }

    /* Finish off any remaining bytes */
    /* simple fast copy, ... unless a cache boundary is crossed */
    switch (n) {
    case 3:
        *dst++ = *src++;
    case 2:
        *dst++ = *src++;
    case 1:
        *dst++ = *src++;
    }

    return s1;
}

u32_t malt_get_interrupts_state() {  //FIXME! is it a right place for this function???
    u32_t r;
    asmi("  nop\n\t\
               mfs %0,rmsr":"=r"(r)::);
    return r & 2;
}

