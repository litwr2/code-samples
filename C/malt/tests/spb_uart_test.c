//#define ENABLE_TRACE
#include <libmalt.h>
#include <stdio.h>
#include <string.h>

void prints_spbus(const char *s) {
    int len = strlen(s);
    for (int i = 0; i < len; i++)
        malt_send_ic_data(SPB_ADDR_UART, 0x83, s[i]);
}

volatile int cnt;

void printing_thread() {
    int lcnt = 0;
    prints_spbus("Hello Printing Thread!\n");
    for (;;)
    {
        if (lcnt != cnt && cnt%10 == 0) {
            prints_spbus("\nXXXXX-YYYYY-ZZZZZ\n"); //IT'S_A_PRINT_FROM_OTHER_CORE
            lcnt = (cnt/10)*10;
        }
    }
}

void listning_thread()
{
    spbus_packet data;
    prints_spbus("Hello Listning Thread!\nPress Control-E to exit\n");
    
    for(;;) {
        malt_send_ic_data(SPB_ADDR_UART, 0x2, 0);
        while(malt_rec_ic_packet(&data) < 0);

        if ((data.data0&0xff000000) == 0) {
            cnt++;
            malt_send_ic_data(SPB_ADDR_UART, 0x83, data.data0&0xff);
            if (cnt%10 == 0) {
                char s[64];
                sprintf(s, "\n%d chars are printed\n", cnt);
                prints_spbus(s);
            }
        }
    }
}

int main_local(void) {
    
    spbus_packet data;
    puts("Hello UART!");
    malt_spbus_uart_setbitrate(115200);
    malt_start_thr(printing_thread);
    malt_start_thr(listning_thread);
    for(;;) {
        malt_send_ic_data(SPB_ADDR_UART, 0x0, 0);
        while(malt_rec_ic_packet(&data) < 0);
        printf("%x %x\n",data.data0,data.data1);
        malt_sleep_us(3000000);
    }
    malt_sm_wait_all_lines_free();
    return 0;
}


int main(void)
{
    malt_local_stack_call(main_local);
    return 0;
}
