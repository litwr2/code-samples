#ifndef _SPBUS_UART_HH
#define _SPBUS_UART_HH

#define _XOPEN_SOURCE 700
//#define _DEFAULT_SOURCE
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <termios.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <string.h>
#include <mutex>
#include "maltemu.h"

class MALT_SoC;
class SpbPacket;

class MALT_SPbus_UART {
    MALT_SoC &malt;
    //static const int PTS_NAME_MAXLENGTH = 32;
    static const u32_t RX_FIFO_SIZE = 1023;
    static const u32_t UART_SPEED_MASK = 0xFFFFFF;
    //char pts_name[PTS_NAME_MAXLENGTH];
    int fd_master, fd_slave;
    fd_set fd_in;
    u32_t status_reg, speed_reg;
    char rx_fifo[RX_FIFO_SIZE];
    u32_t rx_fifo_rp, rx_fifo_wp;
    std::mutex mx_fifo;
public:
    enum ERRFLAGS {RXB_OVERFLOW_ERR = 1, RXB_EMPTY = 2, READ_EMPTY_ERR = 8, RXB_FULL_ext = 32};
    void Tick();
    void SPbusRecv(const SpbPacket& packet);
    MALT_SPbus_UART(MALT_SoC &malt_soc);
    ~MALT_SPbus_UART() {}
};

#endif

