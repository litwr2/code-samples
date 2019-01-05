#include "WsMonitorInstance.h"
#include "WsMonitorInstanceThread.h"
#include "base/Log.h"
#include "ws-ccxt-cpp/it-websocket.h"
#include <algorithm>

namespace base {

class Monitor;

WsMonitorInstance::WsMonitorInstance(Monitor &monitor, std::string exchange, const base::MonitorExchangeConfig &cfg,
                                 MySQLConfig &mysqlConf, std::map<std::string, ccxt::Market> markets, MarketLib::Markets &libMarkets)
    : MonitorInstance(monitor, exchange, cfg, mysqlConf, markets, libMarkets) { }

void WsMonitorInstance::run() {
    if (isSubcheck) {
        return MonitorInstance::run();
    }
    int numberOfMarkets = std::min(config.wsMarketsLimit, (unsigned int) markets.size());
    base::Log::log(LOG_LEVEL_INFO, "ws: using " + std::to_string(numberOfMarkets) + " of them | " + exchange);
    if (!numberOfMarkets) {
        return;
    }

    std::vector<std::string> marketsIds;
    int counter = 0;
    for (auto it = markets.begin(); it != markets.end(); ++it) {
        marketsIds.push_back(it->first);
        if (++counter >= numberOfMarkets) break;
    }
    auto chunks = base::array_chunk(marketsIds, config.wsMarketsPerThread);

    auto chunkIt = chunks.begin();
    for (int i = 0; i < this->config.wsThreads; i++) {
        WsMonitorInstanceThread *thread = new WsMonitorInstanceThread(this->monitor, *this, this->exchange,
                                                                      this->config, *chunkIt);
        this->threads.push_back(thread);
        std::thread *t = new std::thread(&WsMonitorInstanceThread::run, (WsMonitorInstanceThread*) thread);
        t->detach();

        chunkIt++;
        if (chunkIt == chunks.end()) chunkIt = chunks.begin();
    }

    this->printStatLoop();
}

}
