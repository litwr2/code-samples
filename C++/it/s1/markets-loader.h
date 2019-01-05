#ifndef MARKET_LOADER_H
#define MARKET_LOADER_H

#include <list>
#include <chrono>
#include <mutex>

#define MAX_NUMBER_OF_MARKETS 1888U
#define MAX_CONNECTIONS 1U

struct Market_record {
//    std::string id;
    std::string baseCurrency;
    std::string quoteCurrency;
    double quantityIncrement;
    double tickSize;
    double takeLiquidityRate;
    double provideLiquidityRate;
    std::string feeCurrency;
    unsigned updates[MAX_CONNECTIONS];
    std::list<std::pair<std::chrono::time_point<std::chrono::steady_clock>, unsigned>> time[MAX_CONNECTIONS];
    std::mutex mx;
};

extern std::string markets[];
extern Market_record markets_data[];
extern unsigned number_of_markets;

unsigned get_markets();

#endif
