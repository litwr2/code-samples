#ifndef _THREADSCHEDULER_HH
#define _THREADSCHEDULER_HH

#include <thread>
#include <atomic>
#include <condition_variable>
#include <vector>
#include <functional>
#include "maltsoc.hh"

struct CallUnit {
    std::function<void(void)> call;
    enum CallType {SM, M, SL, MC, UART} type;
};

class Scheduler {
    unsigned threads;
    const unsigned max_overtakes = 256;//256;//128;//64; //ticks
    unsigned threadtask_base = 0, tasks_per_thread, thread_count = 0;
    MALT_SoC *malt;
    std::thread *thread_unit;
    unsigned long *jiffy;
    std::vector<CallUnit> call_list;
    std::exception_ptr pe = 0;
    void Launch(unsigned, unsigned, unsigned);
    void sync();
    std::condition_variable cv;
public:
    Scheduler(MALT_SoC *malt_soc, unsigned& threads);
    ~Scheduler();
    void Run();
};

#endif

