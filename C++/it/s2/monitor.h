#ifndef CPPSCAN_MONITOR_H
#define CPPSCAN_MONITOR_H

#include "MonitorInstance.h"
#include "MySQLConfig.h"
#include "MonitorExchangeConfig.h"
#include "base/MonitorCache.h"
#include "base/RedisConfig.h"
#include "markets/markets.h"
#include "base/server/Server.h"
#include "base/server/ServerWorker.h"
#include "base/server/ConnectionInfo.h"
#include "ccxt/bot.h"
#include <list>
#include <map>
#include <vector>
#include <rapidjson/document.h>
#include <string>
#include <my_global.h>
#include <mysql.h>

namespace base {

rapidjson::Document parseConfig(const char *str);

#define MERGE_TYPE_DECREASE_OR_NEW_PRICE "decrease_or_new_price"
#define MERGE_TYPE_DEFAULT "default"

class Monitor {
    bool hasConfig;
    std::map<std::string, std::vector<base::MonitorExchangeConfig>> config;
    MySQLConfig mysqlConf;
    MySQLConfig mysqlConfProxy;
    base::RedisConfig redisConf;
    std::vector<std::map<std::string, double> > lastStat;
    std::vector<std::map<std::string, double> > lastStatPeriod;
    std::map<std::string, bool> lastStatCond = {};
    std::map<std::string, std::map<double, bool>> lastStatCondHist = {};
    std::mutex lastStatLock;
    std::mutex lastStatCondLock;

  public:
    std::mutex updatesMtx;
    // for proxy
    MYSQL *con;
    // for server/etc
    MYSQL *conLocal;
    // TODO source, ts?
    // TODO: store for each conn?
    std::map<int, std::map<std::string, ccxt::Depth>> updatesToSend;

    std::map<std::string, base::MonitorCache*> caches;
    std::map<std::string, std::vector<MonitorInstance*>> instances;
    std::map<std::string, MarketLib::Markets *> libMarkets;
    base::MonitorExchangeConfig& getExConfig(std::string exchange);
    base::MonitorExchangeConfig parseSingleConfig(rapidjson::Value &itr);
    Monitor(std::string cfgStr);
    void initMonitors();
    ~Monitor() noexcept(false);
    void saveLastStat(std::string exchange, int instanceId, std::map<std::string, double> lastStatCur);
    std::vector<std::map<std::string, double> > getLastStat();
    std::vector<std::map<std::string, double> > getLastStatPeriod();
    std::map<std::string, bool> getLastStatCond();
    ServerWorkerCfg serverWorkerCfg;
};
} // namespace base

#endif // CPPSCAN_MONITOR_H
