/* *********************************************************************
 * MALTemu
 *
 * MALT SoC with MicroBlaze CPU emulator 
 *
 * Created by Egor Lukyanchenko
 * Based on MBemu by Joel Yliluoma aka Bisqwit
 * *********************************************************************
 * 
 * Text console class header
 *
 */
 
#ifndef _EMUCONSOLE_HH
#define _EMUCONSOLE_HH
 
#include <termios.h>
#include <sys/ioctl.h>
#include <fcntl.h>

 
class EmuConsole
{
    struct termios back;
    volatile int pending;
public:

    EmuConsole();
    ~EmuConsole();
    bool Hit();
    unsigned Getc();
    void Putc(unsigned c);
    
};

#endif
