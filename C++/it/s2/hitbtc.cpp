#include "./hitbtc.h"
#include "ws-ccxt-cpp/wss-client.h"
#include "../exchange.h"
#include "base/base64.h"
#include "base/RedisHelper.h"
#include "base/Log.h"
#include <map>
#include <rapidjson/document.h>
#include <string>
#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <chrono>
#include <mutex>
#include <sstream>
#include <streambuf>
#include <vector>

namespace ccxt {
// description
std::map<std::string, std::string> HitBTC::describe() {
    return {
        {"id", "hitbtc"}, {"name", "HitBTC"}, {"countries", "UK"}, {"version", "v1"},
    };
}

HitBTC::HitBTC() : Exchange() {
    this->hasMap["CORS"] = false;
    this->hasMap["cancelOrders"] = true;
    this->hasMap["createMarketOrder"] = false;
    this->hasMap["fetchDepositAddress"] = true;
    this->hasMap["fetchTickers"] = true;
    this->hasMap["fetchOHLCV"] = true; // see the method implementation below
    this->hasMap["fetchOrder"] = true;
    this->hasMap["fetchOrders"] = false;
    this->hasMap["fetchClosedOrders"] = true;
    this->hasMap["fetchOpenOrders"] = true;
    this->hasMap["fetchMyTrades"] = "emulated"; // this method is to be deleted; see implementation and
    // comments below
    this->hasMap["fetchCurrencies"] = true;
    this->hasMap["withdraw"] = true;
}

std::map<std::string, Market> HitBTC::fetch_markets() {
    auto parseDoc = [&](const std::string &str) {
        rapidjson::Document d;
        d.Parse(str.c_str());
        if (d.IsArray()) for (rapidjson::Value::ConstValueIterator itr = d.Begin(); itr != d.End(); ++itr) {
            const rapidjson::Value &id = (*itr)["id"];
            const rapidjson::Value &base = (*itr)["baseCurrency"];
            const rapidjson::Value &quote = (*itr)["quoteCurrency"];

            const rapidjson::Value &lotValue = (*itr)["quantityIncrement"];
            std::string lotValueStr = lotValue.IsString()
                ? lotValue.GetString()
                : ((lotValue.IsNumber()) ? ccxt::to_string_with_precision(lotValue.GetDouble(), 9) : "");

            const rapidjson::Value &tickSizeValue = (*itr)["tickSize"];
            std::string tickSizeStr = tickSizeValue.IsString()
                ? tickSizeValue.GetString()
                : ((tickSizeValue.IsNumber()) ? ccxt::to_string_with_precision(tickSizeValue.GetDouble(), 9) : "");

            Market m;
            m.id = id.GetString();
            m.base = base.GetString();
            m.quote = quote.GetString();
            m.symbol = m.base + "/" + m.quote;
            try {
                m.lot = std::stod(lotValueStr);
            } catch (...) {
                base::Log::log(LOG_LEVEL_WARNING, std::string("hitbtc fetch_markets: error converting lot: ") + lotValueStr);
            }
            try {
                m.step = std::stod(tickSizeStr);
            } catch (...) {
                base::Log::log(LOG_LEVEL_WARNING, std::string("hitbtc fetch_markets: error converting step: ") + tickSizeStr);
            }
            m.precisionPrice = precisionFromString(tickSizeStr);
            m.precisionAmount = precisionFromString(lotValueStr);

            m.limitAmountMin = m.lot;
            m.limitPriceMin = m.step;
            m.limitCostMin = m.limitAmountMin * m.limitPriceMin;

            this->markets[m.symbol] = m;
            this->marketsById[m.id] = m;
        }
    };
    if (!this->markets.size()) {
        std::string infoUrl = this->apiUrl + "public/symbol";
        auto response = this->fetch(infoUrl, "GET", {});
        mutex.lock();
        parseDoc(response.second);
        mutex.unlock();
    }
    if (!this->markets.size()) {
        mutex.lock();
        const char *filename = "markets/hitbtc-markets.json";
        std::ifstream t(filename);
        std::string str((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
        parseDoc(str);
        this->mutex.unlock();
    }

    return this->markets;
}

std::string HitBTC::getScanUrl(std::vector<std::string> pairs, int limit) {
    if (pairs.size() == 1) {
        auto it = this->markets.find(pairs.front());
        std::string pairsReq = pairs.front();
        pairsReq.erase(std::remove(pairsReq.begin(), pairsReq.end(), '/'), pairsReq.end());
        if (it != this->markets.end()) {
            pairsReq = it->second.id;
        }

        return std::string("https://api.hitbtc.com/api/2/public/orderbook/") + pairsReq + std::string("?rnd=") +
               std::to_string(rand() % 100000);
    }

    return "";
}

std::map<std::string, Depth> HitBTC::parseScanResponse(std::string rs, std::string url) {
    std::map<std::string, Depth> res;
    auto pos = url.find('?');
    if (pos != std::string::npos) {
        url = url.substr(0, pos);
    }
    auto parts = explode(url, '/');
    if (!parts.size())
        return res;
    std::string mkt = parts[parts.size() - 1];
    auto it = this->marketsById.find(mkt);
    if (it == this->marketsById.end()) {
        return res;
    }
    rapidjson::Document r;
    r.Parse(rs.c_str());
    if (!r.IsObject()) {
        return res;
    }
    Depth d;
    int askCount = 0;
    int bidCount = 0;
    if (r.HasMember("ask")) {
        for (rapidjson::Value::ConstValueIterator itr = r["ask"].Begin(); itr != r["ask"].End(); ++itr) {
            const rapidjson::Value &price = (*itr)["price"];
            const rapidjson::Value &size = (*itr)["size"];
            double askPrice = std::stod(price.GetString());
            double askSize = std::stod(size.GetString());

            std::pair<double, double> p = {askPrice, askSize};

            d.asks[askCount] = p;
            ++askCount;
            if (askCount >= MAXDEPTH)
                break;
        }
        d.askCount = askCount;
    }
    if (r.HasMember("bid")) {
        for (rapidjson::Value::ConstValueIterator itr = r["bid"].Begin(); itr != r["bid"].End(); ++itr) {
            const rapidjson::Value &price = (*itr)["price"];
            const rapidjson::Value &size = (*itr)["size"];
            double bidPrice = std::stod(price.GetString());
            double bidSize = std::stod(size.GetString());

            std::pair<double, double> p = {bidPrice, bidSize};
            d.bids[bidCount] = p;
            ++bidCount;
            if (bidCount >= MAXDEPTH)
                break;
        }
        d.bidCount = bidCount;
    }
    res[it->second.symbol] = d;
    return res;
}

std::pair<int, OrderInfo> HitBTC::process_create_order(CurlHandle *ch) {
    return Exchange::process_create_order(ch);
}

int HitBTC::parse_cancel_order(std::string rs) const {
    rapidjson::Document d;
    d.Parse(rs.c_str());
    rapidjson::Value &item = d;
    if (!item.IsObject()) return 1;
    if (!item.HasMember("clientOrderId")) return 1;
    return 0;
}

std::pair<int, OrderInfo> HitBTC::parse_create_order(std::string rs) const {
    rapidjson::Document d;
    d.Parse(rs.c_str());
    std::pair<int, OrderInfo> ret;
    ret.second = parse_order(d);
    if (!ret.second.id.length()) ret.first = 1;
    return ret;
}

CurlHandle *HitBTC::fetch_my_trades(std::string symbol, std::string since, int limit,
                            std::function<void(int, std::vector<OrderInfo>)> func) {
    std::string url = this->apiUrl + "history/trades";

    std::map<std::string, std::string> query;
    auto headers = this->get_signature_headers("", query);
    if (symbol.length() && markets.find(symbol) != markets.end()) {
        auto mkt = markets[symbol];
        query["symbol"] = mkt.id;
    }
    if (limit > 0) {
        query["limit"] = std::to_string(limit);
    }
    if (query.size())
        url += "?" + http_build_query(query);
    auto response = this->fetch(url, "GET", headers);
    auto r = this->parse_fetch_orders(response.second, "closed");

    func(r.first, r.second);
    return NULL;
}

CurlHandle *HitBTC::fetch_open_orders(std::string symbol, std::string since, int limit,
                              std::function<void(int, std::vector<OrderInfo>)> func) {
    std::string url = this->apiUrl + "order";

    std::map<std::string, std::string> query;
    auto headers = this->get_signature_headers("", query);
    if (symbol.length() && markets.find(symbol) != markets.end()) {
        auto mkt = markets[symbol];
        url += "?symbol=" + mkt.id;
    }
    auto response = this->fetch(url, "GET", headers);
    auto r = this->parse_fetch_orders(response.second, "open");

    func(r.first, r.second);
    return NULL;
}

CurlHandle *HitBTC::fetch_order(std::string id, std::string symbol, std::map<std::string, std::string> params,
                        std::function<void(int, OrderInfo)> func) {
    std::string url = this->apiUrl + "order/" + id;

    std::map<std::string, std::string> query;
    auto headers = this->get_signature_headers("", query);
    auto response = this->fetch(url, "GET", headers);
    if (response.first == 200) {
        lastError = 0;
        lastErrorStr = "";
    } else {
        lastError  = response.first;
        lastErrorStr = response.second;
    }
    rapidjson::Document d;
    d.Parse(response.second.c_str());
    auto r = this->parse_order(d);

    func(r.id.length() > 0 ? 0 : 1, r);
    return NULL;
}

CurlHandle *HitBTC::create_order(std::string symbol, std::string type, std::string side, double amount, double price,
                         std::map<std::string, std::string> params,
                         std::function<void(int, OrderInfo)> func) {
    static unsigned count;
    std::string url = this->apiUrl + "order";
    auto mkt = markets[symbol];
    if (side == "buy" && params.find("timeInForce") == params.end()) {
        params["timeInForce"] = "FOK";
    }

    if (useWs) {
       std::string sb = std::string(R"({"method":"newOrder","params":{"clientOrderId":")")
               + std::to_string(std::chrono::duration_cast<std::chrono::microseconds>(
                   std::chrono::system_clock::now().time_since_epoch()).count()) + "beef"
               + R"(","symbol": ")" + mkt.id + R"(","side":")" + side + R"(","price":")"
               + ccxt::to_string_with_precision(price, mkt.precisionPrice)
               + ((params.find("timeInForce") != params.end()) ? (R"(","timeInForce":")" + params["timeInForce"]) : "")
               + R"(","quantity": ")" + ccxt::to_string_with_precision(amount, mkt.precisionAmount)
               + R"("},"id":)" + std::to_string(++count) + "}";
        pws->write(boost::asio::buffer(sb));
        boost::beast::multi_buffer b;
        rapidjson::Document doc;
        pws->read(b);
        //read_with_timeout(pws->next_layer().next_layer(), b, 10);
        std::string response = std::move(buffers_to_string(b.data()));
        this->lastErrorStr = response;
        doc.Parse(response.c_str());
        if (!doc.IsObject() || !doc.HasMember("result") || !doc["result"].HasMember("symbol")) {
            func(1, OrderInfo());
            return NULL;
        }
        auto r = this->parse_order(doc["result"]);
        //hitbtc FOK hack
        if (params.find("timeInForce") != params.end() && (r.status == "new" || r.status == "open")) {
            r.status = "closed";
            r.filled = r.amount;
        }
        func(r.id.length() > 0 ? 0 : 1, r);
    }
    else {
        std::map<std::string, std::string> query = {
                {"symbol",   mkt.id},
                {"side",     side},
                {"quantity", ccxt::to_string_with_precision(amount, mkt.precisionAmount)},
                {"price",    ccxt::to_string_with_precision(price, mkt.precisionPrice)},
                {"type",     "limit"}
        };
        for (auto it = params.begin(); it != params.end(); ++it)
            query[it->first] = it->second;
        std::string postData = http_build_query(query);
        auto headers = this->get_signature_headers("create_order", query);
        auto response = this->fetch(url, "POST", headers, postData);
        if (response.first == 200) {
            lastError = 0;
            lastErrorStr = "";
        } else {
            lastError = response.first;
            lastErrorStr = response.second;
        }
        rapidjson::Document d;
        d.Parse(response.second.c_str());
        auto r = this->parse_order(d);
        func(r.id.length() > 0 ? 0 : 1, r);
    }
    return NULL;
}

