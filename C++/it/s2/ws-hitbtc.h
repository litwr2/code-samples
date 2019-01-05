#ifndef __WS_HITBTC_H__
#define __WS_HITBTC_H__

#include "../it-websocket.h"
#include "ccxt/exchange.h"
#include <list>

namespace wsccxt {

class Hitbtc: public WebSocketBase {
    std::string host = "api.hitbtc.com";
    std::string dir = "/api/2/ws";
    std::string port = "443";
    ssl::context ctx{ssl::context::sslv23_client};

    std::string subscribe_json(const std::string& market) {
        return std::string(R"({"method":"subscribeOrderbook","params":{"symbol":")")
               + market + R"("},"id":123})";
    }
    std::string unsubscribe_json(const std::string& market) {
        return std::string(R"({"method":"unsubscribeOrderbook","params":{"symbol":")")
               + market + R"("},"id":123})";
    }
    ccxt::ParseResult parseOrderbook(const rapidjson::Document &doc, const std::string &market, WSsession *ws);
    ccxt::ParseResult parseUpdate(const rapidjson::Document &doc, const std::string &market, WSsession *ws);
public:
    Hitbtc(base::DepthMap &map);
    WSsession* connect(std::string url, const std::map<std::string, std::string> &options);
    int subscribe(WSsession *s, const std::vector<std::string> &pair);
    int unsubscribe(WSsession *s, const std::vector<std::string> &pair);
    ccxt::ParseResult parse(const std::string &supd, WSsession *ws);
};

} // end namespace wsccxt

#endif
