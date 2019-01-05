#ifndef __WS_COINTIGER_H__
#define __WS_COINTIGER_H__

#include "../it-websocket.h"
#include "ccxt/exchange.h"
#include <list>

namespace wsccxt {

class Cointiger: public WebSocketBase {
    std::string host = "api.cointiger.pro";
    std::string dir = "/exchange-market/ws";
    std::string port = "443";
    ssl::context ctx{ssl::context::sslv23_client};

    std::string subscribe_json(const std::string& markets) {
        return std::string(R"({"event":"sub","params":{"channel":"market_)")
               + markets + R"(_depth_step0","cb_id":")" + markets + R"(","asks":10,"bids":10}})";
    }
    std::string unsubscribe_json(const std::string& markets) {
        return std::string(R"({"event":"unsub","params":{"channel":"market_)")
               + markets + R"(_depth_step0","cb_id":")" + markets + R"("}})";
    }
    ccxt::ParseResult parseOrderbook(const rapidjson::Document &doc, const std::string &market, WSsession *ws);
    ccxt::ParseResult parseUpdate(const rapidjson::Document &doc, const std::string &market, WSsession *ws);
public:
    Cointiger(base::DepthMap &map);
    WSsession* connect(std::string url, const std::map<std::string, std::string> &options);
    int subscribe(WSsession *s, const std::vector<std::string> &pair);
    int unsubscribe(WSsession *s, const std::vector<std::string> &pair);
    ccxt::ParseResult parse(const std::string &supd, WSsession *ws);
};

} // end namespace wsccxt

#endif
