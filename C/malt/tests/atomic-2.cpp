//#define ENABLE_TRACE
#include <assert.h>
#include <iostream>       // std::cout
#include <atomic>         // std::atomic, std::memory_order_relaxed
#include <thread>         // std::thread
#include <chrono>

struct AtomicCounter {
    std::atomic<int> value;

    void increment(){
        ++value;
    }

    void decrement(){
        --value;
    }

    int get(){
        return value.load();
    }
} a;

volatile int q;

void thread_func() {
    for (int i = 0; i < 10000; i++)
        ++q, a.increment();
    for (int i = 0; i < 10000; i++)
        --q, a.decrement();
}

int main () {
    std::thread first (thread_func);
    std::thread second (thread_func);
    std::thread third (thread_func);
    thread_func();
    first.join();
    second.join();
    third.join();
    std::cout << a.get() << ' ' << q << '\n';
    assert(a.value == 0);
    return 0;
}
