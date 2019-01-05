#define ENABLE_TRACE

#include <libmalt.h>
#include <stdio.h>
#include <stdlib.h>

#define N 24 //it must be less or equal to 24, 2N + 8 - number of data packets

#define max(n,m) ((n>m) ? (n) : (m))

char hr[] = "---------------------------------------------------------------------\n";
volatile u32_t core;
u32_t cores[4];
u64_t result, results[4], results_seq[max(5, N)];
u32_t delay;

u32_t q1(u32_t i) {
    if (i == 0) return 3;
    if (i == 1) return 5;
    return N;
}

u32_t q2(u32_t i) {
    if (i < 3) return 4;
    if (i < 5) return 3;
    return 2;
}

u64_t q3(u32_t n) {
    u64_t shifter = 0x80000080000000, result = 0xFFffffFFFFffff;
    while (n++ < 24) result ^= shifter, shifter >>= 1;
    return result;
}

void receiver() {
    spbus_packet data;
//step 1
    u64_t sum = 0;
    u32_t i, current_core;
    for (i = 0; i < 2*N + 8; ++i) { //total sum
        while (malt_rec_ic_packet(&data) < 0) ++delay;
        sum += (u64_t)data.data0 + ((u64_t)data.data1 << 32);
//malt_trace_print("%d [%x %x] %llx", i, data.data0, data.data1, sum, (u64_t)data.data1 << 32);
    }
    result = sum;
    malt_ic_notify(IC_CORENUM_SMST);
#if !LEGACY_SPB
//step 2
    while (malt_rec_ic_notification() < 0);
    for (u32_t n = 0; n < 4; n++) { //core filtered sums
        sum = 0;
        current_core = cores[n];
        for (i = 0; i < q1(n); ++i) {
            while (malt_rec_ic_packet_from(&data, current_core) < 0) ++delay;
            sum += data.data0 + ((u64_t)data.data1 << 32);
        }
        results[n] = sum;
    }
    malt_ic_notify(IC_CORENUM_SMST);
//step 3
    while (malt_rec_ic_notification() < 0);
    for (u32_t n = 0; n < max(5, N); n++) { //order filtered sums
        sum = 0;
        for (int i = 0; i < 4; i++) {
            if ((n > 4 && i < 2) || (n > 2 && i < 1) || (n >= N && i > 1)) continue; //just a speed-up
            current_core = cores[i];
            while (malt_rec_ic_packet_from(&data, current_core) < 0) ++delay;
            sum += data.data0 + ((u64_t)data.data1 << 32);
        }
        results_seq[n] = sum;
    }
#endif
}

void sender1() {
//step 1
    for (int i = 0; i < 5; i++)
        malt_send_ic_data(core, 1 << (i + 3), 0);
    malt_ic_notify(IC_CORENUM_SMST);
#if !LEGACY_SPB
//step 2
    while (malt_rec_ic_notification() < 0);
    for (int i = 0; i < 5; i++)
        malt_send_ic_data(core, 1 << (i + 3), 0);
    malt_ic_notify(IC_CORENUM_SMST);
//step 3
    while (malt_rec_ic_notification() < 0);
    for (int i = 0; i < 5; i++)
        malt_send_ic_data(core, 1 << (i + 3), 0);
#endif
}

void sender2() {
//step 1
    for (int i = 0; i < N; i++)
        malt_send_ic_data(core, 1 << (i + 8), 0);
    malt_ic_notify(IC_CORENUM_SMST);
#if !LEGACY_SPB
//step 2
    while (malt_rec_ic_notification() < 0);
    for (int i = 0; i < N; i++)
        malt_send_ic_data(core, 1 << (i + 8), 0);
    malt_ic_notify(IC_CORENUM_SMST);
//step 3
    while (malt_rec_ic_notification() < 0);
    for (int i = 0; i < N; i++)
        malt_send_ic_data(core, 1 << (i + 8), 0);
#endif
}

