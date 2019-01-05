/* *********************************************************************
 * MALTemu
 *
 * MALT SoC with MicroBlaze CPU emulator 
 *
 * Created by Egor Lukyanchenko
 * Based on MBemu by Joel Yliluoma aka Bisqwit
 * *********************************************************************
 * 
 * Text console
 *
 */

#include <unistd.h>
#include <chrono>
#include <stdlib.h>
#include "maltemu.h"
#include "emuconsole.hh"


#if ASYNC_IO 
#include <sched.h>
#include <thread>

volatile long started,finished;//FIXME: use atomics

static void io_thread(int *p) {
    while (read(0,p,1)!=1) {
      //fprintf(stderr,"*"); 
      sched_yield();
    }

    //write(2,p,1);

    finished++;
}
static void async_io_notifier(volatile int *p) {
    if (started > finished) 
        return;
    std::thread th(io_thread, const_cast<int*>(p));
    th.detach();
    started++;
}
#endif//ASYNC_IO


EmuConsole::EmuConsole() : pending(-1)
{
    tcgetattr(0, &back);
    struct termios term = back;
    // Disable linebuffer, echoing and Ctrl+C
    term.c_lflag &= ~(ICANON | ECHO | ISIG);
#if ASYNC_IO
    term.c_cc[VMIN] = 1;
#else
    term.c_cc[VMIN] = 0; // 0=no block, 1=do block
#endif

    tcsetattr(0, TCSANOW, &term);
}


EmuConsole::~EmuConsole()
{
    back.c_lflag |= (ICANON | ECHO | ISIG);
    tcsetattr(0, TCSANOW, &back);
}


bool EmuConsole::Hit()
{
    if(pending >= 0) return true;

#if ASYNC_IO
    async_io_notifier(&pending);
    return false;
#else
    static volatile unsigned clock = 0;
    static auto start_time = std::chrono::steady_clock::now();
    if ((++clock & 63) > 0 || std::chrono::steady_clock::now() - start_time < std::chrono::milliseconds(30))
        return false;
    start_time = std::chrono::steady_clock::now();
#endif

    unsigned char c;
    int r = read(0, &c, 1);
    if(r > 0) 
    { 
        pending = c; 
        dbg_io_printf(10, "Got 0x%02X from console", c);
        return true; 
    }
    return false;
}


unsigned EmuConsole::Getc() 
{ 
    int r = pending;

    if(r=='')
    {
        throw exit_exception(0);
    }

    pending = -1; 
    return r; 
}

#ifdef MALTMON
bool inConsoleEscSeq = false;
#endif

void EmuConsole::Putc(unsigned c) 
{ 
  #ifdef MALTMON
  const long BIG_CNT = 10;
  unsigned static long esCnt = BIG_CNT;
  switch (c) {
    case '\033':
      esCnt = 0;
      break;
    case 'm':
      esCnt = BIG_CNT;
      break;
    default:
      esCnt++;  
  }
  inConsoleEscSeq = esCnt < BIG_CNT;
  #endif//MALTMON

  putchar(c); 
  fflush(stdout); 
}
