//#define ENABLE_TRACE
#include <libmalt.h>
#include <stdio.h>
#include <string.h>

#define USE_ALL_SLAVES 1
#define NUMBER_OF_WRITES 800
#define DELAY 70

void prints_spbus(const char *s) {
    for (int i = 0; i < strlen(s); i++)
        malt_spbus_uart_putchar(s[i]);
}

volatile int cnt;

void printing_thread() {
    int lcnt = 0;
    for (;;)
        if (lcnt != cnt) {
            prints_spbus("()");
            lcnt = cnt;
        }
}

int main(void) {
    puts("Hello SPBUS UART!");
    malt_spbus_uart_setbitrate(115200); //FIXME!!! move it to malt_ic_init() [file malt_ic_common.c]
    prints_spbus("Hello Super Master UART!\nPress Control-E to exit\n");
#if USE_ALL_SLAVES
    malt_start_all_slaves(printing_thread);
#else
    malt_start_thr(printing_thread);
#endif
    for(int i = 0; i < NUMBER_OF_WRITES; ++i) {
        cnt++;
        prints_spbus("[]");
        malt_sleep_us(DELAY);
    }
    puts("");
    return 0;
}

