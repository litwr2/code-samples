/* *********************************************************************
 * MALTemu
 *
 * MALT SoC with MicroBlaze CPU emulator 
 *
 * Created by Egor Lukyanchenko
 * Based on MBemu by Joel Yliluoma aka Bisqwit
 * *********************************************************************
 * 
 * Main source
 *
 */

#include <unistd.h>
#include <sys/time.h>
#include <chrono>

#include "maltemu.h"

#include "emuconsole.hh"
#include "thread_scheduler.hh"
#include "maltsoc.hh"
#include "limits.h"

#ifdef  LIFT_RC
int user_rc = 1;
#define RETURN(x) return getenv("MALT_LIFT_RC")!=NULL ? user_rc : x;
#else//!LIFT_RC
#define RETURN(x) return x
#endif//LIFT_RC

bool simd_trace_load;
        
MALTemuConfig  config;
static MALT_SoC *malt;

#ifdef WITH_DLLS
MALT_SoC& GetMALT() { return *malt; } 
#endif

void usage(char name[])
{
    printf("Usage: %s [-p] -i memory_image -m smaster_rom -s slave_rom [-w watch_addr]\n", name);
    exit(-1);
}

int main(int argc, char **argv)
{
    u32 watch_addr = 0;
    int c;
    
    // Parse cmdline parameters
    while ((c = getopt(argc, argv, "pm:s:i:w:")) != -1)
        switch (c)
        {
            case 'm':
                config.smst_iromfile = optarg;
                break;
            case 's':
                config.slv_iromfile = optarg;
                break;
            case 'i':
                config.mem_imagefile = optarg;
                break;
            case 'w':
                watch_addr = std::strtol(optarg, 0, 16);  // hex
                break;
        case 'p':
        config.do_profile = true;
        break;
            default:
                usage(argv[0]);
        }
    
    if (config.smst_iromfile.empty() || config.slv_iromfile.empty() || config.mem_imagefile.empty())
        usage(argv[0]);
    
    extern const char *maltemu_path;
    maltemu_path = strdup(argv[0]);
    
    config.do_profile |= getenv("MALT_PROFILER")!=NULL || getenv("MALT_MONITOR")!=NULL;
    if (config.do_profile) {
        extern const char *profProg;
        profProg    = strdup(config.mem_imagefile.c_str());
    }

    EmuConsole console;
    #if SET_SIGNAL_HANDLER
    void set_signal_handler(EmuConsole&);
    set_signal_handler(console);
    #endif
    try // To reset tty attr in _any case_ of fail
    {
        malt = new MALT_SoC(console, config);

        const char *trace_cpu = getenv("MALT_TRACE_CPU");
        if (trace_cpu) {
          if (!strncmp("0x",trace_cpu,2)) {
            int num = strtoull(trace_cpu+2,NULL,16);
            malt->GetCoreByNum(num).ToggleOpcodeDebug(true);
          } else { 
            report("Invalid MALT_TRACE_CPU format: 0x<hex-num> expected");
          }
        }
        
        simd_trace_load = maltcfg_long("MALT_GEPARD_TRACE_LOAD",0,1,0);

        #if MALTEMU_GDB
        const char* bp0 = getenv("MALT_INIT_BREAK");
        if (bp0) {
            extern void init_gdb(MALT_SoC*,const char *);
            init_gdb(malt,bp0);
        }
        #endif
    }
    catch(exit_exception& e)
    {
        report("MALTemu init failed, exiting from emulator");
        RETURN(e.c);
    }

    if (watch_addr != 0)
        malt->GetSharedMemory().WatchAddress(watch_addr);

    char theConf[0x100];
    #define setConf(fmt...) snprintf(theConf,sizeof(theConf),fmt)
    switch (MALT_CHAINS*MALT_SLVONCHAIN) {
        case 0:
            setConf("supermaster only"); 
            break;
        case 1:
            setConf("supermaster + 1 slave");
            break;
        default:
            setConf("supermaster + %d x %d slaves", MALT_CHAINS, MALT_SLVONCHAIN);    
    }

    #if WITH_GDBSTUB
    void create_gdbstub();
    create_gdbstub();
    #endif

    #if MALTEMU_CDA
    void cda_init(const char *progFile);
    cda_init(config.mem_imagefile.c_str());
    #endif

    report("Starting emulation (%s), press Ctrl+E to exit", theConf);

    srandom(time(0));

    auto start_time = std::chrono::high_resolution_clock::now();

    // Run the machine
    unsigned int n = 0;
    char *emuthreads;
    if ((emuthreads = getenv("MALT_EMUTHREADS")) != 0) {
        if (strcmp(emuthreads, "0") == 0)
            n = std::thread::hardware_concurrency();
        else
            n = atoi(emuthreads);
    }
    if (n == 0) n = 1;
    Scheduler scheduler(malt, n); //it finally sets up n
    report("An emulator run using %u host thread(s)", n);
    try
    {
         scheduler.Run();
    }
    catch(exit_exception& e)
    {
        printf("\x1b[0m");
        report("Exiting from emulator (%lld cycles, %lld cycles/sec)", 
            total_cycles, (total_cycles*1000000)/(uint64_t)(std::chrono::duration<double,
                    std::micro>(std::chrono::high_resolution_clock::now() - start_time)).count() + 1);
        #if MALTEMU_CDA
        void cda_invoke();
        extern int cda_no_output;
        if (!cda_no_output) cda_invoke();
        #endif
        RETURN(e.c);
    }
    RETURN(0);
}

