#include "MonitorInstance.h"
#include "MonitorStat.h"
#include "WsMonitorInstanceThread.h"
#include "ws-ccxt-cpp/it-websocket.h"
#include <thread>

namespace base {

WsMonitorInstanceThread::WsMonitorInstanceThread(Monitor &monitor, MonitorInstance &inst, std::string exchange,
                                                 const base::MonitorExchangeConfig &cfg, std::vector<std::string> markets)
        : MonitorInstanceThread(monitor, inst, exchange, cfg), markets(markets) {
}

void WsMonitorInstanceThread::run() {
    std::set<int> gotNoTimeout;
    std::map<int, double> responseTimeout;
    auto *client = wsccxt::WebSocketBase::get(exchange, inst.depthMap);
    //client->betweenConnectionsTimeout = config.wsBetweenConnectionsTimeout;
    //client->betweenSubscribesTimeout = config.wsBetweenSubscribesTimeout;
    client->thread = this;
    this->stat.websocket = true;
    debug_timemark = base::microtime();

    base::Log::log(LOG_LEVEL_INFO, "ws thread: connecting to markets " + exchange);
    // total connection count
    unsigned count;
    if (this->config.wsScanPairs.empty()) {
        if (this->markets.empty()) return;
        this->initChunks(markets, config.wsConnectionsPerThread, config.connectionsMultiplier);
    }
    else {
        std::vector<std::string> tmp = this->config.wsScanPairs;
        for (int i = 0; i < tmp.size(); ++i)
            if (this->inst.markets.find(tmp[i]) == this->inst.markets.end())
                tmp.erase(tmp.begin() + i), --i;
        this->initChunks(tmp, config.wsConnectionsPerThread, config.connectionsMultiplier);
    }

    try {
        for (count = 0; count < this->chunks.size(); ++count) {
            auto item = this->chunks[count];
            std::map<std::string, std::string> options;
            auto proxy = this->curl->getProxy();
            if (proxy) options["proxy"] = proxy->connect_string;
            auto *session = client->connect("", options);
            this->connById[count] = session;
            this->connIdBySession[session] = count;
            this->connReSubscribeFreeTsQueue[base::microtime() + this->config.wsReSubscribeTimeout/1000.] = count;
            if (!session) {
                this->connToRecreate.insert(count);
                base::Log::log(LOG_LEVEL_WARNING, "ws: could not create a session");
                continue;
            }
            this->connReadyById.insert(count);
            if (exchange == "bitfinex" && this->chunks[count].size() > 250)
                throw std::runtime_error("bitfinex doesn't allow more than 250 subscriptions per connection");
            if (!this->config.wsReSubscribe) {
                for (int i = 0; i < this->chunks[count].size(); ++i) {
                    client->subscribe(session, this->nextChunk(count));
                    std::this_thread::sleep_for(std::chrono::milliseconds(config.wsBetweenSubscribesTimeout));
                }
            } else
                gotNoTimeout.insert(count);
            std::this_thread::sleep_for(std::chrono::milliseconds(config.wsBetweenConnectionsTimeout));
            responseTimeout[count] = base::microtime() + this->config.wsReConnectNoStakanTimeout/1000.;
        }
    } catch(std::exception const& e) {
        base::Log::log(LOG_LEVEL_WARNING, std::string("ws monitor instance thread err: ") + e.what());
    }
    this->connectionCount = count;

    std::map<std::string, ccxt::Depth> stakans;
    ccxt::Updates updates;
    MonitorStat monitorStat;
    monitorStat.websocket = true;
    bool newData = false;
    while (true) {
        int count = 0;
        std::map<double, int>::iterator tsQueueIt;
        for (auto it = client->sessionList.begin(); it != client->sessionList.end(); ++it)
            count += (*it)->pioc->poll_one();  //we don't use `count' later...
        double timeStart = base::microtime();
        if (client->wsEventList.size()) {
            for (auto it = client->wsEventList.begin(); it != client->wsEventList.end(); ++it) {
                auto &item = *it;
                double timeStart = base::microtime();
                if (item.type == WS_EVENT_READ) {
                    auto parseResult = client->parse(item.data, item.pws);
                    if (parseResult.success) {
                        if (parseResult.type == ccxt::ParseResult::FullBook) {
                            for (auto it1 = parseResult.depth.begin(); it1 != parseResult.depth.end(); ++it1) {
                                stakans[it1->first] = it1->second;
                                newData = true;
                                if (client->hasSeqNo) {
                                    item.pws->sequence[it1->first] = it1->second.sequence;
                                }
                                //base::Log::log(LOG_LEVEL_WARNING, it1->first + " full seq " + std::to_string(it1->second.sequence));
                            }
                            gotNoTimeout.insert(this->connIdBySession[item.pws]);
                            monitorStat.wsNewDataTime += base::microtime() - timeStart;
                            monitorStat.wsStakanEvents++;
                        } else if (parseResult.type == ccxt::ParseResult::Update) {
                            updates = parseResult.updates;
                            if (client->hasSeqNo && updates.data.size()) {
                                auto mkt = updates.data.begin()->first;
                                //base::Log::log(LOG_LEVEL_WARNING, mkt + " upd seq " + std::to_string(updates.sequence));
                                if (item.pws->sequence[mkt] + 1 != updates.sequence) {
                                    base::Log::log(LOG_LEVEL_WARNING, "ws: broken update seq " + mkt + " "
                                        + std::to_string(updates.sequence) + " instead of "
                                        + std::to_string(item.pws->sequence[mkt] + 1));
                                    // TODO разные варианты случая сбитого номера
                                    // пока что пересоединяем
                                    this->connToRecreate.insert(this->connIdBySession[item.pws]);
                                    parseResult.updates.data.clear();
                                } else {
                                    item.pws->sequence[mkt]++;
                                }
                            }
                            if (parseResult.updates.data.size()) newData = true;
                            gotNoTimeout.insert(this->connIdBySession[item.pws]);
                            monitorStat.wsNewDataTime += base::microtime() - timeStart;
                            monitorStat.wsStakanEvents++;
                        } else if (parseResult.type == ccxt::ParseResult::SubscribeResponse) {
                            responseTimeout[this->connIdBySession[item.pws]] = base::microtime() + this->config.wsReConnectNoStakanTimeout/1000.;
                            gotNoTimeout.insert(this->connIdBySession[item.pws]);
                            continue;
                        } else if (parseResult.type == ccxt::ParseResult::Ping) {
                            responseTimeout[this->connIdBySession[item.pws]] = base::microtime() + this->config.wsReConnectNoStakanTimeout/1000.;
                            gotNoTimeout.insert(this->connIdBySession[item.pws]);
                            continue;
                        } else {
                            throw "not implemented";
                        }
                    }
                    else {
                        base::Log::log(LOG_LEVEL_TRACE, "Parse error: " + parseResult.error);
                    }
                } else if (item.type == WS_EVENT_EXCEPTION) {
                    this->onSockectError(item.pws, monitorStat);
                }
            }
            if (client->wsEventList.empty())
                base::Log::log(LOG_LEVEL_TRACE, "ws event list empty");
            client->wsEventList.clear();
        } else if (newData) {
            if (stakans.size()) {
                if (config.verifyDepthOrder)
                    ccxt::Exchange::verifyDepth(stakans);
                processUpdate(stakans, CCXT_SOURCE_WS_FULL);
                this->inst.processUpdate(stakans, CCXT_SOURCE_WS_FULL);
                stakans = {};
                monitorStat.wsStepCalculationTime += base::microtime() - timeStart;
                monitorStat.wsStakanEvents++;
            }
            if (updates.data.size()) {
                auto st = this->processRow(updates);
                if (config.verifyDepthOrder)
                    ccxt::Exchange::verifyDepth(st);
                this->inst.processUpdate(st, CCXT_SOURCE_WS_UPDATE);
                updates.data.clear();
                monitorStat.wsStepCalculationTime += base::microtime() - timeStart;
                monitorStat.wsStakanEvents++;
            }
            newData = false;
        } else if (this->config.wsReSubscribe && this->connReSubscribeFreeTsQueue.size()
                   //resubscribe
                   && ((tsQueueIt = [&]{
                       auto it = this->connReSubscribeFreeTsQueue.begin();
                       for (; it != this->connReSubscribeFreeTsQueue.end(); ++it){
                            if (this->connReadyById.find(it->second) == this->connReadyById.end()) continue;
                            auto mt = base::microtime();
                            if (it->first > mt) return this->connReSubscribeFreeTsQueue.end();
                            if (gotNoTimeout.find(it->second) != gotNoTimeout.end()) return it;
                       }
                       return it;}()) != this->connReSubscribeFreeTsQueue.end())) {

            int connId = tsQueueIt->second;
            this->connReSubscribeFreeTsQueue.erase(tsQueueIt);
            this->connReSubscribeFreeTsQueue[base::microtime()
                                             + (double) this->config.wsReSubscribeTimeout / 1000] = connId;

            //todo flag if unsubscribe needed for resubscribe
            std::vector<std::string> v = this->nextChunk(connId);
            if (client->unsubscribe(this->connById[connId], v))
                this->onSockectError(this->connById[connId], monitorStat);
            if (client->subscribe(this->connById[connId], v))
                this->onSockectError(this->connById[connId], monitorStat);
            gotNoTimeout.erase(connId);
            responseTimeout[connId] = base::microtime() + this->config.wsReConnectNoStakanTimeout/1000.;
            monitorStat.wsStepResubscribeTime += base::microtime() - timeStart;
            monitorStat.wsResubscriptions++;
        } else if (this->config.wsReSubscribe && this->connReSubscribeFreeTsQueue.size()
            && responseTimeout[this->connReSubscribeFreeTsQueue.begin()->second] < base::microtime()
            && this->connReadyById.find(this->connReSubscribeFreeTsQueue.begin()->second) != this->connReadyById.end()
            && this->connToRecreate.find(this->connReSubscribeFreeTsQueue.begin()->second) == this->connToRecreate.end()) {

            this->connToRecreate.insert(this->connReSubscribeFreeTsQueue.begin()->second);
//std::cout << "Request to reconnect because of timeout: " << responseTimeout[this->connReSubscribeFreeTsQueue.begin()->second] - debug_timemark <<  ' ' << microtime() - debug_timemark << "\n";

            monitorStat.wsReconnectTimedOutCount++;
        } else if (this->connToRecreate.size()
            && base::microtime() - this->lastReconnect > (double) this->config.wsReConnectTimeout /1000) {

            base::Log::log(LOG_LEVEL_WARNING, std::string("We want to reconnect: " ) + std::to_string(this->connToRecreate.size()));
            this->lastReconnect = base::microtime();
            auto it = this->connToRecreate.begin();
            //todo proxy for reconnect
            // TODO split reconnect
            // TODO check connById exists
            int connId = *it;
            auto *oldWs = this->connById[connId];
            //todo chunked proxy distrib
            auto proxy = this->curl->getProxy();
            base::Log::log(LOG_LEVEL_WARNING, proxy->connect_string);
            auto *newWs = client->reconnect(this->connById[connId], {{"proxy", proxy->connect_string}});
            if (newWs) {
                this->connToRecreate.erase(*it);
                this->connById[connId] = newWs;
                this->connIdBySession.erase(oldWs);
                this->connIdBySession[newWs] = connId;
                this->connReadyById.insert(connId);
                if (this->connReSubscribeFreeTsQueue.size())
                    for (auto itReSub = this->connReSubscribeFreeTsQueue.begin(); itReSub != this->connReSubscribeFreeTsQueue.end();) {
                        if (itReSub->second == connId) {
                            itReSub = this->connReSubscribeFreeTsQueue.erase(itReSub);
                        } else {
                            itReSub++;
                        }
                    }

                if (!this->config.wsReSubscribe) {
                    for (int i = 0; i < this->chunks[connId].size(); ++i) {
                        client->subscribe(newWs, this->nextChunk(connId));
                        std::this_thread::sleep_for(std::chrono::milliseconds(config.wsBetweenSubscribesTimeout));
                    }
                } else {
                    // пометить что таймаута по этому соединению не было
                    gotNoTimeout.insert(connId);
                }

                this->connReSubscribeFreeTsQueue[base::microtime() + (double) this->config.wsReSubscribeTimeout / 1000] = connId;
                monitorStat.wsStepReconnectTime += base::microtime() - timeStart;
                monitorStat.wsReconnections++;
            }
            else {
                 base::Log::log(LOG_LEVEL_TRACE, "reconnect error on conn " + std::to_string(connId));
                 //todo add rate limit for reconnect
            }
        } else {
            usleep(100);
            monitorStat.wsStepIdleTime += base::microtime() - timeStart;
        }
        // TODO other states
        monitorStat.eventCount = this->stat.eventCount;
        monitorStat.wsConnections = this->stat.wsConnections;
        monitorStat.wsSubscriptions = this->stat.wsSubscriptions;
        //todo fill global?
        this->stat = monitorStat;
    }
}

void WsMonitorInstanceThread::onSockectError(wsccxt::WSsession *pws, MonitorStat &monitorStat) {
    this->lastReconnect = base::microtime();
    int id = this->connIdBySession[pws];
    this->connReadyById.erase(id);
    this->connToRecreate.insert(id);
    monitorStat.wsErrorEvents++;
    base::Log::log(LOG_LEVEL_WARNING, std::string("Reconnect queue size: " ) + std::to_string(this->connToRecreate.size()));
    for (auto it = this->connToRecreate.begin(); it != this->connToRecreate.end(); it++)
        base::Log::log(LOG_LEVEL_WARNING, std::string("Reconnect queue: " ) + std::to_string(*it));
}

void WsMonitorInstanceThread::initChunks(std::vector<std::string> markets, int connCount, int connMultiplier) {
    for (int j = 0; j < connMultiplier; ++j) {
        std::vector<std::vector<std::string>> preChunks = base::array_split(markets, connCount);
        int i = 0;
        while (connCount > preChunks.size())
            preChunks.push_back(preChunks[i++]);
        for (auto it = preChunks.begin(); it != preChunks.end(); it++) {
            this->chunks.push_back(array_chunk(*it, this->config.wsChunkSize));
            this->chunkIndexes.push_back(0);
        }
    }
}

std::vector<std::string> WsMonitorInstanceThread::nextChunk(int connId) {
    if (this->chunks.size() <= connId) return {};
    this->chunkIndexes[connId] = (this->chunkIndexes[connId] + 1) % this->chunks[connId].size();
    return this->chunks[connId][this->chunkIndexes[connId]];
}

}
