//it calculates the number pi with given precision using all slave cores
//#define ENABLE_TRACE
#include <stdio.h>
#include <assert.h>
#include <libmalt.h>

#define USE_SM 0

#define MAX_SLOTS 160   //IT IS GENERALLY LESS THAN MAX NUMBER OF SLAVES
#define DIGITS 100      //IT SHOULD BE A MULTIPLE OF 4
#define N DIGITS*7/2

volatile unsigned count;
volatile char result_pi[MAX_SLOTS][DIGITS + 1];
volatile unsigned result_core[MAX_SLOTS];

int pi() {
    unsigned short r[N + 1];
    int i, k, b, d, c = 0, n = malt_cur_core_num(), q = malt_seq_core_num(n);
    char *p = (char*)result_pi[q];

    for (i = 1; i <= N; i++)
        r[i] = 2000;
    for (k = N; k > 0; k -= 14) {
        d = 0;
        i = k;
        for (;;) {
            d += r[i]*10000;
            b = 2*i - 1;
            r[i] = d%b;
            d /= b;
            i--;
            if (i == 0) break;
            d *= i;
        }
        sprintf(p, "%.4d", c + d/10000);
        p += 4;
        c = d%10000;
    }
    *p = 0;
    result_core[q] = n + 1;
    return 0;
}

int main()
{
    u64_t start_time = malt_get_time_us_64(), calculation_time;
    int cores = malt_start_all_slaves(pi), cnt = 0;
    assert(cores = malt_get_total_slaves());
#if USE_SM
    printf("cores = %d + SM\n", cores);
    pi();
#else
    printf("cores = %d\n", cores);
#endif
    malt_wait_all_lines_free();
    calculation_time = malt_get_time_us_64() - start_time;
    printf("number of digits = %d\n", DIGITS);
    start_time = malt_get_time_us_64();
    for (int i = 0; i < MAX_SLOTS; i++)
        if (result_core[i] != 0)
            printf("result %02d [core 0x%03x] = %s\n", cnt++, result_core[i] - 1, result_pi[i]);
    printf("calculation time = %llu us\nprint time = %llu us\n", calculation_time, malt_get_time_us_64() - start_time);
    return 0;
}
