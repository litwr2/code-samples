// Copyright (c) 2018 Vol Lidovski (vol dot litwr at gmail com)
//
// Copyright (c) 2016-2017 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

//------------------------------------------------------------------------------
//
// Example: WebSocket SSL client, synchronous
//
//------------------------------------------------------------------------------

#include "root_certificates.hpp"

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <regex>
#include <chrono>
#include <map>
#include <thread>
#include <mutex>
#include <atomic>
#include <ctime>
#include <climits>
#include <exception>
#include <algorithm>
#include <signal.h>
#include "markets-loader.h"

#define SAVE_RAW_DATA 0
#define SAVED_MARKET_ID 0

#define MARKETS_LIMIT MAX_NUMBER_OF_MARKETS   //use 'ulimit -n 4096' if you want more than 255 markets
#define STAKAN_SIZE 20

using tcp = boost::asio::ip::tcp;               // from <boost/asio/ip/tcp.hpp>
namespace ssl = boost::asio::ssl;               // from <boost/asio/ssl.hpp>
namespace websocket = boost::beast::websocket;  // from <boost/beast/websocket.hpp>

static std::regex json_ini(R"!(^\{\s*"jsonrpc"\s*:\s*"([0-9.]+)"\s*,\s*"result"\s*:\s*([a-z]+)\s*,\s*"id"\s*:\s*([0-9]+)\s*\})!");
static std::regex json_query1(R"!(^\{\s*"jsonrpc"\s*:\s*"([0-9.]+)",\s*"method"\s*:\s*"(\w+)"\s*,\s*"params"\s*:\s*\{\s*"ask"\s*:\s*\[\s*)!");
static std::regex json_query2(R"!(^\{\s*"price"\s*:\s*"([0-9.]+)"\s*,\s*"size"\s*:"([0-9.]+)"\s*\}\s*)!");
static std::regex json_query3(R"(^,\s*\]?\s*)");
static std::regex json_query4(R"(^\]\s*,\s*"bid"\s*:\s*\[\s*)");
static std::regex json_query5(R"!(^\]\s*,\s*"symbol"\s*:\s*"([0-9A-Z]*)"\s*,\s*"sequence"\s*:\s*([0-9]+)\s*\}\s*\}\s*)!");

typedef double datatype;
thread_local std::map<datatype, datatype> ask;
thread_local std::map<datatype, datatype, std::greater<datatype>> bid;
thread_local unsigned long sequence;
std::map<std::string, unsigned> market_ids;
std::pair<datatype, datatype> market_asks[MARKETS_LIMIT][STAKAN_SIZE], market_bids[MARKETS_LIMIT][STAKAN_SIZE];
volatile int get_stat, get_break, get_data1, get_data2;
auto system_start = std::time(0);
auto system_start_sc = std::chrono::steady_clock::now();

#if SAVE_RAW_DATA
std::ofstream rawdata("rawdata.txt");
#endif

unsigned logprec(double prec) {
    unsigned p = 0;
    while (prec < .9999) {
        prec *= 10;
        ++p;
    }
    return p;
}

void print_stakan(unsigned id) {
    unsigned prec1 = logprec(markets_data[id].tickSize), prec2 = logprec(markets_data[id].quantityIncrement);
    markets_data[id].mx.lock();
    std::cout  << std::defaultfloat << "\n" << markets[id] << " updates=" << markets_data[id].updates[0];
    for (int k = 1; k < MAX_CONNECTIONS; k++)
        std::cout << '/' << markets_data[id].updates[k];
    std::cout << " takeLiquidityRate=" << markets_data[id].takeLiquidityRate
        << " provideLiquidityRate=" << markets_data[id].provideLiquidityRate
        << " feeCurrency=" << markets_data[id].feeCurrency << "\n";
    std::cout << "             asks                                                 bids\n" << std::fixed;
    for (unsigned count = 0; count < STAKAN_SIZE; ++count) {
        std::cout.width(16);
        std::cout.precision(prec1);
        std::cout << market_asks[id][count].first;
        std::cout.width(16);
        std::cout.precision(prec2);
        std::cout << market_asks[id][count].second << "              ";
        std::cout.width(16);
        std::cout.precision(prec1);
        std::cout << market_bids[id][count].first;
        std::cout.width(16);
        std::cout.precision(prec2);
        std::cout << market_bids[id][count].second << "\n";
    }
    markets_data[id].mx.unlock();
}