CurlHandle *HitBTC::cancel_order_with_retries(std::string id, std::string symbol, std::map<std::string, std::string> params, int retries, double timeout, std::function<void(int, int)> func) {
    std::string url = this->apiUrl + "order/" + id;

    std::map<std::string, std::string> query;
    auto headers = this->get_signature_headers("", query);
    int origRetries = retries;
    do {
        auto response = this->fetch(url, "DELETE", headers);
        rapidjson::Document d;
        int success = this->parse_cancel_order(response.second);
        if (!success) {
            func(success, origRetries - retries + 1);
            return NULL;
        }
        retries--;
        usleep((int) (timeout * 1000.0));
    } while (retries > 0);

    func(1, origRetries);
    return NULL;
}

std::map<std::string, std::string> HitBTC::get_signature_headers(std::string path,
                                                             std::map<std::string, std::string> query) {
    std::map<std::string, std::string> ret;
    std::string payload = this->bot.key_pub + ":" + this->bot.key_sec;
    std::string base64 = base64_encode((unsigned char*) payload.c_str(), payload.length());
    ret["Authorization"] = std::string("Basic ") + base64;
    if (path != "create_order")
        ret["Content-Type"] = "application/json";
    return ret;
}

std::pair<int, std::vector<OrderInfo>> HitBTC::parse_fetch_my_trades(std::string rs, std::string status) const {
    return this->parse_fetch_orders(rs, status);
}

