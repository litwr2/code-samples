#include <libmalt.h>
#include <stdio.h>
#define IC_BUFFER_SZ_LOG2_MIN 4 //should be more than 4 if PRINT_IC_BUFFERS != 0
#define IC_BUFFER_SZ_LOG2_MAX 10
#define ACK_ARG 70
#define PRINT_IC_BUFFERS 0

volatile u32_t LSP, STACKSZ, cnt, synchro, saved_BCCMDR_GET_RBUFSZ, saved_BUSC_SPB_ICBS_ADDR;

void slave_task() {
    asmi("  addk %0,r0,r1":"=r"(LSP)::);
    STACKSZ = malt_get_spb_rcvbuf() - LSP;
    saved_BCCMDR_GET_RBUFSZ = malt_get_sys_stat(BCCMDR_GET_RBUFSZ);
    saved_BUSC_SPB_ICBS_ADDR = malt_read_reg(BUSC_SPB_ICBS_ADDR);
}

void master_task(spbus_packet *pack) {
    asmi("  addk %0,r0,r1":"=r"(LSP)::);
    STACKSZ = malt_get_spb_rcvbuf() - LSP;
    saved_BCCMDR_GET_RBUFSZ = malt_get_sys_stat(BCCMDR_GET_RBUFSZ);
    saved_BUSC_SPB_ICBS_ADDR = malt_read_reg(BUSC_SPB_ICBS_ADDR);
}

long ack(long n, long x, long y) {
   if (n == 0) return y + 1;
   if (y == 0)
      switch (n) {
         case 1: return x;
         case 2: return 0;
         default: return 1;
      }
   return ack(n - 1, x, ack(n, x, y - 1));
}

void send_pack_to_sm(spbus_packet *pack) {
    malt_send_ic_data(0, ack(1, 1, ACK_ARG), 8);
}

void send_pack_to_sm_from_slave() {
    while (synchro);
    malt_start_pcnt();
    while (malt_get_sys_stat(BCCMDR_GET_PCNT_LT) < malt_seq_core_num(malt_cur_core_num())*1000);
    malt_stop_pcnt();
    malt_send_ic_data(0, ack(1, 1, ACK_ARG), 11);
}

#if PRINT_IC_BUFFERS
char debugger_buffer[40000];
void print_ic_buffer(spbus_packet* pack) {
    malt_hard_ic_buffer_show(debugger_buffer);
    malt_ic_notify(IC_CORENUM_SMST);
}
#endif