//void parse_stage1(websocket::stream<ssl::stream<tcp::socket>>& ws, unsigned id) {
void parse_stage1(websocket::stream<tcp::socket>& ws, unsigned id) {
    boost::beast::multi_buffer b;
    ws.read(b);
    std::cmatch cm;
    std::string sb = std::move(buffers_to_string(b.data()));
#if SAVE_RAW_DATA
    if (id == SAVED_MARKET_ID) rawdata.write(sb.c_str(), sb.size());
#endif
    const char *q = sb.c_str();
    if (!regex_search(q, cm, json_ini) || cm[1] != "2.0" || cm[2] != "true" || cm[3] != "123") {
        throw std::runtime_error("JSON parsing error, stage 1");
    }
}

//void parse_stage2(websocket::stream<ssl::stream<tcp::socket>>& ws, unsigned id, unsigned connection) {
void parse_stage2(websocket::stream<tcp::socket>& ws, unsigned id, unsigned connection) {
    std::cmatch cm;
    boost::beast::multi_buffer b;
    ws.read(b);
    std::string sb = std::move(buffers_to_string(b.data()));
#if SAVE_RAW_DATA
    if (id == SAVED_MARKET_ID) rawdata.write(sb.c_str(), sb.size());
#endif
    const char *q = sb.c_str();

    if (regex_search(q, cm, json_query1) && cm[1] == "2.0" && cm[2] == "snapshotOrderbook") {
        q = cm.suffix().first;
    }
    else
        throw std::runtime_error("JSON parsing error, stage 2 phase 1");
    if (regex_search(q, cm, json_query4)) goto L1;
    for (;;) {
        if (regex_search(q, cm, json_query2)) { //it can be much faster
            double key = std::stod(cm[1]), value = std::stod(cm[2]);
            ask[key] = value;
            q = cm.suffix().first;
        }
        else
            throw std::runtime_error("JSON parsing error, stage 2 phase 2");
        if (regex_search(q, cm, json_query3))
            q = cm.suffix().first;
        else
            break;
    }
    {
        auto p = ask.begin();
        markets_data[id].mx.lock();
        for (int i = 0; i < STAKAN_SIZE && p != ask.end(); ++i, ++p)
            market_asks[id][i] = std::make_pair(p->first, p->second);
        markets_data[id].mx.unlock();
    }
L1: if (regex_search(q, cm, json_query4))
        q = cm.suffix().first;
    else
        throw std::runtime_error("JSON parsing error, stage 2 phase 3");
    if (regex_search(q, cm, json_query5)) goto L2;
    for (;;) {
        if (regex_search(q, cm, json_query2)) {
            double key = std::stod(cm[1]), value = std::stod(cm[2]);
            bid[key] = value;
            q = cm.suffix().first;
        }
        else
            throw std::runtime_error("JSON parsing error, stage 2 phase 4");
        if (regex_search(q, cm, json_query3))
            q = cm.suffix().first;
        else
            break;
    }
    if (regex_search(q, cm, json_query5))
        q = cm.suffix().first;
    else
        throw std::runtime_error("JSON parsing error, stage 2 phase 5");
    {
        auto p = bid.begin();
        markets_data[id].mx.lock();
        for (int i = 0; i < STAKAN_SIZE && p != bid.end(); ++i, ++p)
            market_bids[id][i] = std::make_pair(p->first, p->second);
        markets_data[id].mx.unlock();
    }
L2: markets_data[id].updates[connection]++;
    sequence = std::stoul(cm[2]);
    markets_data[id].time[connection].push_back(make_pair(std::chrono::steady_clock::now(), sequence));
}

