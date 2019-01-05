//#define ENABLE_TRACE

#include <libmalt.h>
#include <stdio.h>

#define NUMBER_OF_READS 2
#define NUMBER_OF_RM 4
#define CONTEXTS 10
#define MAXCORES 100
#define MAXFEBITS 4  //it should be between 1 and 4
#define FEBIT_ALIGN 1

volatile int fe_test_arr[] __attribute__((aligned (16))) = {0xAAAA, 0xBBBB, 0xCCCC, 0xDDDD, 0xBBBB, 0xCCCC, 0xDDDD, 0xAAAA0, 0xCCCC, 0xDDDD, 0xAAAA0, 0xBBBB, 0xEEEE, 0xFFFF};
#define  fe_test ((volatile int*)fe_test_arr)

#define WAYTOEXIT 1
#if WAYTOEXIT==0
#define ERROR(n) printf("ERROR #%d!\n", n)
#elif WAYTOEXIT==1
#define ERROR(n) {printf("ERROR #%d!\n", n);return n;}
#else 
#define ERROR(n) for(;;)
#endif

static int values_to_write[] = {0xBEEF, 0xF00D, 0xDACE, 0xFEED};
static volatile int *fe_func_arg, fe_no, cores[CONTEXTS][MAXCORES][4];
static int eventno;

void add_event(u32_t value, u32_t legend, u32_t addr) {
    malt_trace_func();
    int context = 0, coreseqnum = malt_seq_core_num(malt_cur_core_num());
    while (cores[context][coreseqnum][2] != 0) context++;
    cores[context][coreseqnum][0] = malt_cur_core_num();
    cores[context][coreseqnum][1] = value;
    cores[context][coreseqnum][2] = legend;
    cores[context][coreseqnum][3] = addr;
}

int fe_test_func_r() {
    spbus_packet data;

    malt_trace_func();
    while (malt_rec_ic_packet(&data) < 0);
    u32_t test = *(volatile int*)data.data1;
    malt_trace_print("Slave unblocked");
    add_event(test, (u32_t)"R", data.data0);
    return 0;
}

int fe_test_func_rm() {
    spbus_packet data;

    malt_trace_func();
    while (malt_rec_ic_packet(&data) < 0);
    u32_t test = malt_read_modify((void*)data.data1);
    malt_trace_print("Slave read %X", test);
    add_event(test, (u32_t)"RM", data.data0);
    return 0;
}

int print_events(u32_t value) {
    malt_trace_func();
    int context, qs = malt_get_slaves_count(), ql = malt_get_lines_count(), noread = 0, norm = 0, err = 0;
    for (int i1 = 0; i1 <= ql; i1++)
        for (int i2 = 0; i2 <= qs; i2++) {
            int qc;
            if (i1 == 0 && i2 > 0) continue;
            qc = malt_seq_core_num((i1 << IC_LINENUM_SHIFT) + i2);
            context = 0;
            while (cores[context][qc][2] != 0) {
                if (cores[context][qc][1] != value)
                    err = 1;
                if (((char*)cores[context][qc][2])[1] == 0)
                    noread++;
                else
                    norm++;
                printf("Event %d. Core 0x%04X has sent %04X (type %s) from addr=%d\n",
                    eventno++, cores[context][qc][0], cores[context][qc][1],
                    (char*)cores[context][qc][2], cores[context][qc][3]);
                cores[context][qc][2] = 0;
                context++;
            }
        }
    if (err) return -1;
    return norm*256 + noread;
}