std::pair<int, std::vector<OrderInfo>> HitBTC::parse_fetch_orders(std::string rs, std::string status) const {
    std::pair<int, std::vector<OrderInfo>> ret;
    ret.first = 1;
    rapidjson::Document d;
    d.Parse(rs.c_str());
    if (!d.IsArray()) return ret;
    ret.first = 0;
    for (auto it = d.Begin(); it != d.End(); ++it) {
        auto &item = *it;

        OrderInfo oi = parse_order(*it);

        if (oi.id.length()) ret.second.push_back(oi);
    }
    return ret;
}
double parseISO8601(std::string input)
{
    // prepare the data output placeholders
    struct std::tm time = {0};
    int millis;

    // string cleaning for strtol() - this could be made cleaner, but for the sake of the example itself...
    std::string cleanInput = input
            .replace(4, 1, 1, ' ')
            .replace(7, 1, 1, ' ')
            .replace(10, 1, 1, ' ')
            .replace(13, 1, 1, ' ')
            .replace(16, 1, 1, ' ')
            .replace(19, 1, 1, ' ');

    // pointers for std::strtol()
    const char* timestamp = cleanInput.c_str();
    // last parsing end position - it's where strtol finished parsing the last number found
    char* endPointer;
    // the casts aren't necessary, but I just wanted CLion to be quiet ;)
    // first parse - start with the timestamp string, give endPointer the position after the found number
    time.tm_year = (int) std::strtol(timestamp, &endPointer, 10) - 1900;
    // next parses - use endPointer instead of timestamp (skip the part, that's already parsed)
    time.tm_mon = (int) std::strtol(endPointer, &endPointer, 10) - 1;
    time.tm_mday = (int) std::strtol(endPointer, &endPointer, 10);
    time.tm_hour = (int) std::strtol(endPointer, &endPointer, 10);
    time.tm_min = (int) std::strtol(endPointer, &endPointer, 10);
    time.tm_sec = (int) std::strtol(endPointer, &endPointer, 10);
    millis = (int) std::strtol(endPointer, &endPointer, 10);

    // convert the tm struct into time_t and then from seconds to milliseconds
    return std::mktime(&time) + millis / 1000.0;
}

