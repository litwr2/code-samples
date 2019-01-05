#ifndef CPPSCAN_WSMONITORINSTANCE_H
#define CPPSCAN_WSMONITORINSTANCE_H

#include "MonitorInstance.h"
#include <string>
#include <unordered_map>
#include <vector>
#include <markets/markets.h>

namespace base {

class Monitor;

class WsMonitorInstance : public MonitorInstance {
  protected:

  public:
    WsMonitorInstance(Monitor &monitor, std::string exchange, const base::MonitorExchangeConfig &cfg,
                                 MySQLConfig &mysqlConf, std::map<std::string, ccxt::Market> markets, MarketLib::Markets &libMarkets);
    void run();
};

} // namespace base

#endif // CPPSCAN_WSMONITORINSTANCE_H
