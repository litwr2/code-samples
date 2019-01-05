#ifndef __HITBTC_H__
#define __HITBTC_H__

#include "../exchange.h"
#include "ws-ccxt-cpp/root_certificates.hpp"

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <functional>
#include <map>
#include <mutex>
#include <rapidjson/document.h>

using tcp = boost::asio::ip::tcp;               // from <boost/asio/ip/tcp.hpp>
namespace ssl = boost::asio::ssl;               // from <boost/asio/ssl.hpp>
namespace websocket = boost::beast::websocket;  // from <boost/beast/websocket.hpp>

namespace ccxt {
class HitBTC : public Exchange {
    std::mutex mutex;
    std::string apiUrl = "https://api.hitbtc.com/api/2/";
    std::map<std::string, std::string> get_signature_headers(std::string path,
                                                             std::map<std::string, std::string> query);
    websocket::stream<ssl::stream<tcp::socket>> *pws;
    bool useWs = false;

  public:
    HitBTC() /*: Exchange()*/;

    std::map<std::string, std::string> describe();

    std::map<std::string, Market> fetch_markets();

    std::string getScanUrl(std::vector<std::string> pairs, int limit = 20);

    std::map<std::string, Depth> parseScanResponse(std::string r, std::string url = "");

    std::map<std::string, CurrencyBalance> parse_balance(std::string rs) const;
    OrderInfo parse_order_response(std::string rs) const;
    std::pair<int, OrderInfo> parse_create_order(std::string rs) const;
    bool ws_connect();
    CurlHandle *fetch_balance(std::function<void(int, std::map<std::string, CurrencyBalance>)> func =
                                  [](int, std::map<std::string, CurrencyBalance>) {});
    CurlHandle *fetch_my_trades(std::string symbol = "", std::string since = "", int limit = 0,
                                std::function<void(int, std::vector<OrderInfo>)> func = [](int,
                                                                                           std::vector<OrderInfo>) {});
    CurlHandle *fetch_open_orders(std::string symbol = "", std::string since = "", int limit = 0,
                                  std::function<void(int, std::vector<OrderInfo>)> func =
                                      [](int, std::vector<OrderInfo>) {});
    std::pair<int, std::vector<OrderInfo>> parse_fetch_orders(std::string rs, std::string status = "closed") const;
    std::pair<int, std::vector<OrderInfo>> parse_fetch_my_trades(std::string rs, std::string status = "closed") const;
    OrderInfo parse_order(rapidjson::Value &rs) const;
    int parse_cancel_order(std::string rs) const;
    CurlHandle *fetch_order(std::string id, std::string symbol, std::map<std::string, std::string> params = {},
                            std::function<void(int, OrderInfo)> func = [](int, OrderInfo) {});
    CurlHandle *create_order(std::string symbol, std::string type, std::string side, double amount, double price,
                             std::map<std::string, std::string> params,
                             std::function<void(int, OrderInfo)> func = [](int, OrderInfo) {});
    CurlHandle *cancel_order_with_retries(std::string id, std::string symbol, std::map<std::string, std::string> params = {}, int retries = 1, double timeout = 0, std::function<void(int, int)> func = [](int, int) {});
    std::pair<int, OrderInfo> process_create_order(CurlHandle *ch);

    CurlHandle *async_fetch_balance();
    CurlHandle *async_fetch_open_orders(std::string symbol = "", std::string since = "", int limit = 0);
    CurlHandle *async_fetch_my_trades(std::string symbol = "", std::string since = "", int limit = 0);
    CurlHandle *async_fetch_order(std::string id, std::string symbol, std::map<std::string, std::string> params = {});
    CurlHandle *async_create_order(std::string symbol, std::string type, std::string side, double amount,
                             double price, std::map<std::string, std::string> params = {});
    CurlHandle *async_cancel_order(std::string id, std::string symbol, std::map<std::string, std::string> params = {});
};
} // namespace ccxt

#endif // __HITBTC_H__
