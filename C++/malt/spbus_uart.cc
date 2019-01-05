/*
 * 
 * MALT SPbus UART
 *
 */

#include "maltemu.h"
 
#if ENABLE_SPBUART

#include "maltspbus.hh"
#include "maltsoc.hh"
#include "spbus_uart.hh"

#define XTERM 0
#define SIMPLE_OUTPUT 1
#define PTYERRNO 11
#if XTERM && SIMPLE_OUTPUT
#error XTERM & SIMPLE_OUTPUT are mutually exclusive!
#endif

MALT_SPbus_UART::MALT_SPbus_UART(MALT_SoC &malt_soc) : malt(malt_soc), status_reg(RXB_EMPTY), speed_reg(1), rx_fifo_rp(0), rx_fifo_wp(0) {
#if !SIMPLE_OUTPUT
    char term_name[16];

    if ((fd_master = posix_openpt(O_RDWR)) < 0) {
        report("Error %d on posix_openpt()\n", errno);
        throw exit_exception(PTYERRNO);
    }
    if (grantpt(fd_master) != 0) {
       report("Error %d on grantpt()\n", errno);
       throw exit_exception(PTYERRNO);
    }
    if (unlockpt(fd_master) != 0) {
       report("Error %d on unlockpt()\n", errno);
       throw exit_exception(PTYERRNO);
    }
    report("Slave side of the pseudoterminal is available at %s", strcpy(term_name, ptsname(fd_master)));
    fd_slave = open(term_name, O_RDWR); //Open the slave side ot the PTY
#if XTERM
    char xterm_option[16];
    pid_t pid;
    struct termios mst_orig_term_settings; // Saved terminal settings
    struct termios new_term_settings; // Current terminal settings
    tcgetattr(fd_master, &mst_orig_term_settings);
    new_term_settings = mst_orig_term_settings;
    new_term_settings.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(fd_master, TCSANOW, &new_term_settings);

    /* Fire up an xterm on the master side of the pseudo tty */
    fflush(stderr);
    fflush(stdout);
    pid = fork();
    sprintf(xterm_option, "-Sxx%d", fd_master);
    switch(pid) {
    case -1:
        report("pseudoterminal fork failure");
        close(fd_slave);
        fd_slave = -1;
        close(fd_master);
        throw exit_exception(PTYERRNO);
    case 0:
        for (int i = 0; i < 1000; i++)
            if (i != STDERR_FILENO && i != fd_master) close(i);
        execlp("xterm", "xterm", xterm_option, nullptr);
        report("can't start xterm for pseudoterminal");
        throw exit_exception(PTYERRNO);
    default:
        char input[128];
        close(fd_master);
        report("Starting xterm (pid %d) on %s\n", (int)pid, term_name);

          /* Discard windowid, which comes back as first line from xterm */
          /* (this over-simple bit of code isn't really good enough...)  */

        FD_ZERO(&fd_in);
        FD_SET(fd_slave, &fd_in);
        if (select(fd_slave + 1, &fd_in, 0, 0, 0) < 0 || read(fd_slave, input, sizeof(input)) < 0) {
             report("initial read on fd_slave failed");
             throw exit_exception(PTYERRNO);
        }
        new_term_settings.c_lflag |= ECHO;
        tcsetattr(fd_slave, TCSANOW, &new_term_settings);
    }
#else
    struct termios mst_orig_term_settings; // Saved terminal settings
    struct termios new_mst_term_settings; // Current terminal settings
    tcgetattr(fd_master, &mst_orig_term_settings);
    new_mst_term_settings = mst_orig_term_settings;
    //new_mst_term_settings.c_lflag &= ~(ICANON | ECHO);
    new_mst_term_settings.c_lflag &= ~ECHO;
    tcsetattr(fd_master, TCSANOW, &new_mst_term_settings);

//    struct termios slv_orig_term_settings; // Saved terminal settings
//    struct termios new_slv_term_settings; // Current terminal settings
//    tcgetattr(fd_slave, &slv_orig_term_settings);
//    new_slv_term_settings = slv_orig_term_settings;
//    new_slv_term_settings.c_lflag &= ~(ICANON | ECHO);
//    tcsetattr(fd_slave, TCSANOW, &new_mst_term_settings);
//    new_term_settings.c_lflag |= ECHO;
//    tcsetattr(fd_master, TCSANOW, &new_term_settings);
#endif
#endif
}