//void parse_stage3(websocket::stream<ssl::stream<tcp::socket>>& ws, unsigned id, unsigned connection) {
void parse_stage3(websocket::stream<tcp::socket>& ws, unsigned id, unsigned connection) {
    std::cmatch cm;
    boost::beast::multi_buffer b;
    ws.read(b);
    std::string sb = std::move(buffers_to_string(b.data()));
#if SAVE_RAW_DATA
    if (id == SAVED_MARKET_ID) rawdata.write(sb.c_str(), sb.size());
#endif
    const char *q = sb.c_str();

    if (regex_search(q, cm, json_query1) && cm[1] == "2.0" && cm[2] == "updateOrderbook") {
        q = cm.suffix().first;
    }
    else
        throw std::runtime_error("JSON parsing error, stage 3 phase 1");
    if (regex_search(q, cm, json_query4)) goto L1;
    for (;;) {
        if (regex_search(q, cm, json_query2)) {
            double key = std::stod(cm[1]), value = std::stod(cm[2]);
            if (value == 0)
                ask.erase(key);
            else
                ask[key] = value;
            q = cm.suffix().first;
        }
        else
            throw std::runtime_error("JSON parsing error, stage 3 phase 2");
        if (regex_search(q, cm, json_query3))
            q = cm.suffix().first;
        else
            break;
    }
    {
        auto p = ask.begin();
        markets_data[id].mx.lock();
        for (int i = 0; i < STAKAN_SIZE && p != ask.end(); ++i, ++p)
            market_asks[id][i] = std::make_pair(p->first, p->second);
        markets_data[id].mx.unlock();
    }
L1: if (regex_search(q, cm, json_query4))
        q = cm.suffix().first;
    else
        throw std::runtime_error("JSON parsing error, stage 3 phase 3");
    if (regex_search(q, cm, json_query5)) goto L2;
    for (;;) {
        if (regex_search(q, cm, json_query2)) {
            double key = std::stod(cm[1]), value = std::stod(cm[2]);
            if (value == 0)
                bid.erase(key);
            else
                bid[key] = value;
            q = cm.suffix().first;
        }
        else
            throw std::runtime_error("JSON parsing error, stage 3 phase 4");
        if (regex_search(q, cm, json_query3))
            q = cm.suffix().first;
        else
            break;
    }
    if (regex_search(q, cm, json_query5)) {
        q = cm.suffix().first;
    }
    else
        throw std::runtime_error("JSON parsing error, stage 3 phase 5");
    {
        auto p = bid.begin();
        markets_data[id].mx.lock();
        for (int i = 0; i < STAKAN_SIZE && p != bid.end(); ++i, ++p)
            market_bids[id][i] = std::make_pair(p->first, p->second);
        markets_data[id].mx.unlock();
    }
L2: markets_data[id].updates[connection]++;
    if ((sequence + 1 == std::stoul(cm[2])))
        ++sequence;
    else
        throw std::runtime_error("Update sequence is broken");
    markets_data[id].time[connection].push_back(make_pair(std::chrono::steady_clock::now(), sequence));
}

std::string get_market_query(unsigned id) {
    return R"({"method":"subscribeOrderbook","params":{"symbol":")"
        + markets[id] + R"("},"id":123})";
}