int main() {
    u32_t SP, sum, errst = 0;
    spbus_packet data;

    u32_t lines = malt_get_lines_count();
    u32_t cores = malt_get_slaves_count();

    printf("BCCMDR_GET_RBUFSZ = %u  ", malt_get_sys_stat(BCCMDR_GET_RBUFSZ));
    printf("BUSC_SPB_ICBS_ADDR = %u\n", malt_read_reg(BUSC_SPB_ICBS_ADDR));
    asmi("  addk %0, r0, r1":"=r"(SP)::);
    printf("SP (super master) = %x\n", SP);
    malt_set_mst_callback(1, master_task);
    malt_trg_mst_callback(1, 7);
    malt_sleep_us(100);
    printf("SP (master) = %x  stack data size = %x\n", LSP, STACKSZ);
    malt_start_thread((void*)slave_task, 0, 0);
    malt_sleep_us(100);
    printf("SP (slave) = %x  stack data size = %x\n", LSP, STACKSZ);

    for (int IC_BUFFER_SZ_LOG2 = IC_BUFFER_SZ_LOG2_MIN; IC_BUFFER_SZ_LOG2 <= IC_BUFFER_SZ_LOG2_MAX; IC_BUFFER_SZ_LOG2++) {
        int IC_BUFFER_SZ_PACKS = 1 << (IC_BUFFER_SZ_LOG2 - 2);

        if (IC_BUFFER_SZ_LOG2 < 2 || IC_BUFFER_SZ_LOG2 > 10) {
            printf("\nRequested IC buffer size is out of range\n");
            continue;
        }
        malt_ic_set_buffer_size_sm(IC_BUFFER_SZ_PACKS);
        printf("\nBCCMDR_GET_RBUFSZ = %u  ", saved_BCCMDR_GET_RBUFSZ = malt_get_sys_stat(BCCMDR_GET_RBUFSZ));
        printf("BUSC_SPB_ICBS_ADDR = %u\n", saved_BUSC_SPB_ICBS_ADDR = malt_read_reg(BUSC_SPB_ICBS_ADDR));
        if (saved_BCCMDR_GET_RBUFSZ != IC_BUFFER_SZ_PACKS << 4 || saved_BUSC_SPB_ICBS_ADDR != IC_BUFFER_SZ_LOG2 || saved_BCCMDR_GET_RBUFSZ >> 4 != malt_get_ic_buf_size(IC_CORENUM_SMST)) {
            printf("Buffer size (%d = 2^(%d+2)) bytes is set incorrectly at Supermaster\n", IC_BUFFER_SZ_PACKS << 4, saved_BUSC_SPB_ICBS_ADDR);
            errst = 1;
        }
        asmi("  addk %0, r0, r1":"=r"(SP)::);
        printf("SP (super master) = %x\n", SP);

        malt_ic_set_buffer_size_masters(IC_BUFFER_SZ_PACKS);
        malt_set_mst_callback(1, master_task);
        malt_trg_mst_callback(1, 7);
        malt_sleep_us(100);
        if (saved_BCCMDR_GET_RBUFSZ != IC_BUFFER_SZ_PACKS << 4 || saved_BUSC_SPB_ICBS_ADDR != IC_BUFFER_SZ_LOG2 || saved_BCCMDR_GET_RBUFSZ >> 4 != malt_get_ic_buf_size(0x100)) {
            printf("Buffer size (%d = 2^(%d+2)) bytes is set incorrectly at Master(s)\n", IC_BUFFER_SZ_PACKS << 4, saved_BUSC_SPB_ICBS_ADDR);
            errst = 1;
        }
        printf("SP (master) = %x  stack data size = %x\n", LSP, STACKSZ);

        malt_ic_set_buffer_size_slaves(IC_BUFFER_SZ_PACKS);
        malt_start_thread((void*)slave_task, 0, 0);
        malt_sleep_us(200);
        if (saved_BCCMDR_GET_RBUFSZ != IC_BUFFER_SZ_PACKS << 4 || saved_BUSC_SPB_ICBS_ADDR != IC_BUFFER_SZ_LOG2 || saved_BCCMDR_GET_RBUFSZ >> 4 != malt_get_ic_buf_size(0x101)) {
            printf("Buffer size (%d = 2^(%d+2)) bytes is set incorrectly at Slave(s)\n", IC_BUFFER_SZ_PACKS << 4, saved_BUSC_SPB_ICBS_ADDR);
            errst = 1;
        }
        printf("SP (slave) = %x  stack data size = %x\n\n", LSP, STACKSZ);

        if (errst) continue;

        printf("All cores calculate Ackermann's function at (1, 1, %d) = %d\n", ACK_ARG, ack(1, 1, ACK_ARG));
        for (int i = 1; i <= lines; ++i) {
            malt_set_mst_callback(i, &send_pack_to_sm);
            malt_trg_mst_callback(i, 10);
        }
        sum = 0;
        for (int i = 0; i < lines; i++) {
            cnt = 0;
            while (malt_rec_ic_packet(&data) < 0 && ++cnt < ack(1, 1, ACK_ARG) << 5);
            if (data.data0 == ack(1, 1, ACK_ARG) && data.data1 == 8) sum++;
        }
        if (sum == lines)
            puts("All packets are received from masters");
        else
            errst = 1, printf("Fail! %d of %d packages are received from masters\n", sum, lines);
        synchro = 1;
        malt_start_all_slaves((void*)send_pack_to_sm_from_slave);
        sum = 0;
        synchro = 0;
        for (int i = 0; i < cores*lines; i++) {
            cnt = 0;
            while (malt_rec_ic_packet(&data) < 0 && ++cnt < ack(1, 1, ACK_ARG) << 7);
            if (data.data0 == ack(1, 1, ACK_ARG) && data.data1 == 11) sum++;
        }
        malt_sleep_us(500);
        if (sum == cores*lines)
            puts("All packets are received from slaves"); 
        else
            errst = 1, printf("Fail! %d of %d packages are received from slaves\n", sum, cores*lines);
#if PRINT_IC_BUFFERS
        debugger_buffer[0] = 0;
        malt_set_mst_callback(1, &print_ic_buffer);
        malt_trg_mst_callback(1, 0);
        while (malt_rec_ic_notification() < 0);
        printf("%s\n", debugger_buffer);

        debugger_buffer[0] = 0;
        malt_start_thread_line(1, (void*)print_ic_buffer, 0, 0);
        while (malt_rec_ic_notification() < 0);
        printf("%s\n", debugger_buffer);

        debugger_buffer[0] = 0;
        malt_hard_ic_buffer_show(debugger_buffer);
        printf("%s\n", debugger_buffer);
#endif
    }
    if (errst) puts("There were errors!"); else puts("The test has passed successfully");
    return errst;
}

