#include <iostream>
#include <future>
#include <thread>
using namespace std::chrono_literals;

int res;

int thread1() {
    std::promise<int> p;
    std::future<int> f = p.get_future();
    std::thread([&p] {
          std::this_thread::sleep_for(1ms);
          p.set_value_at_thread_exit(9);
    }).detach();
    res = f.get();
}

int main()
{
    std::thread thr1(thread1);
    std::cout << "Waiting..." << std::flush;
//    f.wait();
    thr1.join();
    std::cout << "Done!\nResult is: " << res << '\n';
}