void get_data_from_market(unsigned id, unsigned connection) {
    static unsigned gid = 0;
    auto const host = "vissi.su"; //@"api.hitbtc.com";
    auto const dir = "/ws/"; //@"/api/2/ws";
    auto const port = "80"; //@"443";
    static std::mutex mx1, mx2, mx3;
    static std::atomic<unsigned> count(0);
    static volatile time_t current_tick, prev_tick = 0;
L1: ask.clear();
    bid.clear();
    try {
        boost::asio::io_context ioc;
        //@ssl::context ctx{ssl::context::sslv23_client};
        //@load_root_certificates(ctx);
        tcp::resolver resolver{ioc};
        //websocket::stream<ssl::stream<tcp::socket>> ws{ioc, ctx};
        websocket::stream<tcp::socket> ws{ioc};
        std::this_thread::sleep_for(std::chrono::milliseconds((id + connection*number_of_markets)*200));
        auto const results = resolver.resolve(host, port);
        //boost::asio::ip::tcp::endpoint localEndpoint(boost::asio::ip::address::from_string("172.20.10.2"), 0);
        //ws.next_layer().next_layer().open(boost::asio::ip::tcp::v4());
        //ws.next_layer().next_layer().bind(boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string("172.20.10.2"), 0));
        ///ws.next_layer().open(boost::asio::ip::tcp::v4());
        ///ws.next_layer().bind(boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string("172.20.10.2"), 0));
        //@boost::asio::connect(ws.next_layer().next_layer(), results.begin(), results.end());
        boost::asio::ip::tcp::resolver::iterator remoteEndpoint = 
        resolver.resolve(boost::asio::ip::tcp::resolver::query(host, port));
        ws.next_layer().connect(*remoteEndpoint);
        //boost::asio::connect(ws.next_layer(), results.begin(), results.end());
        //@ws.next_layer().handshake(ssl::stream_base::client);
        ws.handshake(host, dir);
        ws.write(boost::asio::buffer(get_market_query(id)));
        parse_stage1(ws, id);
        parse_stage2(ws, id, connection);
        std::cout << "Market #" << id << "[" << connection << "] (" << markets[id] << ") is connected\n";
        if (++count == number_of_markets*2) std::cout << "all markets are working\n";
        for (;;) {
            if (get_break) break;
            parse_stage3(ws, id, connection);
//            current_tick = std::time(0);
//            if (current_tick != prev_tick && current_tick%20 == 0 && mx1.try_lock() && current_tick != prev_tick) {
            if (get_stat != 0 && mx1.try_lock() && get_stat != 0) {
                current_tick = std::time(0);
                get_stat = 0;
                prev_tick = current_tick;
                unsigned sum[MAX_CONNECTIONS] = {0}, max_ups[MAX_CONNECTIONS] = {0},
                    min_ups[MAX_CONNECTIONS];
                for (int i = 0; i < MAX_CONNECTIONS; ++i)
                    min_ups[i] = std::numeric_limits<unsigned>::max();
                std::cout << "\n";
                for (int i = 0; i < number_of_markets; ++i) {
                    std::cout << markets[i] << ":" << markets_data[i].updates[0];
                    sum[0] += markets_data[i].updates[0];
                    if (markets_data[i].updates[0] > max_ups[0]) max_ups[0] = markets_data[i].updates[0];
                    if (markets_data[i].updates[0] < min_ups[0]) min_ups[0] = markets_data[i].updates[0];
                    for (int k = 1; k < MAX_CONNECTIONS; ++k) {
                        std::cout << "/" << markets_data[i].updates[k];
                        sum[k] += markets_data[i].updates[k];
                        if (markets_data[i].updates[k] > max_ups[k]) max_ups[k] = markets_data[i].updates[k];
                        if (markets_data[i].updates[k] < min_ups[k]) min_ups[k] = markets_data[i].updates[k];
                    }
                    std::cout << " ";
                }
                std::cout.precision(2);
                std::cout << std::fixed << "\nmin=" << min_ups[0];
                for (int k = 1; k < MAX_CONNECTIONS; ++k)
                    std::cout << '/' << min_ups[k];
                std::cout << ", avr=" << double(sum[0])/number_of_markets;
                for (int k = 1; k < MAX_CONNECTIONS; ++k)
                    std::cout << '/' << double(sum[k])/number_of_markets;
                std::cout << ", max=" << max_ups[0];
                for (int k = 1; k < MAX_CONNECTIONS; ++k)
                    std::cout << '/' << max_ups[k];
                std::cout<< ", cur=" << markets_data[id].updates[0];
                for (int k = 1; k < MAX_CONNECTIONS; ++k)
                    std::cout << '/' << markets_data[id].updates[k];
                std::cout << ", time=" << current_tick - system_start << "\n";
                mx1.unlock();
            }
            if (get_data1 != 0 && mx2.try_lock() && get_data1 != 0 ) {
                get_data1 = 0;
                print_stakan(gid);
                mx2.unlock();
            }
            if (get_data2 != 0 && mx3.try_lock() && get_data2 != 0) {
                get_data2 = 0;
#if 1
                for (int i = 0; i < number_of_markets; ++i) {
                    for (int k = 0; k < MAX_CONNECTIONS; ++k) {
                        auto p = markets_data[i].time[k].begin();
                        std::cout << markets[i] << ":"
                            << std::chrono::duration_cast<std::chrono::milliseconds>(p->first - system_start_sc).count()
                            << '(' << p->second << ')';
                        for (++p; p != markets_data[i].time[k].end(); ++p)
                            std::cout << "/" 
                                << std::chrono::duration_cast<std::chrono::milliseconds>(p->first - system_start_sc).count()
                                << '(' << p->second << ')';
                        std::cout << "\n";
                    }
                }
/*
                for (int i = 0; i < number_of_markets; ++i) {
                    auto p1 = markets_data[i].time[0].begin(), p2 = markets_data[i].time[1].begin(), p1s = p1, p2s = p2;
                    long sum = 0, tc = 0;
                    for (++p1, ++p2;
                        p1 != markets_data[i].time[0].end() && p2 != markets_data[i].time[1].end(); ++p1, ++p2, ++tc)
                        sum += std::abs(std::chrono::duration_cast<std::chrono::milliseconds>(*p2 - *p2s).count()
                            - std::chrono::duration_cast<std::chrono::milliseconds>(*p1 - *p1s).count());
                    std::cout << markets[i] << " " << double(sum)/tc << "\n";
                }*/
#endif
                //print_stakan(gid = (gid + 1)%MARKETS_LIMIT);
                mx3.unlock();
            }
        }
        ws.close(websocket::close_code::normal);
#if SAVE_RAW_DATA
        if (id == SAVED_MARKET_ID) rawdata.close();
#endif
    }
    catch (std::exception const& e) {
        std::cerr << "Market " << markets[id] << "[" << id << "] produces an error: " << e.what() << "\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(2400));
        if (std::string(e.what()).find("JSON parsing error, stage 1") != std::string::npos)
            std::this_thread::sleep_for(std::chrono::seconds(240));
        if (!get_break) goto L1;
    }
    std::cout << id << " exited\n";
}

