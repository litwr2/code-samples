#include "monitor.h"
#include "base/Log.h"
#include "ccxt/exchange.h"
#include "ccxt/bot.h"
#include "ws-ccxt-cpp/it-websocket.h"
#include "WsMonitorInstance.h"
#include <algorithm>
#include <iostream>
#include <list>
#include <rapidjson/document.h>
#include <string>
#include <thread>
#include <tuple>
#include <my_global.h>
#include <mysql.h>
//#define MYSQLPP_SSQLS_NO_STATICS
//#include "../sql/SqlBase.h"

namespace base {

rapidjson::Document parseConfig(const char *str) {
    rapidjson::Document d;
    d.Parse(str);
    return d;
}

base::MonitorExchangeConfig& Monitor::getExConfig(std::string exchange) {
    for (auto it = this->config[exchange].begin(); it != this->config[exchange].end(); ++it) {
        if (it->useWebsocket) continue;
        return *it;
    }
    return this->config[exchange].front();
}

void Monitor::saveLastStat(std::string exchange, int instanceId, std::map<std::string, double> lastStatCur) {
    this->lastStatLock.lock();
    this->lastStat[instanceId] = lastStatCur;
    this->lastStatLock.unlock();


    //todo
    this->lastStatCondLock.lock();
    //todo iterate instanceId for multiple exchanges
    if (this->config.find(exchange) != this->config.end() && this->config[exchange].size() > instanceId
        && this->config[exchange][instanceId].statConditions.size() > 0)
        for (auto it = this->config[exchange][instanceId].statConditions.begin(); it != this->config[exchange][instanceId].statConditions.end(); it++) {
            bool statCondOk = (lastStatCur.find(it->indicator) != lastStatCur.end()
                               && (!it->hasMin || it->min <= lastStatCur[it->indicator])
                               && (!it->hasMax || it->max >= lastStatCur[it->indicator]));
            if (it->hasPeriod) {
                this->lastStatCondHist[it->name][base::microtime()] = statCondOk;
                //очистить старые элементы
                while (this->lastStatCondHist[it->name].size() > 0 && base::microtime() - this->lastStatCondHist[it->name].begin()->first > it->period)
                    this->lastStatCondHist[it->name].erase(this->lastStatCondHist[it->name].begin());

                double periodPerc = 0.0;
                int cnt = 0;
                int total = 0;
                if (this->lastStatCondHist[it->name].size() > 0) {
                    for (auto it1 = this->lastStatCondHist[it->name].begin();
                         it1 != this->lastStatCondHist[it->name].end(); it1++) {
                        total++;
                        if (it1->second) cnt++;
                    }
                    periodPerc = (double) cnt / (double) total * 100;
                }

                this->lastStatPeriod[instanceId][it->name] = periodPerc;
                this->lastStatCond[it->name] = ((!it->hasPeriodMin || it->periodMin <= periodPerc) && (!it->hasPeriodMax || it->periodMax >= periodPerc));

            } else {
                this->lastStatCond[it->name] = statCondOk;
            }
        }
    //todo lock only on save
    this->lastStatCondLock.unlock();
}

    std::vector<std::map<std::string, double> > Monitor::getLastStat() {
        //todo all instances
        std::vector<std::map<std::string, double> > r;
        this->lastStatLock.lock();
        r = this->lastStat;
        this->lastStatLock.unlock();
        return r;
    }

    std::vector<std::map<std::string, double> > Monitor::getLastStatPeriod() {
        //todo all instances
        std::vector<std::map<std::string, double> > r;
        this->lastStatCondLock.lock();
        r = this->lastStatPeriod;
        this->lastStatCondLock.unlock();
        return r;
    }

