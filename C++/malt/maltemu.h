/* *********************************************************************
 * MALTemu
 *
 * MALT SoC with MicroBlaze CPU emulator
 *
 * Created by Egor Lukyanchenko
 * Based on MBemu by Joel Yliluoma aka Bisqwit
 * *********************************************************************
 *
 * MALT emulator general header file
 *
 */

#ifndef _MALTEMU_H
#define _MALTEMU_H

#include <cstdio>
#include <libmalt.h>

#include "maltcfg.h"
#include "emuutils.hh"

const unsigned MALT_CORES       __attribute__((unused)) = 1 + (MALT_CHAINS * (MALT_SLVONCHAIN + 1));

#define ENABLE_OPCODE_DEBUG     1
#define ENABLE_FE               1
#define ENABLE_WATCH            0
#define ENABLE_MEMVALIDATION    0
#define ENABLE_PROFILER         1
#define ENABLE_GEPARD           1
#define ENABLE_FULLFS           1
#define ENABLE_SPBUART          1
#define VERIFY_RAM              0
#define VERIFY_UNINITED_READ    0
#define VERIFY_SMC_ALIGNMENT    1
#define ENABLE_SPB_UART_INP     0

/* Processor core config */
#define C_USE_REGFILE_SWAP      0
#define C_UNALIGNED_EXCEPTIONS  1
#define C_USE_HW_MUL            1
#define C_USE_BARREL            1
#define C_USE_MSR_INSTR         0

/* Debug config */
#define IO_DEBUG_LEVEL          1
#define IC_DEBUG_LEVEL          1
#define FE_DEBUG_LEVEL          1
#define GEPARD_DEBUG_LEVEL      3
#define TRACE_LEVEL             2
#define ERR_LEVEL               3
#define HALT_ON_ERRORS          3

/* Emulator print macros */
#define report(fmt, ...)        {fprintf(stderr, "### " fmt "\n", ##__VA_ARGS__); fflush(stderr);}
#define stub(msg)               report("Stub in " __FILE__ ": %s", msg)
#define dbg_ic_printf(l, ...)   if (IC_DEBUG_LEVEL >= (l)) report(__VA_ARGS__)
#define dbg_io_printf(l, ...)   if (IO_DEBUG_LEVEL >= (l)) report(__VA_ARGS__)
#define dbg_fe_printf(l, ...)   if (FE_DEBUG_LEVEL >= (l)) report(__VA_ARGS__)
#define dbg_gep_printf(l, ...)  if (GEPARD_DEBUG_LEVEL >= (l)) report(__VA_ARGS__)
#define trace_printf(l, ...)    if (TRACE_LEVEL >= (l)) report(__VA_ARGS__)
#define halt_on_error(l)        if (HALT_ON_ERRORS >= (l)) {/*report("Error occured - halting");*/ throw exit_exception(0);}
#define panic(...)              {report(__VA_ARGS__); halt_on_error(0);}

/* Magic numbers */
const u32 MALT_MAGIC_WORD       = 0x4D414C54;
const u32 MALT_REVISION_WORD    = 0x0109000A;
const u32 MALT_BOARD_WORD1      = 0x4D414C54;
const u32 MALT_BOARD_WORD2      = 0x656D7500;

extern unsigned long long total_cycles;
extern void (*mem_err_proc)(const char* reason, uint32_t index);
void system_ok(const char* cmd);

#endif
