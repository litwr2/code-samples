/******************************************************************************\
    MALT functions for help with debugging
\******************************************************************************/

#include <libmalt.h>
#include <stdio.h>
#include <string.h>

#if !LEGACY_SPB
/*
* Print ic soft buffer plus extent optional buffer's slots
*/
void malt_soft_ic_buffer_show(char *debugger_buffer, int extent) {
    extern u32_t __soft_spb_fixed_data, __tls_end;
    u32_t *p = &__tls_end;
    sprintf(debugger_buffer + strlen(debugger_buffer), "soft ic buffer (core = 0x%03lX): start = %lu end = %lu\n",
        malt_cur_core_num(), (((&__soft_spb_fixed_data)[1]) - (u32_t)&__tls_end) >> 4,
        (__soft_spb_fixed_data - (u32_t)&__tls_end) >> 4);
    for (int i = 0; i < ((__soft_spb_fixed_data - (u32_t)&__tls_end) >> 4) + extent; i++)
        sprintf(debugger_buffer + strlen(debugger_buffer),
            "%2d %8lx %8lx %8lx %8lx\n", i, p[i*4], p[i*4 + 1], p[i*4 + 2], p[i*4 + 3]);
}
#endif

/*
* Print ic hard buffer
*/
void malt_hard_ic_buffer_show(char *debugger_buffer) {
    spbus_packet *t = (void*)malt_get_spb_rcvbuf();
    u32_t bsz = malt_get_spb_rcvbuf_size(), cnt = malt_read_reg(BUSC_SPB_RCNT_ADDR), hwcnt = malt_read_reg(BUSC_SPB_HCNT2_ADDR);
    sprintf(debugger_buffer + strlen(debugger_buffer), "hard ic buffer (core = 0x%03lX) at %p size = %lu: end = %lu (%lu), hw = %lx (%lu)\n",
        malt_cur_core_num(), t, bsz, cnt, cnt%(bsz/sizeof(spbus_packet)), hwcnt, hwcnt%bsz >> 4);
    for (int i = 0; i < bsz/sizeof(spbus_packet); i++) {
        u32_t *p = (void*)(t + i);
        sprintf(debugger_buffer + strlen(debugger_buffer), "%3d %8lx %8lx %8lx %8lx\n", i, p[0], p[1], p[2], p[3]);
    }
}

