#include <base/monitor/MonitorInstance.h>
#include "ws-cointiger.h"
#include "../gzip-string.h"

namespace wsccxt {

Cointiger::Cointiger(base::DepthMap &map) : WebSocketBase(map) {
    load_root_certificates(ctx);
}

WSsession* Cointiger::connect(std::string url, const std::map<std::string, std::string> &options) {
    WSsession *s = new WSsession(ctx, std::bind(&Cointiger::storeEvent, this,
            std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    std::string localIp = "";
    if (options.find("proxy") != options.end() && options.at("proxy").find("if:") != std::string::npos) {
        localIp = (options.at("proxy").c_str() + 3);
    }
    base::Log::log(LOG_LEVEL_WARNING, std::string("Connect: ") + localIp);
    if (!s->connect(this, host, port, dir, localIp)) return 0;
    sessionList.push_back(s);
    return s;
}

int Cointiger::subscribe(WSsession *s, const std::vector<std::string> &pairList) {
    if (pairList.empty()) {
        base::Log::log(LOG_LEVEL_WARNING, std::string("cointiger: empty market pairs"));
        return 1;
    }
    for (auto it = pairList.begin(); it != pairList.end(); ++it) {
        std::string pair = *it;
        if (this->markets4session[s].find(pair) != this->markets4session[s].end())
            continue;
        if (!thread->inst.markets[pair].id.length()) {
            base::Log::log(LOG_LEVEL_WARNING, std::string("cointiger: bad sub market id ") + pair);
            return 1;
        }
        auto json = subscribe_json(thread->inst.markets[pair].id);
        if (s->writeASIO(json)) {
            base::Log::log(LOG_LEVEL_WARNING, std::string("subscribe failure: ") + pair);
            return 1;
        }
        this->markets4session[s].insert(std::pair<std::string, bool>(pair, false));
        ++this->thread->stat.wsSubscriptions;
    }
    return 0;
}

int Cointiger::unsubscribe(WSsession *s, const std::vector<std::string> &pairList) {
    if (!pairList.size()) {
        base::Log::log(LOG_LEVEL_WARNING, std::string("cointiger: empty market pairs"));
        return 1;
    }
    for (auto it = pairList.begin(); it != pairList.end(); ++it) {
        std::string pair = *it;
        if (this->markets4session[s].find(pair) == this->markets4session[s].end())
            continue;
        if (!thread->inst.markets[pair].id.length()) {
            base::Log::log(LOG_LEVEL_WARNING, std::string("cointiger: bad sub market id ") + pair);
            return 1;
        }
        auto json = unsubscribe_json(thread->inst.markets[pair].id);
        if (s->writeASIO(json)) {
            base::Log::log(LOG_LEVEL_WARNING, std::string("unsubscribe failure: ") + pair);
            return 1;
        }
        this->markets4session[s].erase(pair);
        --this->thread->stat.wsSubscriptions;
    }
    return 0;
}

ccxt::ParseResult Cointiger::parseOrderbook(const rapidjson::Document &doc, const std::string &market, WSsession *ws) {
    ccxt::ParseResult parseResult;
    ccxt::Depth d;
    auto &params = doc["tick"];
    if (!params.HasMember("asks") || !params["asks"].IsArray() || !params.HasMember("buys") || !params["buys"].IsArray()) {
        parseResult.success = false;
        parseResult.error = "bad JSON";
        return parseResult;
    }
    d.askCount = d.bidCount = 0;
    for (auto it = params["asks"].Begin(); it != params["asks"].End(); ++it) {
        auto &item = *it;
        d.asks[d.askCount] = {std::stod(item[0].GetString()), item[1].GetDouble()};
        d.askCount++;
        if (d.askCount >= MAXDEPTH) break;
    }
    for (auto it = params["buys"].Begin(); it != params["buys"].End(); ++it) {
        auto &item = *it;
        d.bids[d.bidCount] = {std::stod(item[0].GetString()), item[1].GetDouble()};
        d.bidCount++;
        if (d.bidCount >= MAXDEPTH) break;
    }

    parseResult.success = true;
    auto mkt = this->thread->inst.marketsById[ccxt::str_tolower(market)].symbol;
    std::map<std::string, ccxt::Depth> dm{{mkt, d}};
    parseResult.depth = dm;
    parseResult.type = ccxt::ParseResult::FullBook;
    return parseResult;
}

ccxt::ParseResult Cointiger::parseUpdate(const rapidjson::Document &doc, const std::string &market, WSsession *ws) {
    ccxt::ParseResult parseResult;
    ccxt::Depth d;
    throw std::runtime_error("can't be called");
    /*auto &params = doc["params"][1];
    if ((!params.HasMember("asks") || !params["asks"].IsArray()) && (!params.HasMember("bids") || !params["bids"].IsArray())) {
        parseResult.success = false;
        parseResult.error = "bad JSON";
        return parseResult;
    }
    ccxt::DepthRow depthRow;
    auto mkt = this->thread->inst.marketsById[ccxt::str_tolower(market)].symbol;
    if (params.HasMember("asks"))
        for (auto it = params["asks"].Begin(); it != params["asks"].End(); ++it) {
            auto &item = *it;
            depthRow.price = std::stod(item[0].GetString());
            depthRow.amount = std::stod(item[1].GetString());
            depthRow.buy = false;
            depthRow.pair = mkt;
            parseResult.updates.push_back(depthRow);
        }
    if (params.HasMember("bids"))
        for (auto it = params["bids"].Begin(); it != params["bids"].End(); ++it) {
            auto &item = *it;
            depthRow.price = std::stod(item[0].GetString());
            depthRow.amount = std::stod(item[1].GetString());
            depthRow.buy = true;
            depthRow.pair = mkt;
            parseResult.updates.push_back(depthRow);
        }
    parseResult.success = true;*/
    parseResult.type = ccxt::ParseResult::Update;
    return parseResult;
}

ccxt::ParseResult Cointiger::parse(const std::string &data_gzipped, WSsession *wss) {
    ccxt::ParseResult parseResult;
    std::string data;
    try {
        data = Gzip::decompress(data_gzipped);
    } catch (const std::exception &s) {
        parseResult.success = false;
        parseResult.error = "wrong compression";
        return parseResult;
    }
    const char* p = strchr(data.c_str(), '{');
    /*if (!p && !strcmp(data.data(), "open")) {
        parseResult.success = true;
        parseResult.type = ccxt::ParseResult::SubscribeResponse;
        return parseResult;
    }
    if (!p || strcmp(data.data(), "msg")) {
        parseResult.success = false;
        parseResult.error = "wrong pre-JSON data";
        return parseResult;
    }*/
    rapidjson::Document doc;
    doc.Parse(p);

    if (!doc.IsObject()) {
        parseResult.success = false;
        parseResult.error = "empty JSON";
        return parseResult;
    }
    if (doc.HasMember("ping")) {  //it is not handled
        parseResult.type = ccxt::ParseResult::Ping;
        parseResult.success = true;
        return parseResult;
    }
    if (doc.HasMember("event_rep") && doc.HasMember("status")) {
        parseResult.type = ccxt::ParseResult::SubscribeResponse;
        parseResult.success = true;
        if (std::string(doc["event_rep"].GetString()) != "subed" || std::string(doc["status"].GetString()) != "ok") {
            parseResult.success = false;
            parseResult.error = std::string("rejected subscribe: ") + data;
        }
        return parseResult;
    }
    if (!doc.HasMember("tick") || !doc["tick"].IsObject()) {
        parseResult.success = false;
        parseResult.error = "probably bad JSON: " + data;
        return parseResult;
    }
    std::string sym = std::string(doc["channel"].GetString()).substr(7);
    sym = sym.substr(0, sym.find('_'));

    return parseOrderbook(doc, sym, wss);
}

} // end namespace wsccxt
