#include <stdio.h>
#include <assert.h>
#include <libmalt.h>

#define MAX_SLAVES 1001
volatile unsigned long long fib_results[MAX_SLAVES];

void fib_gv(unsigned n, unsigned p) { //argument p holds a position for a result in array fib_results
    unsigned long long a = 0, b = 1;
    while (--n > 0) {
        b = a + b;
        a = b - a;
    }
    fib_results[p] = b;
}

long long ack(unsigned n, unsigned x, unsigned y) {
   if (n == 0) return y + 1;
   if (y == 0)
      switch (n) {
         case 1: return x;
         case 2: return 0;
         default: return 1;
      }
   return ack(n - 1, x, ack(n, x, y - 1));
}

volatile unsigned long long ack_result;

void ack_thread(unsigned n, unsigned x, unsigned y) {
    ack_result = ack(n, x, y);
}

void sum8(int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8) {
    malt_send_ic_data(0, a1 + a2 + a3 + a4 + a5 + a6 + a7 + a8, 0); //result is sent to core 0
}

void fib_ic(unsigned n) {
    unsigned long long a = 0, b = 1;
    while (--n > 0) {
        b = a + b;
        a = b - a;
    }
    malt_send_ic_data(0, b >> 32, b & 0xffffffff); //result is sent to core 0
}

int get_int(int task) {
    spbus_packet data;
    while (malt_rec_ic_packet_from(&data, task) < 0);
    return data.data0;
}

unsigned long long get_ull(int task) {
    spbus_packet data;
    while (malt_rec_ic_packet_from(&data, task) < 0);
    return ((unsigned long long)data.data0 << 32) + data.data1;
}

int main() {
    int task3, task4, task5;
    if (malt_get_total_slaves() < 3) {
        printf("At least 3 slaves are required for this test!\n");
        return 1;
    }
    puts("Calculating Fibonacci numbers and Ackermann's function simultaneously using global variables to return results");
    for (int i = 1; i < malt_get_total_slaves(); i++)
        malt_start_thr(fib_gv, i%93 + 1, i - 1);
    malt_start_thr(ack_thread, 3, 2, 6);
    malt_wait_all_lines_free();
    for (int i = 1; i < malt_get_total_slaves(); i++)
        printf("Fibonacci number %u = %llu\n", i%93 + 1, fib_results[i - 1]);
    printf("Ackermann's function at (3, 2, 6) = 2^6 = %llu\n", ack_result);

    puts("\nCalculating sums of 8 integer numbers and a large Fibonacci number using interconnect to return results");
    task3 = malt_start_thr(sum8, 1, 2, 3, 4, 5, 6, 7, 8);
    task4 = malt_start_thr(sum8, 9, 10, 11, 12, 13, 14, 15, 16);
    task5 = malt_start_thr(fib_ic, 93);
    printf("1 + 2 + 3 + 4 + 5 + 6 + 7 + 8 = (1 + 8)*4 = %d\n", get_int(task3));
    printf("9 + 10 + 11 + 12 + 13 + 14 + 15 + 16 = (9 + 16)*4 = %d\n", get_int(task4));
    printf("Fibonacci number 93 = %llu\n", get_ull(task5));
    return 0;
}

