//#define ENABLE_TRACE

#include <stdio.h>
#include <libmalt.h>
#include <assert.h>

#define N 16

volatile u32_t v[16] __attribute__ ((__aligned__ (16))), ie;

int main() {
    malt_disable_interrupts();
    malt_ext_memop_inc((void*)v, 7);
    malt_ext_memop_memset((void*)v, 5, 16);
    malt_ext_memop_inc((void*)v, 7);
    malt_ext_memop_inc((void*)v, 7);
    malt_ext_memop_inc((void*)v, 7);
    malt_read_modify((void*)&v[5]);
    malt_fill_write((void*)&v[5], 77);
    malt_ext_memop_inc((void*)&v[5], 7);
    for (int i = 0; i < N; ++i)
        malt_ext_memop_inc((void*)&v[1], 1);
    malt_enable_interrupts();
    malt_ext_memop_inc((void*)&v[1], 1);
    malt_sleep_us(500);
    malt_ext_memop_inc((void*)&v[1], 1);
    malt_disable_interrupts();
    for (int i = 0; i < N; ++i)
        malt_ext_memop_inc((void*)&v[1], 1);

    printf("ie=%d %d %d %d %d\n", malt_get_interrupts_state(), v[0], v[1], v[4], v[5]);
    assert(v[0] == 26 && v[1] == 7 + 2*N && v[4] == 0 && v[5] == 84 && ie == 0);
    malt_enable_interrupts();
    return 0;
}
