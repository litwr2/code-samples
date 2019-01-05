/* *********************************************************************
 * MALTemu
 *
 * MALT SoC with MicroBlaze CPU emulator
 *
 * Created by Egor Lukyanchenko
 * Based on MBemu by Joel Yliluoma aka Bisqwit
 * *********************************************************************
 *
 * MALT memory controller class
 *
 */

#ifndef _MALTSHAREDMEMORY_HH
#define _MALTSHAREDMEMORY_HH

#include "maltemu.h"
#include "maltspbus.hh"
#include "maltram.hh"

#include <cstring>
#include <vector>
#include <algorithm>
#include <mutex>

class MALT_SoC;

const int MAX_BLKREAD_WORDS = (SMC_OP_MAX_BYTES / sizeof(u32));
const int BURST_WORDS       = 4;

// Struct to hold data with answer to DMM requests
struct McAnswer
{
    int tick;
    int length;
    int addr;
    SpbPacket packet;
};


// Struct to hold address-size pair
struct MemChunk
{
    u32 start_addr;
    u32 size;

    bool operator < (const MemChunk& mc) const
    {
        return (start_addr < mc.start_addr);
    }
};


// MALT smart shared memory controller
class MALT_SharedMemory : public RAM
{
    #if ENABLE_FE
    #define LOG2_FE_CHUNK_SIZE 4
    #define FE_CHUNK_SIZE (1 << LOG2_FE_CHUNK_SIZE)
    struct fe_t {
        bool empty;
        std::list <u32> locked_reads;
        std::list <u32> locked_rm;
        fe_t() : empty(false) {}
    };
    struct fe_chunk_t {
        fe_t* chunk[FE_CHUNK_SIZE]; // For FE_CHUNK_SIZE seq. words
        fe_chunk_t() {
            memset(chunk, 0, sizeof(chunk));
        }
        inline bool has(int i) {
            return chunk[i];
        }
        inline fe_t& operator[] (int i) {
            fe_t*& p = chunk[i];
            if (!p)
                p = new fe_t();
            return *p;
        }
    };
    // Lazy chunks/cells allocation to avoid host RAM limit
    class FE {
        fe_chunk_t** chunks;
    public:
        FE() {
            chunks = (fe_chunk_t**) calloc(RAM_SIZE/(sizeof(u32_t)*FE_CHUNK_SIZE), sizeof(fe_chunk_t*));  
        }
        inline bool has(int i) {
            int hi = i >> LOG2_FE_CHUNK_SIZE, lo = i&(FE_CHUNK_SIZE - 1);
            return chunks[hi] && chunks[hi]->has(lo);
        }
        inline fe_t& operator[](int i) {
            int hi = i >> LOG2_FE_CHUNK_SIZE, lo = i&(FE_CHUNK_SIZE - 1); 
            fe_chunk_t*& p = chunks[hi];
            if (!p) 
                p = new fe_chunk_t();
            return (*p)[lo]; 
        }
    } fe;
    #endif //ENABLE_FE

    bool watch_enabled;
    u32 watch_addr;
    int tick;

    std::list <SpbPacket> spb_requests;
    std::list <McAnswer>  answer_queue;
    
    u64 global_read_cnt;
    u64 global_write_cnt;
    u64 burst_read_cnt;
    u64 blockread_cnt[MAX_BLKREAD_WORDS];

    MALT_SoC& soc;

    #if ENABLE_MEMVALIDATION
    std::vector<MemChunk> invalid_mem;
    #endif

public:
    std::mutex mx, mx_fe;

    MALT_SharedMemory(MALT_SoC& malt_soc) : 
        RAM(RAM_SIZE, "shared"), soc(malt_soc) {}

    u32 Read(u32 index, u32 core = 0xFFFF, bool exec = false, bool block = false);
    #if ENABLE_FE
    u32 ReadModify(u32 index, u32 core, bool& locked);
    void WriteFill(u32 index, u32 value, u32 core, std::list<u32>& unlocked, u32& rm_len);
    #endif
    void Write(u32 index, u32 value, u32 core = 0xFFFF);

    #if ENABLE_MEMVALIDATION
    bool ValidateAccess(u32 addr);
    void SetInvalidMem(u32 addr, u32 size);
    void RmInvalidMem(u32 addr);
    #endif
    void WatchAddress(u32 addr);

    u64 GetReadStat()  {return global_read_cnt;};
    u64 GetWriteStat() {return global_write_cnt;};
    void ReportStatus();
    void ResetCounters();

    void Tick();
    void SPbusRecv(const SpbPacket& packet);
};

#endif