void sender3() {
//step 1
    for (int i = 0; i < N; i++)
        malt_send_ic_data(core, 0, 1 << i);
    malt_ic_notify(IC_CORENUM_SMST);
#if !LEGACY_SPB
//step 2
    while (malt_rec_ic_notification() < 0);
    for (int i = 0; i < N; i++)
        malt_send_ic_data(core, 0, 1 << i);
    malt_ic_notify(IC_CORENUM_SMST);
//step 3
    while (malt_rec_ic_notification() < 0);
    for (int i = 0; i < N; i++)
        malt_send_ic_data(core, 0, 1 << i);
#endif
}

void wait_for_the_next_step() {
    u32_t msg_cnt = 0;
    while (msg_cnt++ < 4)
        while (malt_rec_ic_notification() < 0);
}

int main() {
    if (malt_get_total_slaves() < 4) {
        printf("At least 4 slaves are required for this test!\n");
        return 1;
    }
    if (N > 24) {
        printf("N is too large!\n");
        return 2;
    }
    printf("Number of packets = %u\n%s", N*2 + 8, hr);
//step 1
    core = malt_start_thread((void*)receiver, 0, 0);
    cores[1] = malt_start_thread((void*)sender1, 0, 0);
    cores[2] = malt_start_thread((void*)sender2, 0, 0);
    cores[3] = malt_start_thread((void*)sender3, 0, 0);
    malt_start_pcnt();
    malt_send_ic_data(core, 1, 0);
    malt_send_ic_data(core, 2, 0);
    malt_send_ic_data(core, 4, 0);
    wait_for_the_next_step();
    malt_stop_pcnt();
    printf("          Total sum = %014llx - ", result);
    if (result == q3(N)) printf("Success"); else printf("Failure");
    printf(", Clock = %llu\n%s", (u64_t)malt_get_sys_stat(BCCMDR_GET_PCNT_HT)*0x100000000LL + malt_get_sys_stat(BCCMDR_GET_PCNT_LT), hr);
#if !LEGACY_SPB
//step 2
    malt_start_pcnt();
    malt_ic_notify(core);
    malt_ic_notify(cores[2]);
    malt_sleep_us(10);;  //iron can work without this delay
    malt_ic_notify(cores[3]);
    malt_ic_notify(cores[1]);
    malt_send_ic_data(core, 4, 0);
    malt_send_ic_data(core, 1, 0);
    malt_send_ic_data(core, 2, 0);
    wait_for_the_next_step();
    malt_stop_pcnt();
    result = 0;
    for (u32_t i = 0; i < 4; i++)
        result ^= results[i], printf("  Core = 0x%03x, Sum = %014llx\n", cores[i], results[i]);
    printf("       Combined sum = %014llx - ", result);
    if (result == q3(N)) printf("Success"); else printf("Failure");
    printf(", Clock = %llu\n%s", (u64_t)malt_get_sys_stat(BCCMDR_GET_PCNT_HT)*0x100000000LL + malt_get_sys_stat(BCCMDR_GET_PCNT_LT), hr);
//step 3
    malt_start_pcnt();
    malt_ic_notify(cores[3]);
    malt_sleep_us(10);;  //iron can work without this delay
    malt_ic_notify(cores[2]);
    malt_ic_notify(cores[1]);
    malt_send_ic_data(core, 4, 0);
    malt_send_ic_data(core, 1, 0);
    malt_send_ic_data(core, 2, 0);
    malt_ic_notify(core);
    malt_wait_all_lines_free();
    malt_stop_pcnt();
    result = 0;
    for (u32_t i = 0; i < max(5, N); i++)
        result ^= results_seq[i], printf("    Order = %02u, Sum = %014llx\n", i, results_seq[i]);
    printf("       Combined sum = %014llx - ", result);
    if (result == q3(N)) printf("Success"); else printf("Failure");
    printf(", Clock = %llu\n", (u64_t)malt_get_sys_stat(BCCMDR_GET_PCNT_HT) * 0x100000000LL + malt_get_sys_stat(BCCMDR_GET_PCNT_LT));
#endif
    return 0;
}

