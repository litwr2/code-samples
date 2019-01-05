#include "okex.h"
#include "../exchange.h"
#include "../crypto.h"
#include "base/Log.h"
#include <map>
#include <rapidjson/document.h>
#include <string>
#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <mutex>
#include <sstream>
#include <streambuf>
#include <vector>
#include <unistd.h>

namespace ccxt {
std::map<std::string, std::string> Okex::describe() {
    return {
        {"id", "okex"}, {"name", "Okex"}, {"countries", "HK"}, {"version", "v1"}, //v2 is available too
    };
}

Okex::Okex() : Exchange() {
    this->hasMap["fetch_my_trades"] = false;
}

std::map<std::string, Market> Okex::fetch_markets() {
    auto parseDoc = [&](const std::string &str) {
        rapidjson::Document d;
        d.Parse(str.c_str());
        if (!(d.IsObject() && d.HasMember("data") && d["data"].IsArray())) return;
        for (auto pd = d["data"].Begin(); pd != d["data"].End(); ++pd) {
            auto &item = *pd;
            if (!item.IsObject() || !item.HasMember("baseCurrency")) continue;
            Market m;
            m.active = true;
            std::string s = item["symbol"].GetString();
            //str_replace(s, "_usd", "_usdt");
            auto parts = ccxt::explode(s, '_');
            if (parts.size() != 2) continue;
            m.base = str_toupper(parts[0]);
            //if (m.base == "TUSD" || m.base == "USD") continue;
            m.quote = str_toupper(parts[1]);
            //if (m.quote == "USD") m.quote = "USDT";
            m.id = s;
            /*if (m.base == "USDT") {
                std::string exch = m.quote;
                m.quote = m.base;
                m.base = exch;
                m.id = str_tolower(m.base) + "_" + str_tolower(m.quote);
            }
            if (m.base == m.quote) continue;*/
            m.symbol = m.base + "/" + m.quote;
            if (item.HasMember("online") && item["online"].IsInt() && item["online"].GetInt() != 1) continue;

            m.precisionPrice = item["maxPriceDigit"].GetUint();
            m.precisionAmount = item["maxSizeDigit"].GetUint();
            m.limitAmountMin = item["minTradeSize"].GetDouble();
            if (m.precisionAmount > 0)
                m.lot = stod(std::string("0.") + std::string(m.precisionAmount - 1, '0') + "1");
            else
                m.lot = stod(std::string("1") + std::string(-m.precisionAmount, '0'));
            if (m.precisionPrice > 0)
                m.limitPriceMin = stod(std::string("0.") + std::string(m.precisionPrice - 1, '0') + "1");
            else
                m.limitPriceMin = stod(std::string("1") + std::string(-m.precisionPrice, '0'));
            this->markets[m.symbol] = m;
            this->marketsById[m.id] = m;
        }
    };
    if (!this->markets.size()) {
        mutex.lock();
        std::string infoUrl = "https://www.okex.com/v2/spot/markets/products";
        auto response = this->fetch(infoUrl, "GET", {});
        parseDoc(response.second);
        mutex.unlock();
    }
    return this->markets;
}

std::string Okex::getScanUrl(std::vector<std::string> pairs, int limit) {
    if (!limit) limit = 20;
    if (pairs.size() == 1) {
        std::string id = pairs[0];
        id = markets[id].id;
        return this->apiUrl + "depth.do?symbol=" + id + "&size=" + std::to_string(limit);
    }
    throw "Okex: multiple pairs not supported";
}

std::map<std::string, Depth> Okex::parseScanResponse(std::string r, std::string url) {
    std::map<std::string, Depth> ret;
    rapidjson::Document doc;
    doc.Parse(r.c_str());
    if (!doc.IsObject()) return ret;
    if (!doc.HasMember("asks") || !doc.HasMember("bids"))
        return ret;
    auto parts = explode(url, '=');
    if (parts.size() < 2)
        return ret;
    auto parts2 = explode(parts[1], '&');
    if (parts2.size() < 2)
        return ret;
    std::string symbol = parts2[0];
    auto mkt = this->marketsById.find(symbol);
    if (mkt == this->marketsById.end()) return ret;

    ccxt::Depth d;
    d.askCount = d.bidCount = 0;

    for (auto it = doc["bids"].Begin(); it != doc["bids"].End(); ++it) {
        auto &item = *it;
        auto price = item[0].GetDouble();
        auto amount = item[1].GetDouble();
        d.bids[d.bidCount] = { price, amount };
        if (++d.bidCount >= MAXDEPTH) break;
    }
    std::map<double, double> asks;
    for (auto it = doc["asks"].Begin(); it != doc["asks"].End(); ++it) {
        auto &item = *it;
        auto price = item[0].GetDouble();
        auto amount = item[1].GetDouble();
        asks[price] = amount;
    }
    for (auto it = asks.begin(); it != asks.end(); ++it) {
        d.asks[d.askCount] = { it->first, it->second };
        if (++d.askCount >= MAXDEPTH) break;
    }
    ret[mkt->second.symbol] = d;

    return ret;
}

std::map<std::string, CurrencyBalance> Okex::parse_fetch_balance(std::string rs) const {
    std::map<std::string, CurrencyBalance> ret;
    rapidjson::Document d;
    d.Parse(rs.c_str());
    if (!(d.IsObject() && d.HasMember("info") && d["info"].IsObject())) return ret;
    for (auto it = d["info"]["funds"]["free"].MemberBegin(); it != d["info"]["funds"]["free"].MemberEnd(); ++it) {
        std::string key = str_toupper(it->name.GetString());
        double val = std::stod(it->value.GetString());
        CurrencyBalance cb;
        cb.free = val;
        ret[key] = cb;
    }
    for (auto it = d["info"]["funds"]["freezed"].MemberBegin(); it != d["info"]["funds"]["freezed"].MemberEnd(); ++it) {
        std::string key = str_toupper(it->name.GetString());
        double val = std::stod(it->value.GetString());
        CurrencyBalance cb = ret[key];
        cb.used = val;
        cb.total = cb.free + cb.used;
        ret[key] = cb;
    }
    return ret;
}

OrderInfo Okex::parse_order_response(std::string rs) const {
    throw "not implemented";
}

std::pair<int, OrderInfo> Okex::parse_create_order(std::string rs) const {
   rapidjson::Document d;
   d.Parse(rs.c_str());
    std::pair<int, OrderInfo> ret;
    ret.first = 1;
    if (!(d.IsObject() && d.HasMember("result") && d["result"].IsBool()
        && d["result"].GetBool() && d.HasMember("order_id"))) return ret;
    ret.first = 0;
    ret.second.id = std::to_string(d["order_id"].GetInt64());
    return ret;
}

std::map<std::string, std::string> Okex::get_signature_headers(std::string path,
                                                                  std::map<std::string, std::string> query) {
    query["api_key"] = this->bot.key_pub;
    this->payload = http_build_query(query);
    std::map<std::string, std::string> headers;
    std::string sign = str_toupper(getMD5(this->payload + "&secret_key=" + this->bot.key_sec));
    this->payload += "&sign=" + sign;
    headers["Content-Type"] = "application/x-www-form-urlencoded";
    return headers;
}

CurlHandle *Okex::fetch_balance(std::function<void(int, std::map<std::string, CurrencyBalance>)> func) {
    std::string method = "userinfo.do";
    std::map<std::string, std::string> query = {};
    std::string url = this->apiUrl + method;
    auto headers = this->get_signature_headers(url, query);
    auto response = this->fetch(url, "POST", headers, this->payload);
    this->lastError = response.first;
    this->lastResponse = response.second;
    if (this->lastError != 200) {
        this->lastErrorStr = this->lastResponse;
    } else {
        this->lastErrorStr = "";
    }
    std::map<std::string, CurrencyBalance> bal = this->parse_fetch_balance(response.second);

    func(bal.size() > 0 ? 0 : 1, bal);
    return NULL;
}

CurlHandle *Okex::fetch_my_trades(std::string symbol, std::string since, int limit,
                            std::function<void(int, std::vector<OrderInfo>)> func) {
    throw "not implemented";
}

CurlHandle *Okex::fetch_open_orders(std::string symbol, std::string since, int limit,
                              std::function<void(int, std::vector<OrderInfo>)> func) {
    std::string httpMethod = "POST";

    std::string method = "order_info.do";
    auto mkt = this->markets.find(symbol);
    if (mkt == this->markets.end()) {
        throw "Okex::fetch_open_orders: unsupported market";
    }
    std::string pair = mkt->second.id;

    std::map<std::string, std::string> query = {
            {"symbol", pair},
            {"order_id",     "-1"},
            {"status",       "1"},
            {"current_page", "0"},
            {"page_length",  "50"},
    };

    std::string url = this->apiUrl + method;
    auto headers = this->get_signature_headers(url, query);
    auto response = this->fetch(url, "POST", headers, this->payload);
    lastError = response.first;
    this->lastResponse = response.second;
    if (lastError != 200) {
        this->lastErrorStr = this->lastResponse;
    } else {
        this->lastErrorStr = "";
    }

    auto parsed = this->parse_fetch_orders(response.second, "open");
    func(parsed.first, parsed.second);
    return NULL;
}

std::pair<int, std::vector<OrderInfo>> Okex::parse_fetch_orders(std::string rs, std::string status) const {
    throw "Okex::parse_fetch_orders not implemented";
}

std::pair<int, std::vector<OrderInfo>> Okex::parse_fetch_my_trades(std::string rs, std::string status) const {
    throw "not implemented";
}

OrderInfo Okex::parse_order(rapidjson::Value &rs) const {
    throw "not implemented";
}

int Okex::parse_cancel_order(std::string rs) const {
    rapidjson::Document d;
    d.Parse(rs.c_str());
    return (d.IsObject() && d.HasMember("result") && d["result"].IsBool()
        && d["result"].GetBool()) ? 0 : 1;
}

std::pair<int, OrderInfo> Okex::parse_fetch_order(std::string rs) const {
    rapidjson::Document d;
    d.Parse(rs.c_str());
    std::pair<int, OrderInfo> ret;
    ret.first = 1;
    if (!(d.IsObject() && d.HasMember("result") && d["result"].IsBool()
          && d["result"].GetBool() && d.HasMember("orders") && d["orders"].IsArray()
          && d["orders"].Size())) return ret;
    ret.first = 0;
    auto &item = d["orders"][0];
    ret.second.id = std::to_string(item["order_id"].GetInt64());
    ret.second.price = item["price"].GetDouble();
    ret.second.filled = item["deal_amount"].GetDouble();
    ret.second.status = "open";
    int status = item["status"].GetInt();
    // order canceled
    if (status == -1 || status == 3)
        ret.first = 1;
    if (status == 2)
        ret.second.status = "closed";
    return ret;
}

CurlHandle *Okex::fetch_order(std::string id, std::string symbol, std::map<std::string, std::string> params,
                        std::function<void(int, OrderInfo)> func) {
    std::string method = "order_info.do";
    auto mkt = this->markets.find(symbol);
    if (mkt == this->markets.end()) {
        throw "Okex::fetch_order: unsupported market";
    }
    std::string pair = mkt->second.id;
    std::map<std::string, std::string> query = {{"symbol", pair},
                                                {"order_id", id}};
    std::string url = this->apiUrl + method;
    auto headers = this->get_signature_headers(url, query);
    auto response = this->fetch(url, "POST", headers, this->payload);
    this->lastResponse = response.second;
    auto parsed = this->parse_fetch_order(response.second);
    parsed.second.symbol = symbol;
    if (!parsed.first) {
        if (this->orderAmountsBySymbol.find(symbol) != this->orderAmountsBySymbol.end()) {
            if (this->orderAmountsBySymbol[symbol].find(parsed.second.id) != this->orderAmountsBySymbol[symbol].end()) {
                parsed.second.amount = this->orderAmountsBySymbol[symbol][parsed.second.id];
                parsed.second.remaining = parsed.second.amount - parsed.second.filled;
            }
            if (parsed.second.amount == parsed.second.filled)
                parsed.second.status = "closed";
        }
        if (this->orderTypesBySymbol.find(symbol) != this->orderTypesBySymbol.end()) {
            if (this->orderTypesBySymbol[symbol].find(parsed.second.id) != this->orderTypesBySymbol[symbol].end())
                parsed.second.side = this->orderTypesBySymbol[symbol][parsed.second.id];
        }
    }

    func(parsed.first, parsed.second);
    return NULL;
}

CurlHandle *Okex::create_order(std::string symbol, std::string type, std::string side, double amount, double price,
                         std::map<std::string, std::string> params,
                         std::function<void(int, OrderInfo)> func) {
    if (side != "buy")
        side = "sell";
    std::string method = "trade.do";
    auto mkt = this->markets.find(symbol);
    if (mkt == this->markets.end()) {
        throw "Okex::create_order: unsupported market";
    }
    std::string pair = mkt->second.id;
    std::map<std::string, std::string> query = {{"type", side},
                                                {"symbol", pair},
                                                {"price", to_string_with_precision(price, mkt->second.precisionPrice)},
                                                {"amount", to_string_with_precision(amount, mkt->second.precisionAmount)}};
    std::string url = this->apiUrl + method;
    auto headers = this->get_signature_headers(url, query);
    auto response = this->fetch(url, "POST", headers, this->payload);
    lastError = response.first;
    this->lastResponse = response.second;
    if (lastError != 200) {
        this->lastErrorStr = this->lastResponse;
    } else {
        this->lastErrorStr = "";
    }
    auto rr = this->parse_create_order(response.second);
    if (rr.second.id.length()) {
        this->orderAmountsBySymbol[symbol][rr.second.id] = amount;
        this->orderTypesBySymbol[symbol][rr.second.id] = side;
        rr.second.side = side;
        rr.second.symbol = symbol;
        rr.second.price = price;
        rr.second.amount = amount;
        rr.second.cost = rr.second.price * rr.second.filled;
        rr.second.status = "open";
        lastErrorStr = "";
    } else {
        if (response.second.find("Cloudflare to restrict access") != std::string::npos)
            lastErrorStr = "Cloudflare to restrict access";
        else
            lastErrorStr = response.second;
    }

    func(rr.first, rr.second);
    return NULL;
}

CurlHandle *Okex::cancel_order_with_retries(std::string id, std::string symbol, std::map<std::string, std::string> params, int retries, double timeout, std::function<void(int, int)> func) {
    std::string method = "cancel_order.do";

    std::string url = this->apiUrl + method;
    int origRetries = retries;
    auto mkt = this->markets.find(symbol);
    if (mkt == this->markets.end()) throw "Okex::cancel_order_with_retries: unsupported market";
    do {
        std::map<std::string, std::string> query = {{"symbol", mkt->second.id},
                                                    {"order_id", id}};

        auto headers = this->get_signature_headers(url, query);
        auto response = this->fetch(url, "POST", headers, this->payload);
        this->lastResponse = response.second;
        this->lastError = response.first;
        int success = this->parse_cancel_order(response.second);
        if (!success) {
            this->lastErrorStr = "";
            func(success, origRetries - retries + 1);
            return NULL;
        }
        this->lastErrorStr = this->lastResponse;
        retries--;
        usleep((int) (timeout * 1000.0));
    } while (retries > 0);

    func(1, origRetries);
    return NULL;
}

CurlHandle *Okex::async_fetch_balance() {
    std::string method = "userinfo.do";
    std::map<std::string, std::string> query = {};
    std::string url = this->apiUrl + method;
    auto headers = this->get_signature_headers(url, query);
    auto ret = this->prepare(url, "POST", headers, this->payload, true);
    ret->type = HANDLER_TYPE_BALANCE;
    return ret;
}

CurlHandle *Okex::async_fetch_open_orders(std::string symbol, std::string since, int limit) {
    std::string httpMethod = "POST";

    std::string method = "order_info.do";
    auto mkt = this->markets.find(symbol);
    if (mkt == this->markets.end()) {
        throw "Okex::fetch_open_orders: unsupported market";
    }
    std::string pair = mkt->second.id;

    std::map<std::string, std::string> query = {
            {"symbol", pair},
            {"order_id",     "-1"},
            {"status",       "1"},
            {"current_page", "0"},
            {"page_length",  "50"},
    };

    std::string url = this->apiUrl + method;
    auto headers = this->get_signature_headers(url, query);
    auto ret = this->prepare(url, "POST", headers, this->payload, true);
    ret->type = HANDLER_TYPE_ORDERS;
    return ret;
}

CurlHandle *Okex::async_fetch_my_trades(std::string symbol, std::string since, int limit) {
    throw "not implemented";
}

CurlHandle *Okex::async_fetch_order(std::string id, std::string symbol, std::map<std::string, std::string> params) {
    std::string method = "order_info.do";
    auto mkt = this->markets.find(symbol);
    if (mkt == this->markets.end()) {
        throw "Okex::async_fetch_order: unsupported market";
    }
    std::string pair = mkt->second.id;
    std::map<std::string, std::string> query = {{"symbol", pair},
                                                {"order_id", id}};
    std::string url = this->apiUrl + method;
    auto headers = this->get_signature_headers(url, query);
    auto ret = this->prepare(url, "POST", headers, this->payload, true);
    ret->type = HANDLER_TYPE_ORDER;
    return ret;
}

CurlHandle *Okex::async_create_order(std::string symbol, std::string type, std::string side, double amount,
                         double price, std::map<std::string, std::string> params) {
    if (side != "buy")
        side = "sell";
    std::string method = "trade.do";
    auto mkt = this->markets.find(symbol);
    if (mkt == this->markets.end()) {
        throw "Okex::create_order: unsupported market";
    }
    std::string pair = mkt->second.id;
    std::map<std::string, std::string> query = {{"type", side},
                                                {"symbol", pair},
                                                {"price", to_string_with_precision(price, mkt->second.precisionPrice)},
                                                {"amount", to_string_with_precision(amount, mkt->second.precisionAmount)}};
    std::string url = this->apiUrl + method;
    auto headers = this->get_signature_headers(url, query);
    auto ret = this->prepare(url, "POST", headers, this->payload, true);

    ret->paramOrderCreate = new ParamOrderCreate();
    ret->paramOrderCreate->symbol = symbol;
    ret->paramOrderCreate->type = type;
    ret->paramOrderCreate->side = side;
    ret->paramOrderCreate->amount = amount;
    ret->paramOrderCreate->price = price;
    ret->type = HANDLER_TYPE_CREATE_ORDER;
    return ret;
}

CurlHandle *Okex::async_cancel_order(std::string id, std::string symbol, std::map<std::string, std::string> params) {
    std::string method = "cancel_order.do";

    std::string url = this->apiUrl + method;
    auto mkt = this->markets.find(symbol);
    if (mkt == this->markets.end()) throw "Okex::cancel_order_with_retries: unsupported market";
    std::map<std::string, std::string> query = {{"symbol", mkt->second.id},
                                                {"order_id", id}};

    auto headers = this->get_signature_headers(url, query);
    auto ret = this->prepare(url, "POST", headers, this->payload, true);
    ret->type = HANDLER_TYPE_CANCEL_ORDER;
    return ret;
}

}
