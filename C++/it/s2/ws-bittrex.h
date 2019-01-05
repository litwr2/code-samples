#ifndef __WS_BITTREX_H__
#define __WS_BITTREX_H__

#include "../it-websocket.h"
#include "ccxt/exchange.h"
#include <list>

namespace wsccxt {

class Bittrex: public WebSocketBase {
    std::string host = "socket.bittrex.com";
    std::string dir = "/signalr/connect?transport=webSockets&clientProtocol="; //should be fully filled before a use!
    std::string port = "443";
    ssl::context ctx{ssl::context::sslv23_client};

    std::string subscribe_json1(const std::string& market) {
        return std::string(R"({"H":"c2","M":"SubscribeToExchangeDeltas","A":[")")
               + market + R"("],"I":2})";
    }
    std::string subscribe_json2(const std::string& market) {
        return std::string(R"({"H":"c2","M":"QueryExchangeState","A":[")")
               + market + R"("],"I":1})";
    }
    ccxt::ParseResult parseOrderbook(const std::string&, WSsession *ws);
    ccxt::ParseResult parseUpdate(const std::string&, WSsession *ws);
public:
    ccxt::Bittrex wsBittrex;
    Bittrex(base::DepthMap &map);
    WSsession* connect(std::string url, const std::map<std::string, std::string> &options);
    int subscribe(WSsession *s, const std::vector<std::string> &pair);
    int unsubscribe(WSsession *s, const std::vector<std::string> &pair);
    ccxt::ParseResult parse(const std::string &supd, WSsession *ws);
};

} // end namespace wsccxt

#endif
