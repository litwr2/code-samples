#ifndef __OKEX_H__
#define __OKEX_H__

#include "../exchange.h"
#include <functional>
#include <map>
#include <mutex>
#include <rapidjson/document.h>

namespace ccxt {
    class Okex : public Exchange {
    protected:
        std::mutex mutex;
        std::string apiUrl = "https://www.okex.com/api/v1/";
        virtual std::map<std::string, std::string> get_signature_headers(std::string path,
                                                                 std::map<std::string, std::string> query);
        std::string signature = "";
        std::string payload = "";
        std::map<std::string, std::map<std::string, double>> orderAmountsBySymbol;
        std::map<std::string, std::map<std::string, std::string>> orderTypesBySymbol;

    public:
        Okex() /*: Exchange()*/;

        virtual std::map<std::string, std::string> describe();

        virtual std::map<std::string, Market> fetch_markets();

        virtual std::string getScanUrl(std::vector<std::string> pairs, int limit = 20);

        virtual std::map<std::string, Depth> parseScanResponse(std::string r, std::string url = "");

        virtual std::pair<int, OrderInfo> parse_fetch_order(std::string rs) const;
        virtual std::map<std::string, CurrencyBalance> parse_fetch_balance(std::string rs) const;
        OrderInfo parse_order_response(std::string rs) const;
        virtual std::pair<int, OrderInfo> parse_create_order(std::string rs) const;
        virtual CurlHandle *fetch_balance(std::function<void(int, std::map<std::string, CurrencyBalance>)> func =
            [](int, std::map<std::string, CurrencyBalance>) {});
        virtual CurlHandle *fetch_my_trades(std::string symbol = "", std::string since = "", int limit = 0,
                                    std::function<void(int, std::vector<OrderInfo>)> func = [](int,
                                                                                               std::vector<OrderInfo>) {});
        virtual CurlHandle *fetch_open_orders(std::string symbol = "", std::string since = "", int limit = 0,
                                      std::function<void(int, std::vector<OrderInfo>)> func =
                                      [](int, std::vector<OrderInfo>) {});
        virtual std::pair<int, std::vector<OrderInfo>> parse_fetch_orders(std::string rs, std::string status = "closed") const;
        virtual std::pair<int, std::vector<OrderInfo>> parse_fetch_my_trades(std::string rs, std::string status = "closed") const;
        virtual OrderInfo parse_order(rapidjson::Value &rs) const;
        virtual int parse_cancel_order(std::string rs) const;
        virtual CurlHandle *fetch_order(std::string id, std::string symbol, std::map<std::string, std::string> params = {},
                                std::function<void(int, OrderInfo)> func = [](int, OrderInfo) {});
        virtual CurlHandle *create_order(std::string symbol, std::string type, std::string side, double amount, double price,
                                 std::map<std::string, std::string> params,
                                 std::function<void(int, OrderInfo)> func = [](int, OrderInfo) {});
        virtual CurlHandle *cancel_order_with_retries(std::string id, std::string symbol, std::map<std::string, std::string> params = {}, int retries = 1, double timeout = 0, std::function<void(int, int)> func = [](int, int) {});

        virtual CurlHandle *async_fetch_balance();
        virtual CurlHandle *async_fetch_open_orders(std::string symbol = "", std::string since = "", int limit = 0);
        virtual CurlHandle *async_fetch_my_trades(std::string symbol = "", std::string since = "", int limit = 0);
        virtual CurlHandle *async_fetch_order(std::string id, std::string symbol, std::map<std::string, std::string> params = {});
        virtual CurlHandle *async_create_order(std::string symbol, std::string type, std::string side, double amount,
                                       double price, std::map<std::string, std::string> params = {});
        virtual CurlHandle *async_cancel_order(std::string id, std::string symbol, std::map<std::string, std::string> params = {});
    };
} // namespace ccxt

#endif // __OKEX_H__
