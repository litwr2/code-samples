#ifndef __OKEX_FUT_H__
#define __OKEX_FUT_H__

#include "../exchange.h"
#include "okex.h"
#include <functional>
#include <map>
#include <mutex>
#include <rapidjson/document.h>

namespace ccxt {
    class OkexFut : public Okex {
    protected:
      std::string marketsUrl = "https://www.okex.com/api/futures/v3/instruments";
      std::string v3Url = "https://www.okex.com/api/futures/v3/";
      std::string getMktId(std::string symbol);
      // BTC1102
      std::map<std::string, Market> marketsByContractName;
      // btc_usd-this_week
      std::map<std::string, Market> marketsBySymbolContractType;

    public:

      long long timeSkew = 28800;

      OkexFut();
      virtual std::map<std::string, std::string> describe();

      virtual std::map<std::string, Market> fetch_markets();
      virtual std::string getScanUrl(std::vector<std::string> pairs, int limit = 20);
      virtual std::map<std::string, Depth> parseScanResponse(std::string r, std::string url = "");

      virtual int parse_cancel_order(std::string rs) const;
      virtual std::pair<int, OrderInfo> parse_create_order(std::string rs) const;
      virtual CurlHandle *create_order(std::string symbol, std::string type, std::string side, double amount, double price,
                                       std::map<std::string, std::string> params,
                                       std::function<void(int, OrderInfo)> func = [](int, OrderInfo) {});
      virtual CurlHandle *fetch_open_orders(std::string symbol = "", std::string since = "", int limit = 0,
                                      std::function<void(int, std::vector<OrderInfo>)> func =
                                      [](int, std::vector<OrderInfo>) {});
      virtual CurlHandle *fetch_order(std::string id, std::string symbol, std::map<std::string, std::string> params = {},
                                        std::function<void(int, OrderInfo)> func = [](int, OrderInfo) {});
      virtual std::pair<int, std::map<std::string, FutureBalance>> parse_fetch_future_balance(std::string rs) const;
      virtual CurlHandle *fetch_future_balance(std::string symbol, std::function<void(int, std::map<std::string, FutureBalance>)> func);
      virtual std::pair<int, std::vector<OrderInfo>> parse_fetch_orders(std::string rs, std::string status) const;

      virtual CurlHandle *cancel_order_with_retries(std::string id, std::string symbol, std::map<std::string, std::string> params = {}, int retries = 1, double timeout = 0, std::function<void(int, int)> func = [](int, int) {});

      virtual CurlHandle *async_fetch_future_balance(std::string symbol);
      virtual CurlHandle *async_fetch_open_orders(std::string symbol = "", std::string since = "", int limit = 0);
      virtual CurlHandle *async_create_order(std::string symbol, std::string type, std::string side, double amount,
                                             double price, std::map<std::string, std::string> params = {});
      virtual CurlHandle *async_cancel_order(std::string id, std::string symbol, std::map<std::string, std::string> params = {});
      virtual CurlHandle *async_fetch_order(std::string id, std::string symbol, std::map<std::string, std::string> params = {});

    };
} // namespace ccxt

#endif // __OKEX_FUT_H__