void signalHandler(int signal) {
   if (signal == SIGINT)
       get_stat = 1;
   else if (signal == SIGUSR1)
       get_data1 = 1;
   else if (signal == SIGUSR2)
       get_data2 = 1;
   else
       get_break = 1;
}

// Sends a WebSocket message and prints the response
int main(int argc, char** argv) {
    std::cout << get_markets() << " markets are ready\n";
    number_of_markets = std::min(MARKETS_LIMIT, number_of_markets);
    std::cout << "Using " << number_of_markets << " of them\n";
    signal(SIGINT, signalHandler);
    signal(SIGUSR1, signalHandler);
    signal(SIGUSR2, signalHandler);
    signal(SIGTERM, signalHandler);
    for (int i = 0; i < number_of_markets; ++i)
        market_ids[markets[i]] = i;
    std::thread t[MARKETS_LIMIT][2];
    std::cout << "Connecting to markets\n";
    try {
        for (int k = 0; k < MAX_CONNECTIONS; ++k)
            for (int i = 0; i < number_of_markets; ++i)
                t[i][k] = std::thread(get_data_from_market, i, k);
        for (int k = 0; k < MAX_CONNECTIONS; ++k)
            for (int i = 0; i < number_of_markets; ++i)
                t[i][k].join();
//        for (int i = 0; i < MARKETS_LIMIT; ++i)
//            print_stakan(i);
    }
    catch(std::exception const& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

