#ifndef PROJECT_LOG_H
#define PROJECT_LOG_H

#define LOG_LEVEL_STAT 6
#define LOG_LEVEL_TRACE 5
#define LOG_LEVEL_DEBUG 4
#define LOG_LEVEL_INFO 3
#define LOG_LEVEL_WARNING 2

#include <mutex>
#include <string>
#include <fstream>

namespace base {

class Log {
    static std::mutex logMutex;

  public:
    enum {Console, File, Both};
    // void Log(int logLevel = LOG_LEVEL_WARNING, bool showStat = 1);
    static int logLevel;
    static int showStat;
    static int logOutput;
    static std::ofstream fo;

    static void log(int logLevel, std::string str);
};

} // namespace base

#endif // PROJECT_LOG_H
