#ifndef __WSMONITORINSTANCETHREAD_H__
#define __WSMONITORINSTANCETHREAD_H__

#include "MonitorInstanceThread.h"
#include "ws-ccxt-cpp/it-websocket.h"
#include <vector>
#include <map>
#include <set>
#include <string>

namespace base {

class MonitorInstance;

class WsMonitorInstanceThread : public MonitorInstanceThread {
    std::vector<std::string> markets;

    // connection map
    int connectionCount = 0;
    double lastReconnect = 0;

    std::vector<std::vector<std::vector<std::string>>> chunks;
    std::vector<std::vector<bool>> chunksSubscribeFree;
    std::vector<std::vector<double>> chunksSubscribeLast;
    std::vector<int> chunkIndexes;

    std::map<int, wsccxt::WSsession*> connById;
    std::map<wsccxt::WSsession*, int> connIdBySession;
    std::set<int> connReadyById;
    // время когда соединение становится доступно для переподписки
    std::map<double, int> connReSubscribeFreeTsQueue;


    std::set<int> connToRecreate;
  public:
    WsMonitorInstanceThread(Monitor &monitor, MonitorInstance &inst, std::string exchange,
                          const base::MonitorExchangeConfig &cfg, std::vector<std::string> markets);
    void run();
    void initChunks(std::vector<std::string> markets, int connCount, int connMultipliers);
    std::vector<std::string> nextChunk(int connId);
    void onSockectError(wsccxt::WSsession *pws, MonitorStat &monitorStat);
};

}

#endif // __WSMONITORINSTANCETHREAD_H__
