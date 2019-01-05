//
// Created by Андрей Викулов on 19.06.18.
//

#include "Log.h"
#include <iostream>
#include <string>

#define LOGFILENAME "system.log"

namespace base {

int Log::logLevel = LOG_LEVEL_WARNING;
int Log::showStat = 1;  // 0 - don't show, 1 - show at a console only, 2 - show everywhere
int Log::logOutput = Log::Console;
std::mutex Log::logMutex;
std::ofstream Log::fo;

void Log::log(int logLevel, std::string str) {
    if (logLevel <= Log::logLevel || (showStat && logLevel == LOG_LEVEL_STAT)) {
        logMutex.lock();
        if (logOutput == Console || logOutput == Both || (showStat && logLevel == LOG_LEVEL_STAT))
            std::cout << str << std::endl;
        if ((logOutput == File || logOutput == Both) && (logLevel != LOG_LEVEL_STAT || showStat == 2)) {
            if (!fo.is_open()) {
                double microtime();
                fo.open(LOGFILENAME, std::ios::app);
                fo << "\n### System starts at " << std::fixed << microtime() << std::endl;
            }
            fo << str << std::endl;
            fo.flush();
        }
        Log::logMutex.unlock();
    }
}
} // namespace base
