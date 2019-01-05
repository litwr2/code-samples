//#define ENABLE_TRACE

#include <stdio.h>
#include <libmalt.h>
#include <assert.h>

void f8(u32_t x, u32_t y, u32_t z, u32_t w, u32_t v, u32_t u, u32_t x1, u32_t x2) {
    malt_send_ic_data(0, 777, x + y + z + w + v + u + x1 + x2);
}

void f7(u32_t x, u32_t y, u32_t z, u32_t w, u32_t v, u32_t u, u32_t x1) {
    malt_send_ic_data(0, 777, x + y + z + w + v + u + x1);
}

void f6(u32_t x, u32_t y, u32_t z, u32_t w, u32_t v, u32_t u) {
    malt_send_ic_data(0, 777, x + y + z + w + v + u);
}

void f5(u32_t x, u32_t y, u32_t z, u32_t w, u32_t v) {
    malt_send_ic_data(0, 777, x + y + z + w + v);
}

void f4(u32_t x, u32_t y, u32_t z, u32_t w) {
    malt_send_ic_data(0, 777, x + y + z + w);
}

void f3(u32_t x, u32_t y, u32_t z) {
    malt_send_ic_data(0, 777, x + y + z);
}

void f2(u32_t x, u32_t y) {
    malt_send_ic_data(0, 777, x + y);
}

void f1(u32_t x) {
    malt_send_ic_data(0, 777, x);
}

void f0(void) {
    malt_send_ic_data(0, 777, 8);
}

int main() {
    spbus_packet data;

    malt_start_thr(&f8, 3, 1, 4, -1, 6, 3, 1, -1);
    while (malt_rec_ic_packet(&data) < 0);
    assert(data.data0 == 777 && data.data1 == 16);

    malt_start_thr(&f7, 3, 2, 4, -1, 5, 1, 1);
    while (malt_rec_ic_packet(&data) < 0);
    assert(data.data0 == 776 && data.data1 == 15);

    malt_start_thr(&f6, 3, 2, 4, 1, 3, 1);
    while (malt_rec_ic_packet(&data) < 0);
    assert(data.data0 == 777 && data.data1 == 14);

    malt_start_thr(&f5, 3, 2, 4, 1, 3);
    while (malt_rec_ic_packet(&data) < 0);
    assert(data.data0 == 777 && data.data1 == 13);

    malt_start_thr(&f4, 5, 4, 1, 2);
    while (malt_rec_ic_packet(&data) < 0);
    assert(data.data0 == 777 && data.data1 == 12);

    malt_start_thr(&f3, 5, 4, 2);
    while (malt_rec_ic_packet(&data) < 0);
    assert(data.data0 == 777 && data.data1 == 11);

    malt_start_thr(&f2, -1, 11);
    while (malt_rec_ic_packet(&data) < 0);
    assert(data.data0 == 777 && data.data1 == 10);

    malt_start_thr(&f1, 9);
    while (malt_rec_ic_packet(&data) < 0);
    assert(data.data0 == 777 && data.data1 == 9);

    malt_start_thr(&f0);
    while (malt_rec_ic_packet(&data) < 0);
    assert(data.data0 == 777 && data.data1 == 8);

    puts("test has passed successfully");
    return 0;
}
