#include "maltsoc.hh"
#include "thread_scheduler.hh"

//synchronization makes the execution a bit slower
#define SYNCHRO_THREADS 0

#if SYNCHRO_THREADS
void Scheduler::sync() {
    static thread_local unsigned sync_counter;
    static unsigned ready_cnt;
    static std::mutex mx;
    if (++sync_counter == max_overtakes) {
        sync_counter = 0;
        std::unique_lock<std::mutex> lk(mx);
        if (++ready_cnt == threads) {
            ready_cnt = 0;
            sync_counter += max_overtakes/128 + 1;  //is it helping?
            cv.notify_all();
        }
        else
            cv.wait(lk);
    }
}
#endif

Scheduler::Scheduler(MALT_SoC *malt_soc, unsigned &threads0): threads(threads0), malt(malt_soc) {
    if (threads > MALT_CORES + 1) threads0 = threads = MALT_CORES + 1;
    jiffy = new unsigned long [threads];
    thread_unit = new std::thread[threads - 1];
    call_list.push_back({std::bind(&MALT_SMChain::Tick, malt->sm_chain), CallUnit::SM});
    //call_list.push_back({std::bind(&MALT_SharedMemory::Tick, &malt->memory_controller), CallUnit::MC});
    //call_list.push_back({std::bind(&MALT_SPbus_UART::Tick, &malt->uart_unit), CallUnit::UART});
    call_list.push_back({[this]{malt->memory_controller.Tick(); 
        #if ENABLE_SPBUART
        malt->uart_unit.Tick();
        #endif
        }, CallUnit::MC});
    for (unsigned i = 0; i < MALT_CHAINS; ++i) {
        call_list.push_back({std::bind(&MALT_Core::CoreTick, malt->chains[i]->master), CallUnit::M});
        for (unsigned j = 0; j < MALT_SLVONCHAIN; ++j)
            call_list.push_back({std::bind(&MALT_Core::CoreTick, malt->chains[i]->slaves[j]), CallUnit::SL});//SlaveTick?
    }
    tasks_per_thread = call_list.size()/threads;
}


Scheduler::~Scheduler() {
    delete [] thread_unit;
    delete [] jiffy;
}

void Scheduler::Launch(unsigned tasks_per_thread, unsigned threadtask_base, unsigned thread_count) {
    static thread_local unsigned long thread_jiffy;
    try {
        for (;;) {
            for (unsigned i = 0; i < tasks_per_thread; ++i)
                call_list[threadtask_base + i].call();
            ++thread_jiffy;
#if SYNCHRO_THREADS
            if (threads > 1) sync();
#endif
            if (pe) break;
        }
    } catch (...) {
        pe = std::current_exception();
    }
    cv.notify_all();
    jiffy[thread_count] = thread_jiffy;
#if MALTEMU_CDA // Crash Dump Analyzer
    void cda_event_save();
    cda_event_save();
#endif
}

void Scheduler::Run() {
    unsigned free_tasks = call_list.size(), free_threads = threads;
    for (unsigned i = 0; i < threads; ++i) jiffy[i] = 0;
    while (free_threads > 1) {
        thread_unit[thread_count] = std::thread(&Scheduler::Launch, this, tasks_per_thread, threadtask_base, thread_count);
        thread_count++;
        threadtask_base += tasks_per_thread;
        free_tasks -= tasks_per_thread;
        tasks_per_thread = free_tasks/--free_threads;
    }
    Launch(free_tasks, threadtask_base, thread_count);
    for (unsigned i = 0; i < threads - 1; ++i)
        thread_unit[i].join();
#if 0
    char s[40 + threads*22];
    sprintf(s, "Sync data for %u threads:", threads);
    for (unsigned i = 0; i < threads; ++i)
        sprintf(s + strlen(s), "  %lu", jiffy[i]);
    report("%s", s);
#endif
    std::rethrow_exception(pe);
}