#if XTERM
#define fd_read fd_slave
#define fd_write fd_slave
#elif SIMPLE_OUTPUT
#define fd_read 0
#define fd_write 1
#else
#define fd_read fd_master
#define fd_write fd_master
#endif

void MALT_SPbus_UART::Tick() {
#if ENABLE_SPB_UART_INP
    char input[128];
    static struct timeval tm;
    static int skip_counter;
    if ((++skip_counter&0xff) == 0) {
        // Wait for data from standard input and master side of PTY
        FD_ZERO(&fd_in);
        FD_SET(fd_read, &fd_in);
        if (select(fd_read + 1, &fd_in, 0, 0, &tm) == -1) {
            report("Error %d on select()", errno);
            throw exit_exception(PTYERRNO);
        }
        else {
            // If data on master side of PTY
            if (FD_ISSET(fd_read, &fd_in)) {
                int rc = read(fd_read, input, sizeof(input));
                if (rc > 0) {
                    if (input[0] == 'E' - 'A' + 1) //Control-E - exit from emulator
                        throw exit_exception(0);
                    std::lock_guard<std::mutex> lk(mx_fifo);
                    for (int i = 0; i < rc; i++) {
                        if (rx_fifo_wp == rx_fifo_rp && (status_reg & RXB_EMPTY) == 0) {
                            status_reg |= RXB_OVERFLOW_ERR;
                            break;
                        }
                        rx_fifo[rx_fifo_wp++ % RX_FIFO_SIZE] = input[i];
                        if (rx_fifo_wp == rx_fifo_rp)
                            status_reg |= RXB_FULL_ext;
                    }
                    status_reg &= ~RXB_EMPTY;
                }
                else if (rc < 0) {
                    report("Error %d on read master PTY\n", errno);
                    throw exit_exception(PTYERRNO);
                }
            }
        }
    }
#endif
}

void MALT_SPbus_UART::SPbusRecv(const SpbPacket& packet) {
    if (packet.data0 & 0x80) { //write
        if ((packet.data0 & 0x7f) == 3) { //write data
            char b = packet.data1 & 0xff;
            write(fd_write, &b, 1);
        }
        else if ((packet.data0 & 0x7f) == 1) { //write speed
            speed_reg = packet.data1 & UART_SPEED_MASK;
        }
    }
    else { //read
        SpbPacket data;
        if ((packet.data0 & 0x7f) == 0) { //read status
            data.type = IC_SPB_STYPE_DATA;
            data.len = 2;
            data.target = packet.source;
            data.add_data = 0;
            data.source = SPB_ADDR_UART;
            data.data0 = status_reg & (READ_EMPTY_ERR | RXB_OVERFLOW_ERR | RXB_EMPTY);
            data.data1 = 0;
            malt.GetSPbusByNum(packet.source).SPbusRecv(data);
            status_reg &= ~(READ_EMPTY_ERR | RXB_OVERFLOW_ERR);
        }
        else if ((packet.data0 & 0x7f) == 2) { //read data
            data.type = IC_SPB_STYPE_DATA;
            data.len = 2;
            data.target = packet.source;
            data.add_data = 0;
            data.source = SPB_ADDR_UART;
            if (rx_fifo_rp != rx_fifo_wp || status_reg & RXB_FULL_ext) {
                std::lock_guard<std::mutex> lk(mx_fifo);
                data.data0 = rx_fifo[rx_fifo_rp++ % RX_FIFO_SIZE];
                status_reg &= ~RXB_FULL_ext;
                if (rx_fifo_rp == rx_fifo_wp)
                    status_reg |= RXB_EMPTY;
            }
            else
                data.data0 = READ_EMPTY_ERR << 24;
            data.data1 = 0;
            malt.GetSPbusByNum(packet.source).SPbusRecv(data);
        }
    }
}

#endif