    std::map<std::string, bool> Monitor::getLastStatCond() {
    std::map<std::string, bool> r;
    this->lastStatCondLock.lock();
    r = this->lastStatCond;
    this->lastStatCondLock.unlock();
    return r;
}

base::MonitorExchangeConfig Monitor::parseSingleConfig(rapidjson::Value &itr) {
    base::MonitorExchangeConfig cfg;

    this->hasConfig = true;

    if (itr.HasMember("threads")) {
        const auto &jsonThreads = itr["threads"];
        if (jsonThreads.IsNumber())
            cfg.threads = jsonThreads.GetInt();
    }

    if (itr.HasMember("curlScanTimeout")) {
        const rapidjson::Value &jsonValue = itr["curlScanTimeout"];
        if (jsonValue.IsNumber())
            cfg.curlScanTimeout = jsonValue.GetInt();
    }

    if (itr.HasMember("scanIp")) {
        const rapidjson::Value &jsonValue = itr["scanIp"];
        if (jsonValue.IsString())
            cfg.scanIp = jsonValue.GetString();
    }

    if (itr.HasMember("connectionsMultiplier")) {
        const rapidjson::Value &jsonValue = itr["connectionsMultiplier"];
        if (jsonValue.IsNumber())
            cfg.connectionsMultiplier = jsonValue.GetInt();
    }


    if (itr.HasMember("wsConnectionsPerThread")) {
        const rapidjson::Value &jsonValue = itr["wsConnectionsPerThread"];
        if (jsonValue.IsNumber())
            cfg.wsConnectionsPerThread = jsonValue.GetInt();
    }

    if (itr.HasMember("wsScanPairs")) {
        const rapidjson::Value &jsonValue = itr["wsScanPairs"];
        if (jsonValue.IsArray())
            for (auto it = jsonValue.Begin(); it != jsonValue.End(); ++it)
                if (it->IsString())
                    cfg.wsScanPairs.push_back(it->GetString());
    }

    if (itr.HasMember("serverIpList")) {
        const rapidjson::Value &jsonValue = itr["serverIpList"];
        if (jsonValue.IsArray())
            for (auto it = jsonValue.Begin(); it != jsonValue.End(); ++it)
                if (it->IsString())
                    cfg.serverIpList.push_back(it->GetString());
    }

    if (itr.HasMember("subcheckCopies")) {
        const rapidjson::Value &jsonValue = itr["subcheckCopies"];
        if (jsonValue.IsNumber())
            cfg.subcheckCopies = jsonValue.GetInt();
    }

    if (itr.HasMember("verifyDepthOrder")) {
        const rapidjson::Value &jsonValue = itr["verifyDepthOrder"];
        if (jsonValue.IsBool())
            cfg.verifyDepthOrder = jsonValue.GetBool();
    }

    if (itr.HasMember("subcheckSingleConnPerProxy")) {
        const rapidjson::Value &jsonValue = itr["subcheckSingleConnPerProxy"];
        if (jsonValue.IsBool())
            cfg.subcheckSingleConnPerProxy = jsonValue.GetBool();
    }

    if (itr.HasMember("subcheckConnPerProxy")) {
        const rapidjson::Value &jsonValue = itr["subcheckConnPerProxy"];
        if (jsonValue.IsNumber())
            cfg.subcheckConnPerProxy = jsonValue.GetInt();
    }

    // ws options

    if (itr.HasMember("wsThreads")) {
        const auto &jsonThreads = itr["wsThreads"];
        if (jsonThreads.IsNumber())
            cfg.wsThreads = jsonThreads.GetInt();
    }

    if (itr.HasMember("wsChunkSize")) {
        const rapidjson::Value &jsonValue = itr["wsChunkSize"];
        if (jsonValue.IsInt())
            cfg.wsChunkSize = jsonValue.GetInt();
    }

    if (itr.HasMember("wsMarketsPerThread")) {
        const rapidjson::Value &jsonValue = itr["wsMarketsPerThread"];
        if (jsonValue.IsNumber())
            cfg.wsMarketsPerThread = jsonValue.GetInt();
    }

    if (itr.HasMember("wsMarketsLimit")) {
        const rapidjson::Value &jsonValue = itr["wsMarketsLimit"];
        if (jsonValue.IsNumber())
            cfg.wsMarketsLimit = jsonValue.GetInt();
    }

    if (itr.HasMember("wsBetweenConnectionsTimeout")) {
        const rapidjson::Value &jsonValue = itr["wsBetweenConnectionsTimeout"];
        if (jsonValue.IsNumber())
            cfg.wsBetweenConnectionsTimeout = jsonValue.GetInt();
    }

    if (itr.HasMember("wsReSubscribe")) {
        const rapidjson::Value &jsonValue = itr["wsReSubscribe"];
        if (jsonValue.IsBool())
            cfg.wsReSubscribe = jsonValue.GetBool();
    }

    if (itr.HasMember("wsReSubscribeTimeout")) {
        const rapidjson::Value &jsonValue = itr["wsReSubscribeTimeout"];
        if (jsonValue.IsNumber())
            cfg.wsReSubscribeTimeout = jsonValue.GetInt();
    }

    if (itr.HasMember("wsReConnectNoStakanTimeout")) {
        const rapidjson::Value &jsonValue = itr["wsReConnectNoStakanTimeout"];
        if (jsonValue.IsNumber())
            cfg.wsReConnectNoStakanTimeout = jsonValue.GetInt();
    }

    if (itr.HasMember("wsReConnectTimeout")) {
        const rapidjson::Value &jsonValue = itr["wsReConnectTimeout"];
        if (jsonValue.IsNumber())
            cfg.wsReConnectTimeout = jsonValue.GetInt();
    }

    if (itr.HasMember("wsBetweenSubscribesTimeout")) {
        const rapidjson::Value &jsonValue = itr["wsBetweenSubscribesTimeout"];
        if (jsonValue.IsNumber())
            cfg.wsBetweenSubscribesTimeout = jsonValue.GetInt();
    }

    // end deprecated

    if (itr.HasMember("cryptopiaShortScan")) {
        const rapidjson::Value &jsonValue = itr["cryptopiaShortScan"];
        if (jsonValue.IsBool())
            cfg.cryptopiaShortScan = jsonValue.GetBool();
    }

    if (itr.HasMember("groupSize")) {
        const rapidjson::Value &jsonValue = itr["groupSize"];
        if (jsonValue.IsNumber())
            cfg.groupSize = jsonValue.GetInt();
    }

    if (itr.HasMember("multiCurlHandlesPerThread")) {
        const rapidjson::Value &jsonValue = itr["multiCurlHandlesPerThread"];
        if (jsonValue.IsNumber())
            cfg.multiCurlHandlesPerThread = jsonValue.GetInt();
    }

    if (itr.HasMember("botId")) {
        const rapidjson::Value &jsonValue = itr["botId"];
        if (jsonValue.IsNumber())
            cfg.botIds.push_back(jsonValue.GetInt());
    }

    if (itr.HasMember("botIds")) {
        const rapidjson::Value &jsonValue = itr["botIds"];
        if (jsonValue.IsArray())
            for (auto jt = jsonValue.Begin(); jt != jsonValue.End(); ++jt) {
                auto &val = *jt;
                if (val.IsInt())
                    cfg.botIds.push_back(val.GetInt());
            }
    }

    if (itr.HasMember("baseCurs")) {
        const rapidjson::Value &jsonBc = itr["baseCurs"];
        if (jsonBc.IsArray()) {
            for (auto jt = jsonBc.Begin(); jt != jsonBc.End(); ++jt) {
                auto &val = *jt;
                if (val.IsString())
                    cfg.baseCurs.push_back(val.GetString());
            }
        }
    }

    if (itr.HasMember("cicleUsleep")) {
        const rapidjson::Value &jsonUsleep = itr["cicleUsleep"];
        if (jsonUsleep.IsNumber())
            cfg.cicleUsleep = jsonUsleep.GetInt();
    }

    if (itr.HasMember("groupHandlesReserve")) {
        const rapidjson::Value &jsonValue = itr["groupHandlesReserve"];
        if (jsonValue.IsNumber())
            cfg.groupHandlesReserve = jsonValue.GetInt();
    }

    if (itr.HasMember("fastTokensOnly")) {
        const rapidjson::Value &jsonValue = itr["fastTokensOnly"];
        if (jsonValue.IsBool())
            cfg.fastTokensOnly = jsonValue.GetBool();
    }

    if (itr.HasMember("noProxy")) {
        const rapidjson::Value &jsonValue = itr["noProxy"];
        if (jsonValue.IsBool())
            cfg.noProxy = jsonValue.GetBool();
    }

    if (itr.HasMember("singleConnPerProxy")) {
        const rapidjson::Value &jsonValue = itr["singleConnPerProxy"];
        if (jsonValue.IsBool())
            cfg.singleConnPerProxy = jsonValue.GetBool();
    }

    if (itr.HasMember("connPerProxy")) {
        const rapidjson::Value &jsonValue = itr["connPerProxy"];
        if (jsonValue.IsInt())
            cfg.connPerProxy = jsonValue.GetInt();
    }

    if (itr.HasMember("useHset")) {
        const rapidjson::Value &jsonValue = itr["useHset"];
        if (jsonValue.IsBool())
            cfg.useHset = jsonValue.GetBool();
    }

    if (itr.HasMember("useTickerScan")) {
        const rapidjson::Value &jsonValue = itr["useTickerScan"];
        if (jsonValue.IsBool())
            cfg.useTickerScan = jsonValue.GetBool();
    }

    if (itr.HasMember("useWebsocket")) {
        const rapidjson::Value &jsonValue = itr["useWebsocket"];
        if (jsonValue.IsBool())
            cfg.useWebsocket = jsonValue.GetBool();
    }

    if (itr.HasMember("enableSubcheckWarmup")) {
        const rapidjson::Value &jsonValue = itr["enableSubcheckWarmup"];
        if (jsonValue.IsBool())
            cfg.enableSubcheckWarmup = jsonValue.GetBool();
    }

    if (itr.HasMember("saveDepth")) {
        const rapidjson::Value &jsonValue = itr["saveDepth"];
        if (jsonValue.IsBool())
            cfg.saveDepth = jsonValue.GetBool();
    }

    if (itr.HasMember("saveAlgo")) {
        const rapidjson::Value &jsonValue = itr["saveAlgo"];
        if (jsonValue.IsBool())
            cfg.saveAlgo = jsonValue.GetBool();
    }

    if (itr.HasMember("enableProxyBan")) {
        const rapidjson::Value &jsonValue = itr["enableProxyBan"];
        if (jsonValue.IsBool())
            cfg.enableProxyBan = jsonValue.GetBool();
    }

    if (itr.HasMember("chunkSize")) {
        const rapidjson::Value &jsonValue = itr["chunkSize"];
        if (jsonValue.IsInt())
            cfg.chunkSize = jsonValue.GetInt();
    }

    if (itr.HasMember("enableProxyBanNoStakan")) {
        const rapidjson::Value &jsonValue = itr["enableProxyBanNoStakan"];
        if (jsonValue.IsInt())
            cfg.enableProxyBanNoStakan = jsonValue.GetInt();
    }

    if (itr.HasMember("outputResponseNoStakan")) {
        const rapidjson::Value &jsonValue = itr["outputResponseNoStakan"];
        if (jsonValue.IsBool())
            cfg.outputResponseNoStakan = jsonValue.GetBool();
    }

    if (itr.HasMember("isJsonValidCheck")) {
        const rapidjson::Value &jsonValue = itr["isJsonValidCheck"];
        if (jsonValue.IsBool())
            cfg.isJsonValidCheck = jsonValue.GetBool();
    }

    if (itr.HasMember("successKeywords")) {
        const rapidjson::Value &jsonFt = itr["successKeywords"];
        for (auto fti = jsonFt.Begin(); fti != jsonFt.End(); ++fti) {
            if (fti->IsString()) {
                cfg.successKeywords.push_back(fti->GetString());
            }
        }
    }

    if (itr.HasMember("rateLimitKeywords")) {
        const rapidjson::Value &jsonFt = itr["rateLimitKeywords"];
        for (auto fti = jsonFt.Begin(); fti != jsonFt.End(); ++fti) {
            if (fti->IsString()) {
                cfg.rateLimitKeywords.push_back(fti->GetString());
            }
        }
    }

    if (itr.HasMember("rateLimitCodes")) {
        const rapidjson::Value &jsonFt = itr["rateLimitCodes"];
        for (auto fti = jsonFt.Begin(); fti != jsonFt.End(); ++fti) {
            if (fti->IsInt()) {
                cfg.rateLimitCodes.push_back(fti->GetInt());
            }
        }
    }

    if (itr.HasMember("noReconnectKeywords")) {
        const rapidjson::Value &jsonFt = itr["noReconnectKeywords"];
        for (auto fti = jsonFt.Begin(); fti != jsonFt.End(); ++fti) {
            if (fti->IsString()) {
                cfg.noReconnectKeywords.push_back(fti->GetString());
            }
        }
    }

    if (itr.HasMember("noReconnectCodes")) {
        const rapidjson::Value &jsonFt = itr["noReconnectCodes"];
        for (auto fti = jsonFt.Begin(); fti != jsonFt.End(); ++fti) {
            if (fti->IsInt()) {
                cfg.noReconnectCodes.push_back(fti->GetInt());
            }
        }
    }

    if (itr.HasMember("statFile")) {
        const rapidjson::Value &jsonValue = itr["statFile"];
        if (jsonValue.IsString())
            cfg.statFile = jsonValue.GetString();
    }

    if (itr.HasMember("proxyDistrib")) {
        const rapidjson::Value &jsonValue = itr["proxyDistrib"];
        if (jsonValue.IsString())
            cfg.proxyDistrib = jsonValue.GetString();
    }

    if (itr.HasMember("proxyOrderBy")) {
        const rapidjson::Value &jsonValue = itr["proxyOrderBy"];
        if (jsonValue.IsString())
            cfg.proxyOrderBy = jsonValue.GetString();
    }

    if (itr.HasMember("proxyCond")) {
        const rapidjson::Value &jsonValue = itr["proxyCond"];
        if (jsonValue.IsString())
            cfg.proxyCond = jsonValue.GetString();
    }

    if (itr.HasMember("proxySubcheckCond")) {
        const rapidjson::Value &jsonValue = itr["proxySubcheckCond"];
        if (jsonValue.IsString())
            cfg.proxySubcheckCond = jsonValue.GetString();
    }

    if (itr.HasMember("queues")) {
        const rapidjson::Value &jsonFt = itr["queues"];
        for (auto fti = jsonFt.Begin(); fti != jsonFt.End(); ++fti) {
            const rapidjson::Value &strValue = (*fti)["name"];
            std::string queueName = "";
            if (strValue.IsString()) {
                queueName = strValue.GetString();
            }
            if (fti->IsObject()) {
                const rapidjson::Value &tokens = (*fti)["tokens"];
                for (auto tokenIt = tokens.Begin(); tokenIt != tokens.End(); ++tokenIt) {
                    cfg.queues[queueName].tokens.push_back(tokenIt->GetString());
                }
            }
        }
    }

    if (itr.HasMember("fastTokens")) {
        const rapidjson::Value &jsonFt = itr["fastTokens"];
        for (auto fti = jsonFt.Begin(); fti != jsonFt.End(); ++fti) {
            if (fti->IsObject()) {
                const rapidjson::Value &tokens = (*fti)["tokens"];
                for (auto tokenIt = tokens.Begin(); tokenIt != tokens.End(); ++tokenIt) {
                    cfg.fastTokens[tokenIt->GetString()] = (*fti)["faster"].GetInt();
                }
            }
        }
    }
    if (itr.HasMember("scanDomains")) {
        const rapidjson::Value &jsonFt = itr["scanDomains"];
        for (auto fti = jsonFt.Begin(); fti != jsonFt.End(); ++fti) {
            if (fti->IsString()) {
                cfg.scanDomains.push_back(fti->GetString());
            }
        }
    }
    if (itr.HasMember("scanInterfaces")) {
        const rapidjson::Value &jsonFt = itr["scanInterfaces"];
        for (auto fti = jsonFt.Begin(); fti != jsonFt.End(); ++fti) {
            if (fti->IsString()) {
                cfg.scanInterfaces.push_back(fti->GetString());
            }
        }
    }

    if (itr.HasMember("scanBases")) {
        const rapidjson::Value &jsonFt = itr["scanBases"];
        for (auto fti = jsonFt.Begin(); fti != jsonFt.End(); ++fti) {
            if (fti->IsString()) {
                cfg.scanBases.push_back(fti->GetString());
            }
        }
    }

    if (itr.HasMember("proxyRateLimit")) {
        const rapidjson::Value &jsonValue = itr["proxyRateLimit"];
        if (jsonValue.IsNumber())
            cfg.proxyRateLimit = jsonValue.GetDouble();
    }

    if (itr.HasMember("curlReloadEvery")) {
        const rapidjson::Value &jsonValue = itr["curlReloadEvery"];
        if (jsonValue.IsNumber())
            cfg.curlReloadEvery = jsonValue.GetDouble();
    }

    if (itr.HasMember("curlReloadWait")) {
        const rapidjson::Value &jsonValue = itr["curlReloadWait"];
        if (jsonValue.IsNumber())
            cfg.curlReloadWait = jsonValue.GetDouble();
    }

    if (itr.HasMember("reconnectPerThreadRateLimit")) {
        const rapidjson::Value &jsonValue = itr["reconnectPerThreadRateLimit"];
        if (jsonValue.IsNumber())
            cfg.reconnectPerThreadRateLimit = jsonValue.GetDouble();
    }

    if (itr.HasMember("proxyCondLimit")) {
        const rapidjson::Value &jsonValue = itr["proxyCondLimit"];
        if (jsonValue.IsNumber())
            cfg.proxyCondLimit = jsonValue.GetInt();
    }

    if (itr.HasMember("proxyBanTimeout")) {
        const rapidjson::Value &jsonValue = itr["proxyBanTimeout"];
        if (jsonValue.IsNumber())
            cfg.proxyBanTimeout = jsonValue.GetInt();
    }

    if (itr.HasMember("statConditions") && itr.FindMember("statConditions")->value.IsArray()) {
        for (auto it = itr.FindMember("statConditions")->value.Begin(); it != itr.FindMember("statConditions")->value.End(); ++it) {
            auto &val = *it;
            if (val.IsObject() && val.HasMember("indicator") && val.HasMember("name")
                && val.FindMember("indicator")->value.IsString() && val.FindMember("name")->value.IsString()) {
                MonitorStatConditions sc;
                sc.indicator = val.FindMember("indicator")->value.GetString();
                sc.name = val.FindMember("name")->value.GetString();
                if (val.HasMember("instanceId") && val.FindMember("instanceId")->value.IsInt())
                    sc.instanceId = val.FindMember("instanceId")->value.GetInt();
                if (val.HasMember("min") && val.FindMember("min")->value.IsNumber()) {
                    sc.min = val.FindMember("min")->value.GetDouble();
                    sc.hasMin = true;
                }
                if (val.HasMember("max") && val.FindMember("max")->value.IsNumber()) {
                    sc.max = val.FindMember("max")->value.GetDouble();
                    sc.hasMax = true;
                }

                if (val.HasMember("period") && val.FindMember("period")->value.IsNumber()) {
                    sc.period = val.FindMember("period")->value.GetDouble();
                    sc.hasPeriod = true;
                }
                if (val.HasMember("periodMin") && val.FindMember("periodMin")->value.IsNumber()) {
                    sc.periodMin = val.FindMember("periodMin")->value.GetDouble();
                    sc.hasPeriodMin = true;
                }
                if (val.HasMember("periodMax") && val.FindMember("periodMax")->value.IsNumber()) {
                    sc.periodMax = val.FindMember("periodMax")->value.GetDouble();
                    sc.hasPeriodMax = true;
                }

                if (sc.hasMin || sc.hasMax)
                    cfg.statConditions.push_back(sc);
            }
        }
    }

    return cfg;
}

Monitor::Monitor(std::string cfgStr) {
    rapidjson::Document config = parseConfig(cfgStr.c_str());
    rapidjson::Value &logLevelVal = config["logLevel"];
    if (logLevelVal.IsString()) {
        std::string logLevelStr = logLevelVal.GetString();
        int logLevel = LOG_LEVEL_WARNING;
        if (logLevelStr == "warning" || logLevelStr == "warn") {
            logLevel = LOG_LEVEL_WARNING;
        } else if (logLevelStr == "info") {
            logLevel = LOG_LEVEL_INFO;
        } else if (logLevelStr == "debug") {
            logLevel = LOG_LEVEL_DEBUG;
        } else if (logLevelStr == "trace") {
            logLevel = LOG_LEVEL_TRACE;
        } else if (logLevelStr == "stat") {
            logLevel = LOG_LEVEL_STAT;
        }
        base::Log::logLevel = logLevel;
        base::Log::log(LOG_LEVEL_INFO, std::string("set log level to ") + logLevelStr);
    }

    if (config.HasMember("logOutput")) {
        const rapidjson::Value &jsonValue = config["logOutput"];
        if (jsonValue.IsString()) {
                char c = toupper(jsonValue.GetString()[0]);
                switch (c) {
                    case 'C':
                    case '0':
                        Log::logOutput = Log::Console;
                        break;
                    case 'F':
                    case '1':
                        Log::logOutput = Log::File;
                        break;
                    case 'B':
                    case '2':
                        Log::logOutput = Log::Both;
                        break;
                    default:
                        ;//error?
                }
            }
    }

    if (config.HasMember("server")) {
        if (config["server"].HasMember("enableSocket") && config["server"]["enableSocket"].IsBool())
            this->serverWorkerCfg.enableSocket = config["server"]["enableSocket"].GetBool();
        if (config["server"].HasMember("port") && config["server"]["port"].IsNumber())
            this->serverWorkerCfg.port = config["server"]["port"].GetInt();
    }

    rapidjson::Value &monitorConfig = config["monitor"];
    rapidjson::Value &exConfig = config["monitor"]["exchanges"];
    if (exConfig.IsArray()) {
        for (auto itr = exConfig.Begin(); itr != exConfig.End(); ++itr) {
            const rapidjson::Value &ex = (*itr)["ex"];
            auto cfg = parseSingleConfig(*itr);
            if (std::find(ccxt::Exchange::exchanges.begin(), ccxt::Exchange::exchanges.end(), ex.GetString()) == ccxt::Exchange::exchanges.end())
                continue;
            this->config[ex.GetString()].push_back(cfg);
            std::cout << ex.GetString() << cfg;
        }
    } else if (exConfig.IsObject()) {
        for (auto itr = exConfig.MemberBegin(); itr != exConfig.MemberEnd(); ++itr) {
            auto cfg = parseSingleConfig(itr->value);
            std::string name = itr->name.GetString();
            if (std::find(ccxt::Exchange::exchanges.begin(), ccxt::Exchange::exchanges.end(), name) == ccxt::Exchange::exchanges.end())
                continue;

            this->config[name].push_back(cfg);
            std::cout << name << cfg;
        }
    }
    std::string mergeType = "default";
    for (auto it = monitorConfig.MemberBegin(); it != monitorConfig.MemberEnd(); ++it) {
        std::string name = it->name.GetString();
        if (name == "mergeType" && it->value.IsString()) {
            mergeType = it->value.GetString();
        }
    }
    // for now, global options exist in each MonitorExchangeConfig
    for (auto it = this->config.begin(); it != this->config.end(); ++it) {
        for (auto jt = it->second.begin(); jt != it->second.end(); ++jt) {
            jt->mergeType = mergeType;
        }
    }


    if (config.HasMember("mysqlProxy")) {
        this->mysqlConfProxy.host = config["mysqlProxy"]["host"].GetString();
        this->mysqlConfProxy.user = config["mysqlProxy"]["user"].GetString();
        this->mysqlConfProxy.password = config["mysqlProxy"]["password"].GetString();
        this->mysqlConfProxy.database = config["mysqlProxy"]["database"].GetString();
    }

    if (config.HasMember("mysql")) {
        this->mysqlConf.host = config["mysql"]["host"].GetString();
        this->mysqlConf.user = config["mysql"]["user"].GetString();
        this->mysqlConf.password = config["mysql"]["password"].GetString();
        this->mysqlConf.database = config["mysql"]["database"].GetString();
    }

    // proxy conn
    this->con = mysql_init(NULL);
    // bot conn
    this->conLocal = mysql_init(NULL);
    auto &localMysqlConf = this->mysqlConfProxy.host.length()
        ? this->mysqlConfProxy : this->mysqlConf;
    if (mysql_real_connect(this->con, localMysqlConf.host.c_str(), localMysqlConf.user.c_str(),
                           localMysqlConf.password.c_str(), localMysqlConf.database.c_str(), 0, NULL, 0) == NULL) {
        fprintf(stderr, "%s\n", mysql_error(this->con));
        mysql_close(con);
        base::Log::log(LOG_LEVEL_WARNING, std::string("monitor: could not connect to mysql (proxy conn)"));
        throw "could not connect mysql proxy";
    }
    if (mysql_real_connect(this->conLocal, this->mysqlConf.host.c_str(), this->mysqlConf.user.c_str(),
                           this->mysqlConf.password.c_str(), this->mysqlConf.database.c_str(), 0, NULL, 0) == NULL) {
        fprintf(stderr, "%s\n", mysql_error(this->conLocal));
        mysql_close(conLocal);
        base::Log::log(LOG_LEVEL_WARNING, std::string("monitor: could not connect to mysql"));
        throw "could not connect mysql";
    }

    if (config.HasMember("redis")) {
        this->redisConf.host = config["redis"]["host"].GetString();
        this->redisConf.password = config["redis"]["password"].GetString();
        if (config["redis"]["port"].IsNumber()) {
            this->redisConf.port = config["redis"]["port"].GetInt();
        }
    }
}

void Monitor::initMonitors() {
    if (!this->hasConfig) {
        base::Log::log(LOG_LEVEL_WARNING, std::string("initMonitors: no monitor config"));
        return;
    }

    for (auto it = this->config.begin(); it != this->config.end(); ++it) {
        if (!it->second.size()) continue;
        auto &conf = it->second.front();
        if (!this->caches[it->first]) this->caches[it->first] = new MonitorCache(it->first, conf);
        if (conf.botIds.size() && con) {
            for (auto jt = conf.botIds.begin(); jt != conf.botIds.end(); ++jt) {
                this->caches.at(it->first)->loadBot(con, *jt);
            }
        }
    }

    std::vector<std::thread *> stdThreads;
    auto &localMysqlConf = this->mysqlConfProxy.host.length()
        ? this->mysqlConfProxy : this->mysqlConf;
    std::list<std::string> wsExchanges = wsccxt::WebSocketBase::exchanges;


    for (auto it = this->config.begin(); it != this->config.end(); ++it) {
        std::string exchange = it->first;
        auto markets = this->caches.at(it->first)->ccxt->fetch_markets();

        int instanceCounter = 0;
        this->lastStat.resize(it->second.size());
        this->lastStatPeriod.resize(it->second.size());
        for (auto jt = it->second.begin(); jt != it->second.end(); ++jt) {
            std::thread *t = NULL;

            auto &cfg = *jt;
            if (!this->libMarkets[exchange]) {
                DepthMap *depthMap = new DepthMap(markets);
                this->libMarkets[exchange] = new MarketLib::Markets(exchange, *depthMap, *(this->caches.at(exchange)));
            }

            if (cfg.useWebsocket && std::find(wsExchanges.begin(), wsExchanges.end(), exchange) != wsExchanges.end()) {
                auto inst = new WsMonitorInstance(*this, exchange, cfg, localMysqlConf, markets,
                                                  *this->libMarkets[exchange]);
                inst->instanceId = instanceCounter;
                this->instances[exchange].push_back(inst);
                t = new std::thread(&WsMonitorInstance::run, (WsMonitorInstance *) inst);
            } else {
                auto inst = new MonitorInstance(*this, exchange, cfg, localMysqlConf, markets,
                                                *this->libMarkets[exchange]);
                inst->instanceId = instanceCounter;
                this->instances[exchange].push_back(inst);
                t = new std::thread(&MonitorInstance::run, inst);
            }
            t->detach();
            stdThreads.push_back(t);

            ++instanceCounter;
        }
    }

    base::Log::log(LOG_LEVEL_INFO, std::string("initMonitors: instances started"));
}

Monitor::~Monitor() noexcept(false) {
    base::Log::log(LOG_LEVEL_WARNING, std::string("monitor: must not be destructed"));
    throw "Monitor must not be destructed";
}

} // namespace base
