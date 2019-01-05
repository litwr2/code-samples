//#define ENABLE_TRACE
#include <assert.h>
#include<thread>
#include<string>
#include<mutex>
#include<condition_variable>
#include<iostream>
#include<chrono>
using namespace std;

unsigned starter;
mutex mx;
volatile int r;
condition_variable cv;

void phase(unsigned cth) {
    //if (cth == 1) this_thread::sleep_for(chrono::microseconds(100));
    unique_lock<mutex> lk(mx);
    r = r*10 + cth;
    while (starter == 0) cv.wait(lk);
}

int main() {
    thread thread_ref[2];
    thread_ref[0] = thread(phase, 1);
    thread_ref[1] = thread(phase, 2);

    //this_thread::sleep_for(chrono::microseconds(50));
    mx.lock();
    r *= 10;
    starter = 1;
    mx.unlock();
    cv.notify_all();

    thread_ref[1].join();
    thread_ref[0].join();

    cout << r << endl;
    assert(r == 120 || r == 102 || r == 12 || r == 21 || r == 210 || r == 201);
    return 0;
}
