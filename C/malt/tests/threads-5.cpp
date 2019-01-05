//#define ENABLE_TRACE
#include <thread>
#include <iostream>
#include <string>
#include <sstream>
#include <chrono>
#include <cassert>

using std::chrono::milliseconds;

std::string rs;
std::ostringstream so;

void independentThread() {
    std::this_thread::sleep_for(milliseconds(2));
    so << "Starting concurrent thread.\n";
    rs += "1";
    std::this_thread::sleep_for(milliseconds(8));
    so << "Exiting concurrent thread.\n";
    rs += "2";
}
 
void threadCaller() {
    so << "Starting thread caller.\n";
    rs += "3";
    std::thread t(independentThread);
    t.detach();
    std::this_thread::sleep_for(milliseconds(4));
    so << "Exiting thread caller.\n";
    rs += "4";
}

int main() {
    threadCaller();
    std::this_thread::sleep_for(milliseconds(12));
    std::cout << so.str() << std::endl;
    std::cout << rs << std::endl;
    assert(rs == "3142");
    return 0;
}

