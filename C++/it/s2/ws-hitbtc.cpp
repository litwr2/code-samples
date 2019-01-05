#include <base/monitor/MonitorInstance.h>
#include "ws-hitbtc.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

namespace wsccxt {

Hitbtc::Hitbtc(base::DepthMap &map) : WebSocketBase(map) {
    load_root_certificates(ctx);
    this->hasSeqNo = true;
}

WSsession* Hitbtc::connect(std::string url, const std::map<std::string, std::string> &options) {
    WSsession *s = new WSsession(ctx, std::bind(&Hitbtc::storeEvent, this,
            std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    std::string localIp = "";
    if (options.find("proxy") != options.end() && options.at("proxy").find("if:") != std::string::npos) {
        localIp = (options.at("proxy").c_str() + 3);
    }
    base::Log::log(LOG_LEVEL_WARNING, std::string("Connect: ") + localIp);
    if (!s->connect(this, host, port, dir, localIp)) {
        return 0;
    }
    sessionList.push_back(s);
    return s;
}

int Hitbtc::subscribe(WSsession *s, const std::vector<std::string> &pairs) {
    if (pairs.empty()) {
        base::Log::log(LOG_LEVEL_WARNING, std::string("hitbtc: empty market pairs for subscribe"));
        return 1;
    }
    for (int i = 0; i < pairs.size(); ++i) {
        if (thread->inst.markets.find(pairs[i]) == thread->inst.markets.end()) {
            base::Log::log(LOG_LEVEL_WARNING, std::string("hitbtc: bad sub market id ") + pairs[i]);
            return 1;
        }
        if (this->markets4session[s].find(pairs[i]) != this->markets4session[s].end())
            continue;
        std::string json = subscribe_json(thread->inst.markets[pairs[i]].id);
        if (s->writeASIO(json)) {
            base::Log::log(LOG_LEVEL_WARNING, std::string("subscribe failure: ") + pairs[i]);
            return 1;
        }
        this->markets4session[s].insert(std::pair<std::string, bool>(pairs[i], false));
        ++this->thread->stat.wsSubscriptions;
    }
    return 0;
}

int Hitbtc::unsubscribe(WSsession *s, const std::vector<std::string> &pairs) {
    for (int i = 0; i < pairs.size(); ++i) {
        if (thread->inst.markets.find(pairs[i]) == thread->inst.markets.end()) {
            base::Log::log(LOG_LEVEL_WARNING, std::string("hitbtc: bad sub market id ") + pairs[i]);
            return 1;
        }
        if (this->markets4session[s].find(pairs[i]) == this->markets4session[s].end())
            continue;
        std::string json = unsubscribe_json(thread->inst.markets[pairs[i]].id);
        if (s->writeASIO(json)) {
            base::Log::log(LOG_LEVEL_WARNING, std::string("subscribe failure: ") + pairs[i]);
            return 1;
        }
        this->markets4session[s].erase(pairs[i]);
        this->thread->stat.wsSubscriptions--;
    }
    return 0;
}

ccxt::ParseResult Hitbtc::parseOrderbook(const rapidjson::Document &doc, const std::string &market, WSsession *ws) {
    ccxt::ParseResult parseResult;
    ccxt::Depth d;
    if (!doc.HasMember("jsonrpc") || !doc["jsonrpc"].IsString() || doc["jsonrpc"] != "2.0"
        || !doc.HasMember("params") || !doc["params"].IsObject()) {
        parseResult.success = false;
        parseResult.error = "bad JSON";
        return parseResult;
    }
    auto &params = doc["params"];
    if (!params.HasMember("ask") || !params["ask"].IsArray() || !params.HasMember("bid") || !params["bid"].IsArray()) {
        parseResult.success = false;
        parseResult.error = "bad JSON";
        return parseResult;
    }
    d.askCount = d.bidCount = 0;
    for (auto it = params["ask"].Begin(); it != params["ask"].End(); ++it) {
        auto &item = *it;
        d.asks[d.askCount] = {std::stod(item["price"].GetString()), std::stod(item["size"].GetString())};
        d.askCount++;
        if (d.askCount >= MAXDEPTH) break;
    }
    for (auto it = params["bid"].Begin(); it != params["bid"].End(); ++it) {
        auto &item = *it;
        d.bids[d.bidCount] = {std::stod(item["price"].GetString()), std::stod(item["size"].GetString())};
        d.bidCount++;
        if (d.bidCount >= MAXDEPTH) break;
    }
    auto mkt = this->thread->inst.marketsById[market].symbol;
    parseResult.updates.sequence = d.sequence = params["sequence"].GetInt64();
    parseResult.success = true;
    inet_aton(ws->serverIp.c_str(), (struct in_addr *) &d.ip);
    inet_aton(ws->localIp.c_str(), (struct in_addr *) &d.clientIp);

    std::map<std::string, ccxt::Depth> dm{{mkt, d}};
    parseResult.depth = dm;
    parseResult.type = ccxt::ParseResult::FullBook;
    return parseResult;
}

ccxt::ParseResult Hitbtc::parseUpdate(const rapidjson::Document &doc, const std::string &market, WSsession *ws) {
    ccxt::ParseResult parseResult;
    if (!doc.HasMember("jsonrpc") || !doc["jsonrpc"].IsString() || doc["jsonrpc"] != "2.0"
        || !doc.HasMember("params") || !doc["params"].IsObject()) {
        parseResult.success = false;
        parseResult.error = "bad JSON";
        return parseResult;
    }
    auto &params = doc["params"];
    if (!params.HasMember("ask") || !params["ask"].IsArray() || !params.HasMember("bid") || !params["bid"].IsArray()) {
        parseResult.success = false;
        parseResult.error = "update JSON parse error";
        return parseResult;
    }
    ccxt::DepthRow depthRow;
    auto mkt = this->thread->inst.marketsById[market].symbol;
    parseResult.updates.sequence = params["sequence"].GetInt64();

    unsigned long ip = 0;
    unsigned long clientIp = 0;
    inet_aton(ws->serverIp.c_str(), (struct in_addr *) &ip);
    inet_aton(ws->localIp.c_str(), (struct in_addr *) &clientIp);
    parseResult.updates.data[mkt].clear();
    if (params.HasMember("ask"))
        for (auto it = params["ask"].Begin(); it != params["ask"].End(); ++it) {
            auto &item = *it;
            depthRow.price = std::stod(item["price"].GetString());
            depthRow.amount = std::stod(item["size"].GetString());
            depthRow.getResponseTs = base::microtime();
            depthRow.buy = false;
            depthRow.ip = ip;
            depthRow.clientIp = clientIp;
            parseResult.updates.data[mkt].push_back(depthRow);
        }
    if (params.HasMember("bid"))
        for (auto it = params["bid"].Begin(); it != params["bid"].End(); ++it) {
            auto &item = *it;
            depthRow.price = std::stod(item["price"].GetString());
            depthRow.amount = std::stod(item["size"].GetString());
            depthRow.getResponseTs = base::microtime();
            depthRow.buy = true;
            depthRow.ip = ip;
            depthRow.clientIp = clientIp;
            parseResult.updates.data[mkt].push_back(depthRow);
        }
    //if (!parseResult.updates.data.size()) {
        //base::Log::log(LOG_LEVEL_WARNING, std::string("empty json ") + base::GetJsonText(doc));
    //}
    parseResult.success = true;
    parseResult.type = ccxt::ParseResult::Update;

    return parseResult;
}

ccxt::ParseResult Hitbtc::parse(const std::string &data, WSsession *wss) {
    rapidjson::Document doc;
    doc.Parse(data.c_str());
    ccxt::ParseResult parseResult;
    if (!doc.IsObject()) {
        parseResult.success = false;
        parseResult.error = "empty JSON";
        return parseResult;
    }
    if (doc.HasMember("result")) {
        parseResult.type = ccxt::ParseResult::SubscribeResponse;
        parseResult.success = true;
        if (doc["result"] == false) {
            parseResult.success = false;
            parseResult.error = std::string("subscription failure: ") + data;
        }
        return parseResult;
    }
    if (!doc.HasMember("params") || !doc["params"].HasMember("symbol") || !doc.HasMember("method")) {
        parseResult.success = false;
        parseResult.error = std::string("probably bad JSON: ") + data;
        return parseResult;
    }
    auto sym = doc["params"]["symbol"].GetString();

    return (doc["method"] == "updateOrderbook")
        ? parseUpdate(doc, sym, wss)
        : parseOrderbook(doc, sym, wss);
}

} // end namespace wsccxt
