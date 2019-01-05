#include <base/monitor/MonitorInstance.h>
#include <base/base64.h>
#include "ws-bittrex.h"
#include "../unzip-noheader.h"

namespace wsccxt {

Bittrex::Bittrex(base::DepthMap &map) : WebSocketBase(map) {
    load_root_certificates(ctx);
    this->hasSeqNo = true;
}

WSsession* Bittrex::connect(std::string url, const std::map<std::string, std::string> &options) {
    WSsession *s = new WSsession(ctx, std::bind(&Bittrex::storeEvent, this,
            std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    std::string localIp = "";
    if (options.find("proxy") != options.end() && options.at("proxy").find("if:") != std::string::npos) {
        localIp = (options.at("proxy").c_str() + 3);
    }
    base::Log::log(LOG_LEVEL_WARNING, std::string("Connect: ") + localIp);
    std::string json_connectionToken = "Zdc%2FRILqbYJ4rM0pJoAaUpVs%2FAbZLKP1WzzNq9g%2BqvTHqG57PMlfiQLzHNdkhqbzM0QxtYNxs8SiCl7EjV5xOobWauy8Nf2xwu4CX1Lly%2ByH68u6";
    std::string infoUrl = "https://socket.bittrex.com/signalr/negotiate?connectionData=%5B%7B%22name%22%3A%22c2%22%7D%5D&clientProtocol=1.5";
    auto response = wsBittrex.fetch(infoUrl, "GET", {});
    rapidjson::Document doc;
    doc.Parse(response.second.c_str());
    if (!doc.HasParseError() && doc.HasMember("ConnectionToken") && doc["ConnectionToken"].IsString())
        json_connectionToken = ccxt::urlencode(doc["ConnectionToken"].GetString());
    auto adjustedDir = dir + "1.5&connectionToken=" + json_connectionToken + "&connectionData=%5B%7B%22name%22%3A%22c2%22%7D%5D&tid=3";
    if (!s->connect(this, host, port, adjustedDir, localIp)) return 0;
    sessionList.push_back(s);
    return s;
}

int Bittrex::subscribe(WSsession *s, const std::vector<std::string> &pairs) {
    if (pairs.empty()) {
        base::Log::log(LOG_LEVEL_WARNING, std::string("bittrex: empty market pairs for subscribe"));
        return 1;
    }
    for (int i = 0; i < pairs.size(); ++i) {
        if (thread->inst.markets.find(pairs[i]) == thread->inst.markets.end()) {
            base::Log::log(LOG_LEVEL_WARNING, std::string("bittrex: bad sub market id ") + pairs[i]);
            return 1;
        }
        if (this->markets4session[s].find(pairs[i]) != this->markets4session[s].end())
            continue;
        std::string json = subscribe_json2(thread->inst.markets[pairs[i]].id);
        if (s->writeASIO(json)) {
            base::Log::log(LOG_LEVEL_WARNING, std::string("subscribe1 failure: ") + pairs[i]);
            return 1;
        }
        json = subscribe_json1(thread->inst.markets[pairs[i]].id);
        if (s->writeASIO(json)) {
            base::Log::log(LOG_LEVEL_WARNING, std::string("subscribe2 failure: ") + pairs[i]);
            return 1;
        }
        this->markets4session[s].insert(std::pair<std::string, bool>(pairs[i], false));
        ++this->thread->stat.wsSubscriptions;
    }
    return 0;
}

int Bittrex::unsubscribe(WSsession *s, const std::vector<std::string> &pairs) {
    return 0;
}

ccxt::ParseResult Bittrex::parseOrderbook(const std::string &data, WSsession *ws) {
    ccxt::ParseResult parseResult;
    ccxt::Depth d;
    rapidjson::Document doc;
    doc.Parse(data.c_str());
    if (!doc.HasMember("M") || !doc["M"].IsString() || !doc.HasMember("N") || !doc["N"].IsUint()
        || !doc.HasMember("Z") || !doc["Z"].IsArray() || !doc.HasMember("S") || !doc["S"].IsArray()) {
        parseResult.success = false;
        parseResult.error = "bad JSON";
        return parseResult;
    }
    auto market = doc["M"].GetString();
    auto mkt = this->thread->inst.marketsById[market].symbol;
    parseResult.updates.sequence = ws->sequence[mkt] = d.sequence = doc["N"].GetInt64();
    d.askCount = d.bidCount = 0;
    for (auto it = doc["S"].Begin(); it != doc["S"].End(); ++it) {
        auto &item = *it;
        d.asks[d.askCount] = {item["R"].GetDouble(), item["Q"].GetDouble()};
        d.askCount++;
        if (d.askCount >= MAXDEPTH) break;
    }
    for (auto it = doc["Z"].Begin(); it != doc["Z"].End(); ++it) {
        auto &item = *it;
        d.bids[d.bidCount] = {item["R"].GetDouble(), item["Q"].GetDouble()};
        d.bidCount++;
        if (d.bidCount >= MAXDEPTH) break;
    }
    parseResult.success = true;
    std::map<std::string, ccxt::Depth> dm{{mkt, d}};
    parseResult.depth = dm;
    parseResult.type = ccxt::ParseResult::FullBook;
    return parseResult;
}

ccxt::ParseResult Bittrex::parseUpdate(const std::string &data, WSsession *ws) {
    ccxt::ParseResult parseResult;
    rapidjson::Document doc;
    doc.Parse(data.c_str());
    if (!doc.HasMember("M") || !doc["M"].IsString() || !doc.HasMember("N") || !doc["N"].IsUint()
        || !doc.HasMember("Z") || !doc["Z"].IsArray() || !doc.HasMember("S") || !doc["S"].IsArray()) {

        parseResult.success = false;
        parseResult.error = "bad JSON";
        return parseResult;
    }
    auto market = doc["M"].GetString();
    auto mkt = this->thread->inst.marketsById[market].symbol;
    ccxt::DepthRow depthRow;
    if (ws->sequence.find(mkt) == ws->sequence.end()) {
        parseResult.success = false;
        parseResult.error = "update is premature";
        return parseResult;
    }
    parseResult.updates.sequence = doc["N"].GetInt64();
    parseResult.updates.data[mkt].clear();
    for (auto it = doc["S"].Begin(); it != doc["S"].End(); ++it) {
        auto &item = *it;
        depthRow.price = item["R"].GetDouble();
        depthRow.amount = item["Q"].GetDouble();
        depthRow.getResponseTs = base::microtime();
        depthRow.buy = false;
        parseResult.updates.data[mkt].push_back(depthRow);
    }
    for (auto it = doc["Z"].Begin(); it != doc["Z"].End(); ++it) {
        auto &item = *it;
        depthRow.price = item["R"].GetDouble();
        depthRow.amount = item["Q"].GetDouble();
        depthRow.getResponseTs = base::microtime();
        depthRow.buy = true;
        parseResult.updates.data[mkt].push_back(depthRow);
    }
    parseResult.success = true;
    parseResult.type = ccxt::ParseResult::Update;
    return parseResult;
}

ccxt::ParseResult Bittrex::parse(const std::string &data, WSsession *wss) {
    rapidjson::Document doc;
    doc.Parse(data.c_str());
    ccxt::ParseResult parseResult;
    parseResult.error = "empty JSON";
    if (!doc.IsObject()) {
       parseResult.success = false;
       return parseResult;
    }
    if (doc.HasMember("S") || doc.MemberCount() == 0) { // e.g., {} or {"C":"d-CF9C849-B,0|CjGT,0|CjGU,1","S":1,"M":[]}
        parseResult.type = ccxt::ParseResult::Ping;
        parseResult.success = true;
        return parseResult;
    }
    if (doc.HasMember("R") && doc["R"].IsBool() && doc["R"].GetBool()) {  //acknowledge #2 for updates, e.g., {"R":true,"I":"1"}
        parseResult.type = ccxt::ParseResult::SubscribeResponse;
        parseResult.success = true;
        return parseResult;
    }
    if (doc.HasMember("G") && doc["G"].IsString()) {  //acknowledge #1 for updates, e.g., {"C":"d-CF9C849-B,0|CjGT,0|CjGU,2|nI,66","G":"3imj ... 2BA==","M":[]}
        parseResult.type = ccxt::ParseResult::SubscribeResponse;
        parseResult.success = true;
        return parseResult;
    }
    if (doc.HasMember("R") && doc["R"].IsString()) {  //e.g., {"R":"vV ... ", "I":"2"}"
        auto encodedData = doc["R"].GetString();
        auto data = std::move(ZipNoHeader::decompress(base64_decode(encodedData)));
//std::cout << "An orderbook: " << data.substr(0, 70) << "\n";
        return parseOrderbook(data, wss);
    }
    if (doc.HasMember("M") && doc["M"].IsArray() && doc["M"][0].IsObject() && doc["M"][0].HasMember("A") 
        && doc["M"][0]["A"].IsArray() && doc["M"][0]["A"][0].IsString()) {  //e.g., {"C":"d-81E7886-C,0|H0Z1,0|H0Z2,2|ER,13F4F","M":[{"H":"C2","M":"uE","A":["dZA5D ... eMN"]}]}

        auto encodedData = doc["M"][0]["A"][0].GetString();
        auto data = std::move(ZipNoHeader::decompress(base64_decode(encodedData)));
//std::cout << "An update: " << data << "\n";
        return parseUpdate(data, wss);
    }
    base::Log::log(LOG_LEVEL_WARNING, std::string("Unknown json type: ") + data);
    parseResult.success = false;
    parseResult.error = "unknown JSON type";
    return parseResult;

}

} // end namespace wsccxt