OrderInfo HitBTC::parse_order(rapidjson::Value &rs) const {
    OrderInfo oi;
    rapidjson::Value &item = rs;
    if (!item.IsObject()) return oi;
    if (!item.HasMember("clientOrderId")) return oi;

    std::string marketId = item["symbol"].GetString();
    std::string statusStr = item.HasMember("status") ? item["status"].GetString() : "";
    if (marketsById.find(marketId) == marketsById.end()) return oi; // error
    auto mkt = marketsById.at(marketId);

    oi.symbol = mkt.symbol;
    oi.status = (statusStr == "filled" || statusStr == "") ? "closed" : "open";
    if (statusStr == "canceled") oi.status = statusStr;
    if (statusStr == "expired") oi.status = "expired";
    // suspended, expired?
    oi.type = item.HasMember("type") ? item["type"].GetString() : "limit";
    oi.side = item["side"].GetString();
    double amount = 0.0;
    double filled = 0.0;
    double price = 0.0;
    if (item["quantity"].IsString()) {
        amount = std::stod(item["quantity"].GetString());
    } else if (item["quantity"].IsNumber()) {
        amount = item["quantity"].GetDouble();
    }
    if (item.HasMember("cumQuantity")) {
        if (item["cumQuantity"].IsString()) {
            filled = std::stod(item["cumQuantity"].GetString());
        } else if (item["cumQuantity"].IsNumber()) {
            filled = item["cumQuantity"].GetDouble();
        }
    } else {
        filled = amount;
    }
    if (item["price"].IsString()) {
        price = std::stod(item["price"].GetString());
    } else if (item["price"].IsNumber()) {
        price = item["price"].GetDouble();
    }
    oi.amount = amount;
    oi.filled = filled;
    oi.price = price;
    oi.remaining = oi.amount - oi.filled;
    oi.cost = oi.filled * oi.price;
    oi.id = item["clientOrderId"].GetString();
    if (item.HasMember("timeInForce"))
        oi.timeInForce = item["timeInForce"].GetString();
    if (item.HasMember("createdAt")) {
        std::string orderTime = item["createdAt"].GetString();
        oi.timestamp = parseISO8601(orderTime);
        oi.timestamp_str = orderTime;
    }

    oi.info["response"] = base::GetJsonText(rs);

    return oi;
}

