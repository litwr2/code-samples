#ifndef CPPSCAN_CHUNKS_H
#define CPPSCAN_CHUNKS_H

#include <map>
#include <string>
#include <vector>
#include "MonitorExchangeConfig.h"

namespace base {

#define CHUNK_TYPE_TOKEN 1
#define CHUNK_TYPE_PAIR 0

class Chunks {
public:
    int chunkType = 0;
    int chunkSize = 0;
    int chunkTokenSize = 0;

    std::string exchange;
    std::vector<std::vector<std::string>> chunks;
    int chunkIndex = 0;

    Chunks(std::string exchange, const base::MonitorExchangeConfig &cfg);
    void init(std::map<std::string, int> fastTokens, std::vector<std::string> initWithPairs, bool fastTokensOnly, std::vector<std::string> bases);
    void init(std::map<std::string, int> pairs, int connCount);
    std::vector<std::string> nextChunk();
    std::vector<std::string> nextChunkWs(int connId);
};

}

#endif // CPPSCAN_CHUNKS_H
