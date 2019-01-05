/******************************************************************************\
    Misc. MALT interconnect functions
\******************************************************************************/

#include <stdarg.h>
#include <libmalt.h>

u64_t start_clock;
extern u32_t __tls_end;

/*
 * Initialize interconnect system
 */
void malt_ic_init()
{
    int i;
    spbus_packet pack;
    start_clock = malt_get_time_us_64();
    const int lines = malt_get_lines_count();

    malt_trace_func();
#if !LEGACY_SPB
#define MALT_SPBUS_HARD_LIMIT_DEFAULT (u32_t)((spbus_packet*)&__tls_end + SPB_HARD_LIMIT_DEFAULT_PACKETS)
#define MALT_SPBUS_SOFT_LIMIT_DEFAULT (u32_t)((spbus_packet*)&__tls_end + SPB_SOFT_LIMIT_DEFAULT_PACKETS)
    extern u32_t __soft_spb_fixed_data;  //init new interconnect
    __soft_spb_fixed_data = *(&__soft_spb_fixed_data + 1) = (u32_t)&__tls_end;
    *(&__soft_spb_fixed_data + 2) = 0;
    *(&__soft_spb_fixed_data + 3) = MALT_SPBUS_HARD_LIMIT_DEFAULT;
    *(&__soft_spb_fixed_data + 4) = MALT_SPBUS_SOFT_LIMIT_DEFAULT;
#endif

    for (i = 1; i <= lines; ++i)
    {
        pack.type = IC_SPB_STYPE_TSTART;
        pack.target = malt_mst_on_line(i);
        #if LEGACY_SPB
        void malt_mst_shepherd_tasic();
        pack.data1 = (u32_t)malt_mst_shepherd_tasic;
        #else
        pack.data1 = (u32_t)malt_mst_shepherd;
        pack.data0 = (u32_t)&__tls_end;
        #endif
        malt_spbus_send(&pack);
    }
    malt_set_interrupt_mask(malt_get_interrupt_mask() | INTMSK_INTERCON); //init new interconnect
    malt_enable_interrupts();
    malt_start_pcnt();
    #if !TASIC
    malt_spbus_uart_setbitrate(115200);
    #endif
}

/*
* Change IC buffer size
*/

static volatile u32_t change_ic_buf_size_sync, change_ic_buf_size_sync2;

static int log2i(u32_t n) {
    int r = 0;
    if (n == 0) return -2;
    for (u32_t i = 0; i < 32; i++)
       if ((n & 1) == 0)
           n >>= 1, ++r;
       else {
           n >>= 1;
           break;
       }
    if (n != 0) return -1; //error, size is not the power of 2
    return r;
}

static void change_ic_buf_size_master(spbus_packet *pack) {
    int icbs = malt_get_spb_rcvbuf_size();
    if (icbs != 4 << pack->data1 && malt_is_master()) {
        malt_disable_interrupts();
        malt_write_reg(BUSC_SPB_ICBS_ADDR, pack->data1);
        asmi("addk r1, r0, %0"::"r"(SPB_MEM_BASE_ADDR + malt_get_local_mem_size() - 4):);
        malt_enable_interrupts();
        change_ic_buf_size_sync = icbs;
        for(;;);
    }
    change_ic_buf_size_sync = icbs;
}

int malt_ic_set_buffer_size_masters(u32_t size) { //size in packets
    int log2 = log2i(size);
    if (log2 < 0) return -1;
    for (int line = 1; line <= malt_get_lines_count(); ++line) {
        spbus_packet pack;
        malt_set_mst_callback(line, &change_ic_buf_size_master);
        change_ic_buf_size_sync = 0;
        malt_trg_mst_callback(line, log2 + 2);
        while (change_ic_buf_size_sync == 0);
        if (change_ic_buf_size_sync != size << 4) {
            pack.type = IC_SPB_STYPE_TSTART;
            pack.target = malt_mst_on_line(line);
            pack.data1 = (u32_t)malt_mst_shepherd;
            pack.data0 = (u32_t) &__tls_end;
            malt_spbus_send(&pack);
            malt_trace_print("IC buffer size has changed to %d bytes at master 0x%04x\n", 4 << log2 + 2, line << 8);
        }
    }
    return 1;
}

static void change_ic_buf_size_slave() {
    spbus_packet pack;
    change_ic_buf_size_sync = 0;
    while (malt_rec_ic_packet(&pack) < 0);
    malt_disable_interrupts();
    malt_write_reg(BUSC_SPB_ICBS_ADDR, pack.data0);
    asmi("addk r1, r0, %0"::"r"(SPB_MEM_BASE_ADDR + malt_get_local_mem_size() - 4):);
    malt_trace_print("IC buffer size has changed to %d bytes at slave 0x%04x\n", 4 << pack.data0, malt_cur_core_num());
    ((void(*)(void))0)(); //restart
}