std::map<std::string, CurrencyBalance> HitBTC::parse_balance(std::string rs) const {
    std::map<std::string, CurrencyBalance> ret;
    rapidjson::Document d;
    d.Parse(rs.c_str());
    if (!d.IsArray()) return ret;
    for (auto it = d.Begin(); it != d.End(); ++it) {
        auto &o = *it;
        if (!o.IsObject()) continue;
        if (!o.HasMember("currency") || !o.HasMember("available") || !o.HasMember("reserved")) continue;
        std::string currency = o["currency"].GetString();
        std::string availableStr = o["available"].GetString();
        std::string reservedStr = o["reserved"].GetString();
        double available = std::stod(availableStr);
        double reserved = std::stod(reservedStr);
        CurrencyBalance cb;
        cb.free = available;
        cb.used = reserved;
        cb.total = cb.free + cb.used;
        ret[currency] = cb;
    }
    return ret;
};

CurlHandle *HitBTC::fetch_balance(std::function<void(int, std::map<std::string, CurrencyBalance>)> func) {
    std::string url = this->apiUrl + "trading/balance";

    std::map<std::string, std::string> query;
    auto headers = this->get_signature_headers("", query);
    auto response = this->fetch(url, "GET", headers);
    std::map<std::string, CurrencyBalance> curBal = this->parse_balance(response.second);

    func(curBal.size() > 0 ? 0 : 1, curBal);
    return NULL;
}

CurlHandle *HitBTC::async_fetch_balance() {
    std::string url = this->apiUrl + "trading/balance";

    std::map<std::string, std::string> query;
    auto headers = this->get_signature_headers("", query);
    CurlHandle *ret = this->prepare(url, "GET", headers, "", true);
    ret->type = HANDLER_TYPE_BALANCE;
    return ret;
}

