#include "okex-fut.h"
#include "../exchange.h"
#include "../crypto.h"
#include "base/Log.h"
#include <map>
#include <rapidjson/document.h>
#include "../DateHelper.h"
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
#include <ctime>
#include <strings.h>

namespace ccxt {
std::map<std::string, std::string> OkexFut::describe() {
    return {
        {"id", "okex-fut"}, {"name", "Okex Futures"}, {"countries", "HK"}, {"version", "v1"},
    };
}

OkexFut::OkexFut() : Okex() {
    this->hasMap["fetch_my_trades"] = false;
}

std::map<std::string, Market> OkexFut::fetch_markets() {
    auto parseDoc = [&](const std::string &str) {
        rapidjson::Document d;
        d.Parse(str.c_str());
        if (!(d.IsArray())) return;
        for (auto pd = d.Begin(); pd != d.End(); ++pd) {
            auto &item = *pd;
            if (!item.IsObject() || !item.HasMember("instrument_id")) continue;
            Market m;
            m.active = true;
            std::string sId = item["instrument_id"].GetString();
            std::string sBase = item["underlying_index"].GetString();
            std::string sQuote = item["quote_currency"].GetString();
            std::string sListing = item["listing"].GetString();
            std::string sDelivery = item["delivery"].GetString();
            std::string sQuoteIncr = "";
            if (item.HasMember("quote_increment"))
                sQuoteIncr = item["quote_increment"].GetString();
            else if (item.HasMember("tick_size"))
                sQuoteIncr = item["tick_size"].GetString();
            auto parts = ccxt::explode(sId, '-');
            if (parts.size() != 3) continue;
            m.base = parts[0] + parts[2].substr(2);
            m.quote = sQuote;
            m.id = sId;
            m.symbol = m.base + "/" + m.quote;
            m.contractType = getContractType(m.symbol);

            if (sQuoteIncr.length())
                m.precisionPrice = precisionFromString(sQuoteIncr);
            else
                m.precisionPrice = 4;
            m.precisionAmount = 4;
            // ??
            m.limitAmountMin = stod(std::string("0.") + std::string(m.precisionAmount - 1, '0') + "1");
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
            //BTC-USD-181026 -> BTC1102
            std::string contractName = m.id.substr(0, 3) + m.id.substr(10, 4);
            this->marketsByContractName[contractName] = m;

            std::string symbolPrefix = str_tolower(m.id).substr(0, 7);
            str_replace(symbolPrefix, "-", "_");
            //btc_usd-this_week
            std::string symbolContractType = symbolPrefix + "-" + m.contractType;
            this->marketsBySymbolContractType[symbolContractType] = m;
        }
    };
    if (!this->markets.size()) {
        mutex.lock();

        std::string infoUrl = this->marketsUrl;
        auto response = this->fetch(infoUrl, "GET", {});
        parseDoc(response.second);

        mutex.unlock();
    }
    return this->markets;
}

std::string OkexFut::getScanUrl(std::vector<std::string> pairs, int limit) {
    if (!limit) limit = 20;
    if (pairs.size() == 1) {
        std::string id = pairs[0];
        id = markets[id].id;
        return this->v3Url + "instruments/" + id + "/book?size=" + std::to_string(limit);
    }
    throw "OkexFut: multiple pairs not supported";
}

std::map<std::string, Depth> OkexFut::parseScanResponse(std::string r, std::string url) {
    std::map<std::string, Depth> ret;
    rapidjson::Document doc;
    doc.Parse(r.c_str());
    if (!doc.IsObject()) return ret;
    if (!doc.HasMember("asks") || !doc.HasMember("bids"))
        return ret;
    auto parts = explode(url, '/');
    if (parts.size() < 3)
        return ret;
    std::string symbol = parts[parts.size() - 2];
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
    for (auto it = doc["asks"].Begin(); it != doc["asks"].End(); ++it) {
        auto &item = *it;
        auto price = item[0].GetDouble();
        auto amount = item[1].GetDouble();
        d.asks[d.askCount] = { price, amount };
        if (++d.askCount >= MAXDEPTH) break;
    }
    ret[mkt->second.symbol] = d;

    return ret;
}

int OkexFut::parse_cancel_order(std::string rs) const {
    rapidjson::Document d;
    d.Parse(rs.c_str());
    return (d.IsObject() && d.HasMember("result") && d["result"].IsBool()
            && d["result"].GetBool()) ? 0 : 1;
}

std::pair<int, OrderInfo> OkexFut::parse_create_order(std::string rs) const {
    rapidjson::Document d;
    d.Parse(rs.c_str());
    std::pair<int, OrderInfo> ret;
    ret.first = 1;
    if (!(d.IsObject() && d.HasMember("result") && d["result"].IsBool() && d["result"].GetBool())) return ret;
    ret.second.id = d["order_id"].IsInt64()
        ? std::to_string(d["order_id"].GetInt64())
        : d["order_id"].GetString();
    ret.first = 0;
    return ret;
}

std::pair<int, std::map<std::string, FutureBalance>> OkexFut::parse_fetch_future_balance(std::string rs) const {
    std::pair<int, std::map<std::string, FutureBalance>> ret;
    rapidjson::Document d;
    d.Parse(rs.c_str());
    ret.first = 1;
    if (!(d.IsObject() && d.HasMember("result") && d["result"].IsBool() && d["result"].GetBool())) return ret;
    ret.first = 0;

    //std::string marginMode = d["margin_mode"].GetString();
    std::string posStr = "holding";
    for (auto it = d[posStr.c_str()].Begin(); it != d[posStr.c_str()].End(); ++it) {
        auto &item = *it;
        //std::string key = str_toupper(item["instrument_id"].GetString());
        std::string mktId = std::string(item["symbol"].GetString()) + "-" + item["contract_type"].GetString();
        auto mkt = this->marketsBySymbolContractType.find(mktId);
        if (mkt == this->marketsBySymbolContractType.end()) continue;
        std::string key = mkt->second.symbol;

        FutureBalance cb;

        if (item.HasMember("lever_rate")) {
            cb.long_leverage = cb.short_leverage = item["lever_rate"].IsString()
                                                   ? std::stod(item["lever_rate"].GetString())
                                                   : item["lever_rate"].GetDouble();
        }

        cb.long_qty = item["buy_amount"].IsString()
            ? std::stod(item["buy_amount"].GetString())
            : item["buy_amount"].GetDouble();
        cb.long_avail_qty = item["buy_available"].IsString()
            ? std::stod(item["buy_available"].GetString())
            : item["buy_available"].GetDouble();

        /*cb.long_avg_cost = item["buy_price_cost"].IsString()
                           ? std::stod(item["buy_price_cost"].GetString())
                           : item["buy_price_cost"].GetDouble();*/
        cb.long_settlement_price = item["buy_price_avg"].IsString()
                                   ? std::stod(item["buy_price_avg"].GetString())
                                   : item["buy_price_avg"].GetDouble();

        cb.short_qty = item["sell_amount"].IsString()
                       ? std::stod(item["sell_amount"].GetString())
                       : item["sell_amount"].GetDouble();
        cb.short_avail_qty = item["sell_available"].IsString()
                             ? std::stod(item["sell_available"].GetString())
                             : item["sell_available"].GetDouble();
        /*cb.short_avg_cost = item["sell_price_cost"].IsString()
                            ? std::stod(item["sell_price_cost"].GetString())
                            : item["sell_price_cost"].GetDouble();*/
        cb.short_settlement_price = item["sell_price_avg"].IsString()
                                    ? std::stod(item["sell_price_avg"].GetString())
                                    : item["sell_price_avg"].GetDouble();
        ret.second[key] = cb;
    }
    return ret;
}

CurlHandle *OkexFut::fetch_order(std::string id, std::string symbol, std::map<std::string, std::string> params,
                              std::function<void(int, OrderInfo)> func) {
    std::string method = "future_order_info.do";
    auto mkt = this->markets.find(symbol);
    if (mkt == this->markets.end()) {
        throw "OkexFut::fetch_order: unsupported market";
    }
    std::string pair = mkt->second.id;
    std::map<std::string, std::string> query = {
            {"symbol", getMktId(pair)},
            {"contract_type", mkt->second.contractType},
            {"order_id", id}
    };
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

CurlHandle *OkexFut::fetch_open_orders(std::string symbol, std::string since, int limit,
                              std::function<void(int, std::vector<OrderInfo>)> func) {
    std::string httpMethod = "POST";

    std::string method = "future_order_info.do";
    auto mkt = this->markets.find(symbol);
    if (mkt == this->markets.end()) {
        throw "OkexFut::fetch_open_orders: unsupported market";
    }
    std::string pair = mkt->second.id;

    std::map<std::string, std::string> query = {
            {"symbol", getMktId(pair)},
            {"contract_type", mkt->second.contractType},
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

std::pair<int, std::vector<OrderInfo>> OkexFut::parse_fetch_orders(std::string rs, std::string status) const {
    rapidjson::Document d;
    d.Parse(rs.c_str());
    std::pair<int, std::vector<OrderInfo>> ret;
    ret.first = 1;
    if (!(d.IsObject() && d.HasMember("result") && d["result"].IsBool() && d["result"].GetBool()
          && d.HasMember("orders") && d["orders"].IsArray())) return ret;
    ret.first = 0;
    for (auto it = d["orders"].Begin(); it != d["orders"].End(); ++it) {
        auto &item = *it;
        OrderInfo oi;
        oi.id = item["order_id"].IsInt64()
                ? std::to_string(item["order_id"].GetInt64())
                : item["order_id"].GetString();
        std::string mktId = item["contract_name"].GetString();
        auto mkt = this->marketsByContractName.find(mktId);
        if (mkt == this->marketsByContractName.end()) continue;
        oi.symbol = mkt->second.symbol;
        oi.price = item["price"].IsString()
                ? std::stod(item["price"].GetString())
                : item["price"].GetDouble();
        // 1: open long 2: open short 3: close long 4: close short
        std::string type = item["type"].IsInt()
                ? std::to_string(item["type"].GetInt())
                : item["type"].GetString();
        oi.side = type == "1" || type == "2" ? "buy" : "sell";
        oi.type = type == "1"
                  ? "open_long"
                  : (type == "2"
                     ? "open_short"
                     : (type == "3"
                        ? "close_long"
                        : "close_short"));
        oi.amount = item["amount"].IsString()
                ? std::stod(item["amount"].GetString())
                : item["amount"].GetDouble();
        oi.filled = item["deal_amount"].IsString()
                ? std::stod(item["deal_amount"].GetString())
                : item["deal_amount"].GetDouble();
        oi.status = status;
        ret.second.push_back(oi);
    }

    return ret;
}

CurlHandle *OkexFut::fetch_future_balance(std::string symbol, std::function<void(int, std::map<std::string, FutureBalance>)> func) {
    //std::string method = "future_position_4fix.do";
    std::string method = "future_position.do";
    std::string httpMethod = "POST";
    auto mkt = this->markets.find(symbol);
    if (mkt == this->markets.end()) throw "OkexFut::fetch_future_balance: mkt not found";
    std::string pair = mkt->second.id;
    std::map<std::string, std::string> query = {
            {"symbol", getMktId(pair)},
            {"contract_type", mkt->second.contractType}
    };
    std::string url = this->apiUrl + method;
    auto headers = this->get_signature_headers(url, query);
    auto response = this->fetch(url, httpMethod, headers, this->payload);
    this->lastError = response.first;
    this->lastResponse = response.second;
    if (this->lastError != 200) {
        this->lastErrorStr = this->lastResponse;
    } else {
        this->lastErrorStr = "";
    }
    auto bal = this->parse_fetch_future_balance(response.second);

    func(bal.first, bal.second);
    return NULL;
}

std::string OkexFut::getMktId(std::string symbol) {
    return str_tolower(symbol.substr(0, 3)) + "_usd";
}

CurlHandle *OkexFut::create_order(std::string symbol, std::string type, std::string side, double amount, double price,
                               std::map<std::string, std::string> params,
                               std::function<void(int, OrderInfo)> func) {
    std::string method = "future_trade.do";
    auto mkt = this->markets.find(symbol);
    if (mkt == this->markets.end()) {
        throw "OkexFut::create_order: unsupported market";
    }
    std::string pair = mkt->second.id;
    std::string typeInt = "1";
    if (side == "open_long") {
        typeInt = "1";
    } else if (side == "open_short") {
        typeInt = "2";
    } else if (side == "close_long" || side == "liquidate_long") {
        typeInt = "3";
    } else if (side == "close_short" || side == "liquidate_short") {
        typeInt = "4";
    }
    std::string matchPrice = "0";
    if (type == "market") matchPrice = "1";
    std::map<std::string, std::string> query = {{"type", typeInt},
                                                {"symbol", getMktId(pair)},
                                                {"contract_type", mkt->second.contractType},
                                                {"match_price", matchPrice}, // 1 is market order
                                                {"price", to_string_with_precision(price, mkt->second.precisionPrice)},
                                                {"amount", to_string_with_precision(amount, mkt->second.precisionAmount)},
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


CurlHandle *OkexFut::async_fetch_future_balance(std::string symbol) {
    std::string method = "future_position.do";
    std::string httpMethod = "POST";
    auto mkt = this->markets.find(symbol);
    if (mkt == this->markets.end()) throw "OkexFut::fetch_future_balance: mkt not found";
    std::string pair = mkt->second.id;
    std::map<std::string, std::string> query = {
            {"symbol", getMktId(pair)},
            {"contract_type", mkt->second.contractType}
    };
    std::string url = this->apiUrl + method;
    auto headers = this->get_signature_headers(url, query);
    auto ret = this->prepare(url, httpMethod, headers, this->payload, true);
    ret->type = HANDLER_TYPE_FUTURE_BALANCE;
    return ret;
}

CurlHandle *OkexFut::async_fetch_order(std::string id, std::string symbol, std::map<std::string, std::string> params) {
    std::string method = "future_order_info.do";
    auto mkt = this->markets.find(symbol);
    if (mkt == this->markets.end()) {
        throw "OkexFut::async_fetch_order: unsupported market";
    }
    std::string pair = mkt->second.id;
    std::map<std::string, std::string> query = {
            {"symbol", getMktId(pair)},
            {"contract_type", mkt->second.contractType},
            {"order_id", id}
    };
    std::string url = this->apiUrl + method;
    auto headers = this->get_signature_headers(url, query);
    auto ret = this->prepare(url, "POST", headers, this->payload, true);
    ret->type = HANDLER_TYPE_ORDER;
    return ret;
}

CurlHandle *OkexFut::async_fetch_open_orders(std::string symbol, std::string since, int limit) {
    std::string httpMethod = "POST";

    std::string method = "future_order_info.do";
    auto mkt = this->markets.find(symbol);
    if (mkt == this->markets.end()) {
        throw "OkexFut::fetch_open_orders: unsupported market";
    }
    std::string pair = mkt->second.id;

    std::map<std::string, std::string> query = {
            {"symbol", getMktId(pair)},
            {"contract_type", mkt->second.contractType},
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

CurlHandle *OkexFut::cancel_order_with_retries(std::string id, std::string symbol, std::map<std::string, std::string> params, int retries, double timeout, std::function<void(int, int)> func) {
    std::string method = "future_cancel.do";

    std::string url = this->apiUrl + method;
    int origRetries = retries;
    auto mkt = this->markets.find(symbol);
    if (mkt == this->markets.end()) throw "OkexFut::cancel_order_with_retries: unsupported market";
    std::string pair = mkt->second.id;
    std::map<std::string, std::string> query = {
            {"symbol", getMktId(pair)},
            {"contract_type", mkt->second.contractType},
            {"order_id", id}};
    do {
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


CurlHandle *OkexFut::async_create_order(std::string symbol, std::string type, std::string side, double amount,
                                     double price, std::map<std::string, std::string> params) {
    std::string method = "future_trade.do";
    auto mkt = this->markets.find(symbol);
    if (mkt == this->markets.end()) {
        throw "OkexFut::create_order: unsupported market";
    }
    std::string pair = mkt->second.id;
    std::string typeInt = "1";
    if (side == "open_long") {
        typeInt = "1";
    } else if (side == "open_short") {
        typeInt = "2";
    } else if (side == "close_long" || side == "liquidate_long") {
        typeInt = "3";
    } else if (side == "close_short" || side == "liquidate_short") {
        typeInt = "4";
    }
    std::string matchPrice = "0";
    if (type == "market") matchPrice = "1";
    std::map<std::string, std::string> query = {{"type", typeInt},
                                                {"symbol", getMktId(pair)},
                                                {"contract_type", mkt->second.contractType},
                                                {"match_price", matchPrice}, // 1 is market order
                                                {"price", to_string_with_precision(price, mkt->second.precisionPrice)},
                                                {"amount", to_string_with_precision(amount, mkt->second.precisionAmount)},
    };
    std::string url = this->apiUrl + method;
    auto headers = this->get_signature_headers(url, query);
    auto ret = this->prepare(url, "POST", headers, this->payload, true);

    ret->type = HANDLER_TYPE_CREATE_ORDER;
    ret->paramOrderCreate = new ParamOrderCreate();
    ret->paramOrderCreate->symbol = symbol;
    ret->paramOrderCreate->type = type;
    ret->paramOrderCreate->side = side;
    ret->paramOrderCreate->amount = amount;
    ret->paramOrderCreate->price = price;
    return ret;
}

CurlHandle *OkexFut::async_cancel_order(std::string id, std::string symbol, std::map<std::string, std::string> params) {
    std::string method = "future_cancel.do";

    std::string url = this->apiUrl + method;
    auto mkt = this->markets.find(symbol);
    if (mkt == this->markets.end()) throw "OkexFut::cancel_order_with_retries: unsupported market";
    std::string pair = mkt->second.id;
    std::map<std::string, std::string> query = {
            {"symbol", getMktId(pair)},
            {"contract_type", mkt->second.contractType},
            {"order_id", id}};
    auto headers = this->get_signature_headers(url, query);
    auto ret = this->prepare(url, "POST", headers, this->payload, true);
    ret->type = HANDLER_TYPE_CANCEL_ORDER;
    return ret;
}

}
