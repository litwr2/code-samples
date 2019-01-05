//#define ENABLE_TRACE
#include <assert.h>
#include <iostream>
#include <thread>
#include <future>
#include <chrono>
#include <functional>

using namespace std;

#define FIBN 25

template<unsigned N> struct Fib {
        enum { v = Fib<N - 1>::v + Fib<N - 2>::v };
};

template<> struct Fib<1> {
        enum { v = 1 };
};

template<> struct Fib<0> {
        enum { v = 0 };
};

int fib(int n) {
	return n < 2 ? n : fib(n - 1) + fib(n - 2);
}

int main() {
    int n = 0, r;
    packaged_task<int(int)> task(fib);
    future<int> f = task.get_future();
    thread t = thread(move(task), FIBN);
    cout << ++n << " fib(" << FIBN << ") = " << fib(FIBN) << endl;
    cout << ++n << " fib(" << FIBN << ") = " << (r = f.get()) << endl;
    t.join();
    assert(r == Fib<FIBN>::v);

    task = move(*new packaged_task<int(int)>(fib));
    f = task.get_future();
    task(FIBN);
    cout << ++n << " fib(" << FIBN << ") = " << (r = f.get()) << endl;
    assert(r == Fib<FIBN>::v);

    packaged_task<int()> task2(bind(fib, FIBN));
    f = task2.get_future();
    task2();
    cout << ++n << " fib(" << FIBN << ") = " << (r = f.get()) << endl;
    assert(r == Fib<FIBN>::v);

    task2 = move(*new packaged_task<int()>([]{ return fib(FIBN); }));
    f = task2.get_future();
    t = thread(move(task2));
    cout << ++n << " fib(" << FIBN << ") = " << fib(FIBN) << endl;
    cout << ++n << " fib(" << FIBN << ") = " << (r = f.get()) << endl;
    t.join();
    assert(r == Fib<FIBN>::v);
    return 0;
}

