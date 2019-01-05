//#define ENABLE_TRACE
#include <assert.h>
#include <iostream>
#include <future>
#include <thread>

int main() {
    // future from a packaged_task
    std::packaged_task<int()> task([]{ return 7; }); // wrap the function
    std::future<int> f1 = task.get_future();  // get a future
    std::thread t(std::move(task)); // launch on a thread
 
    // future from an async()
    std::shared_future<int> f2 = std::async(std::launch::async, []{ return 8; });
 
    // future from a promise
    std::promise<int> p;
    std::future<int> f3 = p.get_future();
    std::thread( [&p]{ p.set_value_at_thread_exit(9); }).detach();

    std::cout << "Waiting..." << std::flush;
//    f1.wait();
//    f2.wait();
//    f3.wait();
    int r1, r2, r3;
    std::cout << "Done!\nResults are: "
              << (r1 = f1.get()) << ' ' << (r2 = f2.get()) << ' ' << (r3 = f3.get()) << '\n';
    assert(r1 == 7 && r2 == 8 && r3 == 9);
    std::cout << "Done!\nResults are: "
              << (r2 = f2.get()) << '\n';
    assert(r2 == 8);
    t.join();
    return 0;
}