int main() {
    malt_enable_interrupts();
    malt_set_interrupt_mask(INTMSK_INTERCON);
    int test, reads;
    if (malt_get_slaves_count()*malt_get_lines_count() < (NUMBER_OF_READS + NUMBER_OF_RM)*MAXFEBITS) {
        puts("Not enough slaves");
        return 10;
    }
    if (FEBIT_ALIGN != 1 && FEBIT_ALIGN != 2 && FEBIT_ALIGN != 4) {
        puts("Wrong align");
        return 10;
    }
    printf("Starting Full-Empty bit test\n");
    for (fe_no = 0; fe_no < MAXFEBITS; fe_no++) {
        u32_t saved_value;
        fe_func_arg = fe_test + fe_no*FEBIT_ALIGN;
        saved_value = *fe_func_arg;
        printf("Trying to read-modify address %x (%d)...\n", fe_func_arg, fe_no);
        add_event(malt_read_modify((void*)fe_func_arg), (u32_t)"RM", fe_no);
        test = print_events(saved_value);
        if (test >> 8 != 1 || (test & 0xff) != 0)
            ERROR(5);
    }
    for (fe_no = 0; fe_no < MAXFEBITS; fe_no++) {
        fe_func_arg = fe_test + fe_no*FEBIT_ALIGN;
        printf("Launching %d slave threads - slaves should block at addr=%d...\n", NUMBER_OF_READS, fe_no);
        for (int i = 0; i < NUMBER_OF_READS; i++) {
            int core;
            if ((core = malt_start_thread(&fe_test_func_r, 0, 0)) < 0) {
                puts("Can't start this thread");
                ERROR(2);
            }
            malt_send_ic_data(core, fe_no, (u32_t)fe_func_arg);
        }
    }
    printf("Sleeping for 0.1ms...\n");
    malt_sleep_us(100);

    if (malt_all_lines_free()) {
        printf("Slaves are not blocked - bad :(. Stoping program\n");
        ERROR(3);
    }

    printf("Slaves are still blocked - good :)\n");
    for (fe_no = 0; fe_no < MAXFEBITS; fe_no++) {
        u32_t saved_value;
        fe_func_arg = fe_test + fe_no*FEBIT_ALIGN;
        printf("Writing %04X then reading from cell %d...\n", values_to_write[fe_no], fe_no);

        malt_fill_write(fe_func_arg, values_to_write[fe_no]);
        malt_sleep_us(200);
        saved_value = *fe_func_arg;
        printf("Standart read at addr=%d got %X\n", fe_no, saved_value);
        test = print_events(saved_value);
        if (test < 0 || test >> 8 != 0 || (test & 0xff) != NUMBER_OF_READS)
           ERROR(5);
    }
    printf("All reader slaves were unblocked - good :). SuperMaster has %X\n", *fe_func_arg);
    if (values_to_write[fe_no - 1] != *fe_func_arg)
        ERROR(4);
    puts("\nTesting read-modify from slaves");
    printf("Launching %d slave threads (%d R + %d RM) - slaves should block...\n",
        NUMBER_OF_READS + NUMBER_OF_RM, NUMBER_OF_READS, NUMBER_OF_RM);
    reads = MAXFEBITS*NUMBER_OF_READS;
    for (fe_no = 0; fe_no < MAXFEBITS; fe_no++) {
        int core;
        fe_func_arg = fe_test + fe_no*FEBIT_ALIGN;
        for (int i = 0; i < NUMBER_OF_RM; i++)
            if ((core = malt_start_thread(&fe_test_func_rm, 0, 0)) < 0) {
                puts("Can't start this thread for a rm");
                return 2;
            }
            else
                malt_send_ic_data(core, fe_no, (u32_t)fe_func_arg);
        malt_sleep_us(200);
        for (int i = 0; i < NUMBER_OF_READS; i++)
            if ((core = malt_start_thread(&fe_test_func_r, 0, 0)) < 0) {
                puts("Can't start this thread for a read");
                return 2;
            }
            else
                malt_send_ic_data(core, fe_no, (u32_t)fe_func_arg);
        malt_sleep_us(400);
        test = print_events(values_to_write[fe_no]);
        if (test < 0 || test >> 8 != 1 || (test & 0xff) > 0)
            ERROR(5);
    }
    for (fe_no = 0; fe_no < MAXFEBITS; fe_no++) {
        int rm_prev = 1;
        fe_func_arg = fe_test + fe_no*FEBIT_ALIGN;
        for (int i = 0; i < NUMBER_OF_RM; i++) {
            malt_fill_write(fe_func_arg, fe_no*NUMBER_OF_RM + i);
            printf("\n  write-unblock with value %04X to addr=%d\n", fe_no*NUMBER_OF_RM + i, fe_no);
            malt_sleep_us(400);
            test = print_events(fe_no*NUMBER_OF_RM + i);
            if (test < 0 || test >> 8 > 1 || (test & 0xff) > reads)
                ERROR(5);
            if (test >> 8 == 0 && rm_prev == 0)
                ERROR(6);
            rm_prev = test >> 8;
            reads -= test & 0xff;
        }
    }
    if (reads > 0) {
        printf("There are %d unblocked reads remaining\n", reads);
        ERROR(8);
    }
    else if (reads < 0) {
        printf("An anamalous value of %d unblocked reads\n", reads);
        ERROR(9);
    }
    return 0;
}

