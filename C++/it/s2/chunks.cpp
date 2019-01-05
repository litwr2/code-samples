#include "chunks.h"
#include "base/Log.h"
#include "base/ChunkHelper.h"

namespace base {

Chunks::Chunks(std::string exchange, const base::MonitorExchangeConfig &cfg) {
    this->chunkSize = 1;
    if (exchange == "yobit")
        this->chunkSize = 36;
    if (exchange == "liqui")
        this->chunkSize = 18;
    if (exchange == "cryptopia")
        this->chunkSize = 5;
    if (exchange == "bibox")
        this->chunkSize = cfg.chunkSize;

    this->chunkTokenSize = 1;
    if (exchange == "yobit")
        this->chunkTokenSize = 6;
    if (exchange == "liqui")
        this->chunkTokenSize = 3;

    this->chunkType = CHUNK_TYPE_PAIR;
    if (exchange == "yobit" || exchange == "liqui" || exchange == "cryptopia") {
        this->chunkType = CHUNK_TYPE_TOKEN;
        if (cfg.cryptopiaShortScan) {
            this->chunkType = CHUNK_TYPE_PAIR;
            this->chunkSize = 1;
        }
    }
}

void Chunks::init(std::map<std::string, int> fastTokens, std::vector <std::string> initWithPairs, bool fastTokensOnly,
                  std::vector <std::string> bases) {
    throw "Not implemented";
//    this->chunks = {};
//    this->chunkIndex = 0;
//
//    if (initWithPairs.size()) {
//        this->chunks = base::array_chunk(initWithPairs, this->chunkSize);
//    } else {
//        auto markets = this->monitor.caches.at(this->exchange)->getPairsModels();
//        // $tokens = ArrayHelper::getColumn($markets, "c1"); // base
//        std::vector<std::string> tokens;
//        for (auto it = markets.begin(); it != markets.end(); ++it) {
//            if (!fastTokensOnly || fastTokens.find(it->second.base) != fastTokens.end())
//                tokens.push_back(it->second.base);
//        }
//        // $tokens = base::array_unique($tokens);
//        tokens = base::array_unique(tokens);
//        if (bases.size() == 0)
//            bases = this->cache.getBaseCurrencies();
//
//        if (fastTokens.size()) {
//            std::vector<std::string> withFastTokens;
//            for (auto it = tokens.begin(); it != tokens.end(); ++it) {
//                int mult = 1;
//                if (fastTokens.find(*it) != fastTokens.end()) {
//                    mult = fastTokens[*it];
//                } else if (std::find(bases.begin(), bases.end(), *it) != bases.end()) {
//                    // scan base currencies 10 times faster
//                    mult = 10;
//                }
//                for (int i = 0; i < mult; ++i) {
//                    withFastTokens.push_back(*it);
//                }
//            }
//            tokens = withFastTokens;
//        }
//        // shuffle($tokens);
//        base::shuffleVector(tokens);
//
//        if (this->chunkType == CHUNK_TYPE_TOKEN) {
//            auto tokenChunks = base::array_chunk(tokens, this->chunkTokenSize);
//            for (auto it = tokenChunks.begin(); it != tokenChunks.end(); it++)
//                this->chunks.push_back(this->cache.getPairsByTokens(*it));
//        } else {
//            this->chunks = base::array_chunk(this->cache.getPairsByTokens(tokens, bases), this->chunkSize);
//        }
//    }
}



std::vector<std::string> Chunks::nextChunk() {
    if (!this->chunks.size()) {
        base::Log::log(LOG_LEVEL_WARNING, std::string("MonitorInstance: chunks not loaded"));
        throw "chunks not loaded";
    }
    std::vector<std::string> ret = this->chunks[this->chunkIndex];
    this->chunkIndex++;
    this->chunkIndex = this->chunkIndex % this->chunks.size();
    return ret;
}


}