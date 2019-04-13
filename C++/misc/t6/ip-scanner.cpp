/*
Given 10,000 ip-addresses of servers. It is necessary to poll addresses on availability (up/down). As soon as the server has become unavailable, issue dd-MM-yyyy hh: mm: ss.fff, ip: port down to the console, if the server has risen to issue dd-MM-yyyy hh: mm: ss.fff, ip: port up. The list of servers is stored in a file in the form of ip: port. Output to console and file. To work with the network use only system calls.
*/

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <time.h>
#include <errno.h>
#include <string>
#include <utility>
#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include <mutex>
#include <chrono>
#include <atomic>
#include <map>
#include <set>

#define NUMBER_OF_THREADS 200
#define DELAY1_MS 1
#define DELAY2_MS 30

// Get current date/time, format is dd-MM-yyyy hh:mm:ss.fff
const std::string currentDateTime(const std::chrono::milliseconds& ms) {
    time_t now = ms.count()/1000;
    tm tstruct = *localtime(&now);
    char buf[80];
    strftime(buf, sizeof(buf), "%d-%m-%Y %X.", &tstruct);
    sprintf(buf + strlen(buf), "%03d", ms.count()%1000);
    return buf;
}

int checkServer(const std::string &ip, int portno) {  //returns 0 if server is open
    const char *hostname = ip.c_str();
    struct sockaddr_in serv_addr;
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0)
        return -3; //ERROR opening socket;

    hostent *server = gethostbyname(hostname);

    if (server == NULL)
        return -2; //ERROR, no such host
    memset((char*)&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    memcpy((char*)&serv_addr.sin_addr.s_addr, (char*)server->h_addr, server->h_length);
    serv_addr.sin_port = htons(portno);
    int ior = connect(sockfd, (sockaddr*)&serv_addr, sizeof(serv_addr));
    close(sockfd);
    return ior;
}

void outputAll(std::ofstream &fo, const std::string& s) {
    std::cout << s;
    fo << s << std::flush;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        std::cerr << "usage: " << argv[0] << " FILE-WITH-SERVER-LIST FILE-FOR-OUTPUT\n";
        exit(1);
    }
    std::ifstream serverList(argv[1]);
    if (!serverList) {
        std::cerr << argv[1] << " not found\n";
        exit(2);
    }
    std::ofstream output(argv[2]);
    if (!output) {
        std::cerr << argv[2] << " is not opened\n";
        exit(3);
    }
    int lineno = 0;
    std::set<std::pair<std::string, int>> servers;
    while (!serverList.eof()) {
        std::string s;
        getline(serverList, s);
        ++lineno;
        if (s.empty()) continue;  //skips empty lines
        auto pos = s.find(':');
        if (pos == std::string::npos) {
            std::cerr << "error in line #" << lineno << " of " << argv[1] << "\n";
            continue; //exit(3);
        }
        servers.insert({s.substr(0, pos), std::stoi(s.substr(pos + 1))});
    }

    std::atomic<int> qt(0);
    std::mutex mx, mxw;
    std::map<std::pair<std::string, int>, std::chrono::milliseconds> serverSet, serverSetUpdate;
    std::set<std::pair<std::string, int>> waiting;
    for (auto v: servers) {
        ++qt;
        std::thread([&, v]{
            mxw.lock();
            waiting.insert(v);
            mxw.unlock();
            if (checkServer(v.first,  v.second) == 0) {
                std::lock_guard<std::mutex> lk(mx);
                serverSet.insert({v, std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())});
            }
            mxw.lock();
            waiting.erase(v);
            mxw.unlock();
            --qt;}).detach();
        std::this_thread::sleep_for(std::chrono::milliseconds(DELAY1_MS));
        while (qt >= NUMBER_OF_THREADS);
    }
    std::multimap<std::chrono::milliseconds, std::string> sortedOutput;
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY2_MS));
ETERNAL_LOOP:
    sortedOutput.clear();
    for (auto v: servers) {
        {
        std::lock_guard<std::mutex> lk(mxw);
        if (waiting.find(v) != waiting.end()) continue;
        }
        ++qt;
        std::thread([&, v]{
            std::unique_lock<std::mutex> lk(mxw);
            waiting.insert(v);
            lk.unlock();
            if (checkServer(v.first,  v.second) == 0) {
                std::lock_guard<std::mutex> lk(mx);
                serverSetUpdate.insert({v, std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())});
            }
            lk.lock();
            waiting.erase(v);
            --qt;}).detach();
        std::this_thread::sleep_for(std::chrono::milliseconds(DELAY1_MS));
        while (qt >= NUMBER_OF_THREADS);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY2_MS));
    mx.lock();
    for (auto el: serverSet)
        if (serverSetUpdate.find(el.first) == serverSetUpdate.end())
            sortedOutput.insert({el.second, currentDateTime(el.second) + ", " + el.first.first + ":" + std::to_string(el.first.second) + " down\n"});
    for (auto el: serverSetUpdate)
        if (serverSet.find(el.first) == serverSet.end())
            sortedOutput.insert({el.second, currentDateTime(el.second) + ", " + el.first.first + ":" + std::to_string(el.first.second) + " up\n"});
    serverSet = std::move(serverSetUpdate);
    mx.unlock();
    for (auto el: sortedOutput)
        outputAll(output, el.second);
    goto ETERNAL_LOOP;
    output.close();
    return 0;
}