static void change_ic_buf_size_on_slaves(spbus_packet* ppack) {
    spbus_packet pack;
    for (int core = 1; core <= malt_get_slaves_count(); ++core) {
        pack.type = IC_SPB_STYPE_TSTART;
        pack.target = ppack->target + core;
        pack.data1 = (u32_t)change_ic_buf_size_slave;
        pack.data0 = (u32_t) &__tls_end;
        change_ic_buf_size_sync = 1;
        malt_spbus_send(&pack);
        while (change_ic_buf_size_sync);
        malt_send_ic_data(ppack->target + core, ppack->data1, 0);
    }
    change_ic_buf_size_sync2 = 0;
}

int malt_ic_set_buffer_size_slaves(u32_t size) { //size in packets
    int log2 = log2i(size);
    if (log2 < 0) return -1;
    for (int line = 1; line <= malt_get_lines_count(); ++line) {
        malt_set_mst_callback(line, &change_ic_buf_size_on_slaves);
        change_ic_buf_size_sync2 = 1;
        malt_trg_mst_callback(line, log2 + 2);
        while(change_ic_buf_size_sync2);
    }
//    malt_sleep_us(10);
    return 1;
}

int malt_ic_set_buffer_size_sm(u32_t size) { //size in packets
    int log2 = log2i(size);
    if (log2 < 0) return -1;
    if (malt_get_spb_rcvbuf_size() != size << 4 && malt_is_smaster()) {
        malt_write_reg(BUSC_SPB_ICBS_ADDR, log2 + 2);
        malt_trace_print("IC buffer size has changed to %d bytes at supermaster\n", 4 << log2 + 2);
        //if (!malt_core_is_master(malt_cur_core_num())) //we have to check a case when the stack is moved to local mem at SM
        //    asmi("addk r1, r1, %0"::"r"(size):);  //should we copy the stack here?
    }
    return 1;
}

static void get_ic_buf_size_master(spbus_packet *pack) {
    malt_send_ic_data(pack->data1, malt_get_sys_stat(BCCMDR_GET_RBUFSZ) >> 4, 0);
}

static volatile int get_ic_buf_size_sync;

static void get_ic_buf_size_slave() {
    spbus_packet pack;
    get_ic_buf_size_sync = 0;  //FIXME! It is not atomic but it almost always should work
    while (malt_rec_ic_packet(&pack) < 0);
    malt_send_ic_data(pack.data0, malt_get_sys_stat(BCCMDR_GET_RBUFSZ) >> 4, 0);
}

static void get_ic_buf_size_slave_helper(spbus_packet *pack) {
    spbus_packet data;
    data.type = IC_SPB_STYPE_TSTART;
    data.target = pack->data1;
    data.data1 = (u32_t)get_ic_buf_size_slave;
    data.data0 = (u32_t) &__tls_end;
    get_ic_buf_size_sync = 1;  //FIXME! It is not atomic but it almost always should work
    malt_spbus_send(&data);
    while (get_ic_buf_size_sync);
    malt_send_ic_data(pack->data1, pack->source, 0);
}

int malt_get_ic_buf_size(u32_t core) {  //returns size in pockets
    spbus_packet data;
    if (malt_core_line_num(core) > malt_get_lines_count() || malt_slv_num(core) > malt_get_slaves_count())
        return -2;
    if (core == malt_cur_core_num())
        return malt_get_sys_stat(BCCMDR_GET_RBUFSZ) >> 4;
    else if (malt_core_is_master(core)) {
        malt_set_mst_callback(malt_core_line_num(core), get_ic_buf_size_master);
        malt_trg_mst_callback(malt_core_line_num(core), malt_cur_core_num());
        while (malt_rec_ic_packet(&data) < 0);
        return data.data0;
    }
    else if (malt_core_is_slave(core)) {
        malt_set_mst_callback(malt_core_line_num(core), get_ic_buf_size_slave_helper);
        malt_trg_mst_callback(malt_core_line_num(core), core);
        while (malt_rec_ic_packet(&data) < 0);
        return data.data0;
    }
    else
        return -1;
}

/*
 * Get master state
 */
u32_t malt_get_master_state(int line)
{
    //Unused? u32_t data;
    u32_t mst = malt_mst_on_line(line);

    malt_send_ic_msg(mst, IC_REQ_STATUS | mst);
    //while (malt_rec_ic_msg(&data, NULL) < 0);
    spbus_packet pack;
    //printf("state %X\n", mst);
    while (malt_rec_ic_packet(&pack) < 0);
    //printf("a %X\n", pack.data0);

    return pack.data0;
}

/*
 * Check if all slave are free in register
 */
int malt_all_slaves_free_reg(u64_t reg)
{
    return ((!reg || (__builtin_ctzll(reg) > malt_get_slaves_count())) ? 1 : 0);
}


void malt_set_slv_busy_reg(slv_busy_t reg)
{
    malt_write_reg(SPBMEM_SLVBUSY, (u32_t)(reg & 0xFFFFFFFF));
    malt_write_reg(SPBMEM_SLVBUSY1, (u32_t)((reg >> 32) & 0xFFFFFFFF));
}

