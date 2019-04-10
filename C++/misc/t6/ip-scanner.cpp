/*
Given 10,000 ip-addresses of servers. It is necessary to poll addresses on availability (up / down). As soon as the server has become unavailable, issue dd-MM-yyyy hh: mm: ss.fff, ip: port down to the console, if the server has risen to issue dd-MM-yyyy hh: mm: ss.fff, ip: port up. The list of servers is stored in a file in the form of ip: port. Output to console and file. To work with the network use only system calls.
*/
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <string>
#include <vector>
#include <utility>
#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include <mutex>
#include <chrono>
#include <atomic>
#include <time.h>

#define NUMBER_OF_THREADS 255

// Get current date/time, format is dd-MM-yyyy hh:mm:ss.fff
const std::string currentDateTime() {
    std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
    time_t     now = ms.count()/1000;
    struct tm  tstruct = *localtime(&now);
    char       buf[80];
    strftime(buf, sizeof(buf), "%d-%m-%Y %X.", &tstruct);
    sprintf(buf + strlen(buf), "%03d", ms.count()%1000);
    return buf;
}

int checkServer(const std::string &ip, int portno) {  //returns 0 if server is open
    const char *hostname = ip.c_str();
    struct sockaddr_in serv_addr;
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0)
        return -1; //ERROR opening socket;

    struct hostent *server = gethostbyname(hostname);

    if (server == NULL)
        return -2; //ERROR, no such host

    memset((char*)&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    memcpy((char*)&serv_addr.sin_addr.s_addr, (char*)server->h_addr, server->h_length);
    serv_addr.sin_port = htons(portno);
    int ior = connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    close(sockfd);
    return ior;
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
    std::vector<std::pair<std::string, int>> servers;
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
        servers.push_back({s.substr(0, pos), std::stoi(s.substr(pos + 1))});
    }

    std::atomic<int> qt(0);
    std::mutex mx;
//ETERNAL_LOOP:
    for (auto v: servers) {
        ++qt;
        std::thread([v, &qt, &mx, &output]{
            std::string s = currentDateTime() + ", " + v.first + ":" + std::to_string(v.second);
            if (checkServer(v.first,  v.second) == 0)
                s += " up\n";
            else
                s += " down\n";
            mx.lock();
            std::cout << s;
            output << s; 
            mx.unlock();
            --qt;}).detach();
        while (qt >= NUMBER_OF_THREADS);
    }
    while (qt);
//    goto ETERNAL_LOOP;
    output.close();
    return 0;
}

