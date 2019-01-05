#include "root_certificates.hpp"
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <regex>
#include <iostream>
#include "markets-loader.h"

using tcp = boost::asio::ip::tcp;               // from <boost/asio/ip/tcp.hpp>
namespace ssl = boost::asio::ssl;               // from <boost/asio/ssl.hpp>
namespace http = boost::beast::http;

std::string markets[MAX_NUMBER_OF_MARKETS];
Market_record markets_data[MAX_NUMBER_OF_MARKETS];
unsigned number_of_markets;

static std::regex json_start("^[^[]*\\[\\s*");
static std::regex json_comma("^\\s*,\\s*");
static std::regex json_end("^\\s*]\\s*");
static std::regex json_main(R"!(^\s*\{\s*"id"\s*:\s*"([0-9A-Z]+)"\s*,\s*"baseCurrency"\s*:\s*"([0-9A-Z]+)"\s*,\s*)!"
        R"!("quoteCurrency"\s*:\s*"([0-9A-Z]+)"\s*,\s*"quantityIncrement"\s*:\s*"([0-9.]+)"\s*,\s*)!"
        R"!("tickSize"\s*:\s*"([0-9.]+)"\s*,\s*"takeLiquidityRate"\s*:\s*"([0-9.]+)"\s*,\s*)!"
        R"!("provideLiquidityRate"\s*:\s*"(-?[0-9.]+)"\s*,\s*"feeCurrency"\s*:\s*"([0-9A-Z]+)"\s*\}\s*)!");

unsigned get_markets() {
    auto const host = "api.hitbtc.com";
    auto const port = "443";
    auto const target = "/api/2/public/symbol";
    int version = 10; //11;

    try {
        // The io_context is required for all I/O
        boost::asio::io_context ioc;

        // The SSL context is required, and holds certificates
        ssl::context ctx{ssl::context::sslv23_client};

        // This holds the root certificate used for verification
        load_root_certificates(ctx);

        // These objects perform our I/O
        tcp::resolver resolver{ioc};
        ssl::stream<tcp::socket> stream{ioc, ctx};

        // Set SNI Hostname (many hosts need this to handshake successfully)
        if(! SSL_set_tlsext_host_name(stream.native_handle(), host))
        {
            boost::system::error_code ec{static_cast<int>(::ERR_get_error()), boost::asio::error::get_ssl_category()};
            throw boost::system::system_error{ec};
        }

        // Look up the domain name
        auto const results = resolver.resolve(host, port);

        // Make the connection on the IP address we get from a lookup
        boost::asio::connect(stream.next_layer(), results.begin(), results.end());

        // Perform the SSL handshake
        stream.handshake(ssl::stream_base::client);

        // Set up an HTTP GET request message
        http::request<http::string_body> req{http::verb::get, target, version};
        req.set(http::field::host, host);
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

        // Send the HTTP request to the remote host
        http::write(stream, req);

        // This buffer is used for reading and must be persisted
        boost::beast::flat_buffer buffer;

        // Declare a container to hold the response
        http::response<http::dynamic_body> res;

        // Receive the HTTP response
        http::read(stream, buffer, res);

        // Parse the message to standard out
        std::cmatch cm;
        std::ostringstream so;
        so << res;
        std::string sb = so.str(); //std::move
        const char *q = sb.c_str();
        if (regex_search(q, cm, json_start))
            q = cm.suffix().first;
        else
            throw std::runtime_error("JSON markets proccesing error, stage 1");
        for (;;) {
            if (regex_search(q, cm, json_main)) {
                markets[number_of_markets] = cm[1];
                markets_data[number_of_markets].baseCurrency = cm[2];
                markets_data[number_of_markets].quoteCurrency = cm[3];
                markets_data[number_of_markets].quantityIncrement = std::stod(cm[4]);
                markets_data[number_of_markets].tickSize = std::stod(cm[5]);
                markets_data[number_of_markets].takeLiquidityRate = std::stod(cm[6]);
                markets_data[number_of_markets].provideLiquidityRate = std::stod(cm[7]);
                markets_data[number_of_markets++].feeCurrency = cm[8];
                q = cm.suffix().first;
            }
            else{std::cout << q << "\n";
                throw std::runtime_error("JSON markets proccesing error, stage 2");}
            if (regex_search(q, cm, json_comma))
                q = cm.suffix().first;
            else if (regex_search(q, cm, json_end))
                break;
            else
                throw std::runtime_error("JSON markets proccesing error, stage 3");

        }
//        std::cout << res << std::endl;

        // Gracefully close the stream
        boost::system::error_code ec;
        stream.shutdown(ec);
        if(ec == boost::asio::error::eof)
        {
            // Rationale:
            // http://stackoverflow.com/questions/25587403/boost-asio-ssl-async-shutdown-always-finishes-with-an-error
            ec.assign(0, ec.category());
        }
        if(ec)
            throw boost::system::system_error{ec};

        // If we get here then the connection is closed gracefully
    }
    catch(std::exception const& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    return number_of_markets;
}