CurlHandle *HitBTC::async_fetch_open_orders(std::string symbol, std::string since, int limit) {
    std::string url = this->apiUrl + "order";

    std::map<std::string, std::string> query;
    auto headers = this->get_signature_headers("", query);
    if (symbol.length() && markets.find(symbol) != markets.end()) {
        auto mkt = markets[symbol];
        url += "?symbol=" + mkt.id;
    }
    auto r = this->prepare(url, "GET", headers, "", true);
    r->type = HANDLER_TYPE_ORDERS;
    return r;
}

CurlHandle *HitBTC::async_fetch_my_trades(std::string symbol, std::string since, int limit) {
    std::string url = this->apiUrl + "history/trades";

    std::map<std::string, std::string> query;
    auto headers = this->get_signature_headers("", query);
    if (symbol.length() && markets.find(symbol) != markets.end()) {
        auto mkt = markets[symbol];
        query["symbol"] = mkt.id;
    }
    if (limit > 0) {
        query["limit"] = std::to_string(limit);
    }
    if (query.size())
        url += "?" + http_build_query(query);
    auto r = this->prepare(url, "GET", headers, "", true);
    r->type = HANDLER_TYPE_MY_TRADES;
    return r;
}

CurlHandle *HitBTC::async_fetch_order(std::string id, std::string symbol, std::map<std::string, std::string> params) {
    std::string url = this->apiUrl + "order/" + id;

    std::map<std::string, std::string> query;
    auto headers = this->get_signature_headers("", query);
    auto r = this->prepare(url, "GET", headers, "", true);
    r->type = HANDLER_TYPE_ORDER;
    return r;

}

CurlHandle *HitBTC::async_create_order(std::string symbol, std::string type, std::string side, double amount,
                         double price, std::map<std::string, std::string> params) {
    std::string url = this->apiUrl + "order";
    auto mkt = markets[symbol];

    std::map<std::string, std::string> query = {
            {"symbol", mkt.id},
            {"side", side},
            {"quantity", ccxt::to_string_with_precision(amount, mkt.precisionAmount)},
            {"price", ccxt::to_string_with_precision(price, mkt.precisionPrice)},
            {"type", "limit"}
    };
    for (auto it = params.begin(); it != params.end(); ++it)
        query[it->first] = it->second;
    std::string postData = http_build_query(query);
    auto headers = this->get_signature_headers("create_order", query);
    auto r = this->prepare(url, "POST", headers, postData, true);
    r->type = HANDLER_TYPE_CREATE_ORDER;
    return r;
}

CurlHandle *HitBTC::async_cancel_order(std::string id, std::string symbol, std::map<std::string, std::string> params) {
    std::string url = this->apiUrl + "order/" + id;

    std::map<std::string, std::string> query;
    auto headers = this->get_signature_headers("", query);
    auto r = this->prepare(url, "DELETE", headers, "", true);
    r->type = HANDLER_TYPE_CANCEL_ORDER;
    return r;
}

bool HitBTC::ws_connect() {
    auto const host = "api.hitbtc.com";
    auto const dir = "/api/2/ws";
    auto const port = "443";

    boost::asio::io_context ioc;
    tcp::resolver resolver{ioc};
    auto const results = resolver.resolve(host, port);

    ssl::context ctx{ssl::context::sslv23_client};
    load_root_certificates(ctx);
    pws = new websocket::stream<ssl::stream<tcp::socket>>{ioc, ctx};
    pws->next_layer().next_layer().connect(*results);
    pws->next_layer().handshake(ssl::stream_base::client);
    pws->handshake(host, dir);
    std::string sb = R"({"method":"login","params":{"algo": "BASIC","pKey":")"
        + this->bot.key_pub + R"(","sKey": ")" + this->bot.key_sec + "\"}}";
    pws->write(boost::asio::buffer(sb));
    boost::beast::multi_buffer b;
    rapidjson::Document doc;
    pws->read(b);
    sb = std::move(buffers_to_string(b.data()));
    doc.Parse(sb.c_str());
    if (!doc.IsObject() || !doc.HasMember("result") || !doc["result"].GetBool()) throw "Login failure";
    useWs = true;

    return true;
}
} // namespace ccxt
