//#define ENABLE_TRACE
#include <iostream>
#include <future>
#include <mutex>

#define NT 10

std::recursive_mutex mx;

int fib(int n) {
   std::lock_guard<std::recursive_mutex> l(mx);
   if (n < 2) return n;
   return fib(n - 1) + fib(n - 2);
}

int results[NT];

int main() {
    std::future<int> f[NT];
    std::cout << "starting threads\n";
    for (int i = 0; i < NT; i++)
        f[i] = std::async(std::launch::async, fib, 10);
    std::cout << "getting results\n";
    for (int i = 0; i < NT; i++)
        results[i] = f[i].get();
    std::cout << "printing\n";
    for (int i = 0; i < NT; i++)
        std::cout << i << ' ' << results[i] << '\n';
    return 0;
}
