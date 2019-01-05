/* *********************************************************************
 * MALTemu
 *
 * MALT SoC with MicroBlaze CPU emulator
 *
 * Created by Egor Lukyanchenko
 * Based on MBemu by Joel Yliluoma aka Bisqwit
 * *********************************************************************
 *
 * MALT smart memory controller
 *
 */


#include "maltemu.h"
#include "maltsharedmemory.hh"

#include "maltsoc.hh"


#if ENABLE_MEMVALIDATION
bool MALT_SharedMemory::ValidateAccess(u32 addr)
{
    for (auto i = 0; i < invalid_mem.size(); ++i)
    {
        if (addr > invalid_mem[i].start_addr && addr < invalid_mem[i].start_addr+invalid_mem[i].size)
            return false;
            
        if (addr < invalid_mem[i].start_addr)
            break;
    }
            
    return true;
}


void MALT_SharedMemory::SetInvalidMem(u32 addr, u32 size)
{
    MemChunk chunk;
    chunk.start_addr = addr / 4;
    chunk.size = size / 4;
    
    invalid_mem.push_back(chunk);
    std::sort(invalid_mem.begin(), invalid_mem.end());
}


void MALT_SharedMemory::RmInvalidMem(u32 addr)
{
    addr /= 4;
    for (int i = 0; i < invalid_mem.size(); ++i)
    {
        if (addr == invalid_mem[i].start_addr)
        {
            invalid_mem.erase(invalid_mem.begin()+i);
            return;
        }
    }
    
    dbg_io_printf(1, "Address 0x%08X which was requested to remove from invalid was not found", addr*4);
    halt_on_error(2);
}
#endif

// Read shared memory
u32 MALT_SharedMemory::Read(u32 index, u32 core, bool exec, bool block)
{
    #if ENABLE_WATCH
    if (watch_enabled && watch_addr == index)
    {
        dbg_io_printf(1, "Read from watched memory (%s) address 0x%08X from core 0x%04X: 0x%08X", ram_tag.c_str(), index*4, core, memory[index]);
        //soc.GetCoreByNum(core).GetCpu().DumpRegs(true, true);
    }
    #endif
    
    #if ENABLE_MEMVALIDATION
    if (!ValidateAccess(index))
    {
        dbg_io_printf(1, "Read from invalid memory at address 0x%08X from core %08X", index*4, core);
        halt_on_error(2);
    }
    #endif
  
    #if ENABLE_FE
    if (fe.has(index) && fe[index].empty)
    {
        dbg_fe_printf(2, "Core 0x%X is trying to read empty cell\n", core);
        
        // if memory cell is empty - lock this core
        //? lock = true;
        soc.GetCoreByNum(core).GetBusCtrl().SetFELock(true);
        mx_fe.lock();
        fe[index].locked_reads.push_back(core);
        mx_fe.unlock();
        dbg_fe_printf(2, "Core %x locked on read at address 0x%X\n", core, index*4);
        return 0;
    }
    #endif // ENABLE_FE
    
    if (!exec && !block)
        ++global_read_cnt;
    
    return RAM::Read(index, core);
}


#if ENABLE_FE
u32 MALT_SharedMemory::ReadModify(u32 index, u32 core, bool &locked)
{
    dbg_fe_printf(2, "### RM to FE-memory address 0x%X from core 0x%x\n", index*4, core);

    if (index >= (size_bytes / 4))
    {
        dbg_fe_printf(2,"### RM to FE-memory address 0x%X out of bounds\n", index*4);
        return 0;
    }
    if (fe[index].empty)
    {
        mx_fe.lock();
        fe[index].locked_rm.push_back(core);
        mx_fe.unlock();
        locked = 1;
        dbg_fe_printf(2,"### Core 0x%04x locked at RM address %X\n", core, index*4);
        return 0;
    }
    else
    {
        // if the memory cell is full - empty it
        locked = 0;
        fe[index].empty = true;
    }
    return RAM::Read(index, core);
}

void MALT_SharedMemory::WriteFill(u32 index, u32 value, u32 core, std::list<u32>& unlocked, u32& rm_len)
{
    dbg_fe_printf(2, "### WF to FE-memory address 0x%X from core %x\n", index*4, core);

    if (index >= (size_bytes / 4))
    {
        dbg_io_printf(2, "### Write-fill to FE-memory address 0x%X out of bounds\n", index*4);
        return;
    }

    fe[index].empty = false;   // fill memory cell
    RAM::Write(index, value, core);

    mx_fe.lock();
    for(std::list<u32>::iterator i = fe[index].locked_reads.begin();
        i != fe[index].locked_reads.end(); ++i)
    {
        // unlock all cores locked on read...
        unlocked.push_back(*i);

        dbg_fe_printf(2, "Reading core 0x%04x unlocked from address 0x%X, got 0x%X\n", *i, index*4, S(value));
    }

    // ... and clear locked list
    fe[index].locked_reads.clear();

    rm_len = fe[index].locked_rm.size();
    if (!fe[index].locked_rm.empty())
    {
        // unlock one core locked on read-modify...
        std::list<u32>::iterator i = fe[index].locked_rm.begin();
        unlocked.push_back(*i);

        dbg_fe_printf(2, "RM core 0x%04x unlocked from address 0x%X, got 0x%X\n", *i, index*4, S(value));

        // ... and remove it from the list
        fe[index].locked_rm.erase(i);
        fe[index].empty = true;   // empty memory cell
    }
    mx_fe.unlock();
}
#endif


void MALT_SharedMemory::Write(u32 index, u32 value, u32 core)
{
    #if ENABLE_WATCH
    if (watch_enabled && watch_addr == index)
    {
        dbg_io_printf(1, "Write to watched memory (%s) address 0x%08X from core 0x%04X: 0x%08X", 
                      ram_tag.c_str(), index*4, core, S(value));
        //soc.GetCoreByNum(core).GetCpu().DumpRegs(true, true);
    }
    #endif

    #if ENABLE_MEMVALIDATION
    if (!ValidateAccess(index))
    {
        dbg_io_printf(1, "Write to invalid memory at address 0x%08X from core %08X", index*4, core);
        halt_on_error(2);
    }
    #endif  

    RAM::Write(index, value, core);

    #if ENABLE_FE 
    if (fe.has(index)) {// Do we need unlock locked cores here ?
        fe[index].empty = false;
        dbg_fe_printf(2, "Write core 0x%04x unlocked from address 0x%X", core, index*4);
    }
    #endif
        
    ++global_write_cnt;
}

void MALT_SharedMemory::WatchAddress(u32 addr) 
{
    watch_enabled = true; 
    watch_addr = (addr & MMD_ADDR_MASK & ~3) >> 2; 
    if (watch_addr > size_bytes/4)
        watch_addr = 0;
}


void MALT_SharedMemory::ResetCounters() 
{
    for (auto i = 0; i < MAX_BLKREAD_WORDS; ++i)
        blockread_cnt[i] = 0;
        
    burst_read_cnt = 0;
    global_read_cnt = 0;
    global_write_cnt = 0;
}

void MALT_SharedMemory::ReportStatus() 
{
    dbg_fe_printf(1, "Shared memory read statistics:");
    for (auto i = 0; i < MAX_BLKREAD_WORDS; ++i)
    {
        if (blockread_cnt[i] != 0)
            dbg_fe_printf(1, "%05d words: %06llu reads", 
                             i, (unsigned long long)blockread_cnt[i]);
    }
    
    dbg_fe_printf(1, "Bursts: %llu Single reads: %llu Single writes: %llu", 
        (unsigned long long)burst_read_cnt, (unsigned long long)global_read_cnt, 
        (unsigned long long)global_write_cnt);
}


/* Receive SPbus packet with request */
void MALT_SharedMemory::SPbusRecv(const SpbPacket& packet)
{
    std::lock_guard<std::mutex> lk(mx);
    spb_requests.push_back(packet);
}


/* Clock tick for memory controller */
void MALT_SharedMemory::Tick()
{
    ++tick;
    std::lock_guard<std::mutex> lk(mx);
    if (!spb_requests.empty())
    {
        // Serve pending requests to MC
        SpbPacket req = spb_requests.front();
        spb_requests.pop_front();
        
        u32 addr = req.data0;

        #if VERIFY_SMC_ALIGNMENT
        if (addr % SMC_OP_ALIGNMENT && (req.type == SMC_BLOCK_READ || req.type == SMC_MEMCPY || req.type == SMC_MEMSET || req.type == SMC_INDIRECTR)) {
            dbg_io_printf(1, "Requested SMC operation 0x%02X with analigned address 0x%08X from 0x%04X", req.type, addr, req.source);
            halt_on_error(2);
        }
        #endif
        
        McAnswer answer;
        answer.packet.type = SPB_TYPE_DMM_READ;
        answer.packet.target = req.source;
        answer.packet.source = SPB_ADDR_MCTRL;
        
        answer.length = 0;
        
        // Perform extended memory operations
        switch (req.type)
        {
            case SMC_BLOCK_READ:            // Read block from memory
                answer.length = req.add_data;
                
                dbg_fe_printf(2, "Requested SMC_BLOCK_READ len %d from 0x%08X", answer.length, addr);

                #if VERIFY_SMC_ALIGNMENT
                if (req.add_data % SMC_OP_ALIGNMENT)
                {
                    dbg_fe_printf(1, "SMC_BLOCK_READ with unaligned size 0x%08X", req.add_data);
                    halt_on_error(2);
                }
                #endif

                if (answer.length > SMC_OP_MAX_BYTES || answer.length < SMC_OP_MIN_BYTES)
                {
                    dbg_fe_printf(1, "SMC_BLOCK_READ with incorrect length %d from 0x%08X", answer.length, addr);
                    halt_on_error(2);
                }
               
                answer.tick = tick + DMM_BREAD_LAT + (DMM_PERWRD_LAT * answer.packet.len);
                answer.addr = addr;
                
                // fill data
                for (auto i = 0; i < answer.length / 4; ++i)
                   answer.packet.FillDataWord(i, Read(answer.addr / 4 + i, 0xFFFF, false, true)); 
                
                int words_read;
                words_read = (answer.length / sizeof(u32));
                ++blockread_cnt[words_read];
                burst_read_cnt += DIVUP(words_read, BURST_WORDS);
 
                break;
                
            case SMC_INCREMENT:             // Add value to memory cell content
            {
                dbg_io_printf(2, "Requested SMC_INCREMENT by %hd at 0x%08X", req.add_data, addr);
                // perform increment
                u32_t inc_result = HostRead(addr / 4) + (i16_t)req.add_data;
                HostWrite(addr / 4, inc_result);
                dbg_io_printf(3, "Increment result %d", inc_result);
                
                answer.tick = tick + DMM_BREAD_LAT;
                answer.addr = addr;
                answer.length = 4;
                
                for (auto i = 0; i < answer.length / 4; ++i)
                   answer.packet.FillDataWord(i, Read(answer.addr / 4 + i, SPB_ADDR_MCTRL, false, true)); 
                break;
            }
                
            case SMC_MEMCPY:                // Copy block in memory
                
                dbg_io_printf(2, "Requested SMC_MEMCPY from 0x%08X to 0x%08X size %d", addr, req.data1, req.add_data);

                #if VERIFY_SMC_ALIGNMENT
                if (req.add_data % SMC_OP_ALIGNMENT)
                {
                    dbg_io_printf(1, "SMC_MEMCPY with unaligned size 0x%08X", req.add_data);
                    halt_on_error(2);
                }
                #endif

                // perform copy
                for (auto i = 0; i < req.add_data / 4; ++i)
                    HostWrite(req.data1/4 + i, HostRead(addr/4 + i));

                break;
                
            case SMC_MEMSET:                // Fill block in memory
                
                dbg_io_printf(2, "Requested SMC_MEMSET of 0x%08X with 0x%08X size %d", addr, req.data1, req.add_data);
 
                #if VERIFY_SMC_ALIGNMENT
                if (req.add_data % SMC_OP_ALIGNMENT)
                {
                    dbg_io_printf(1, "SMC_MEMSET with unaligned size 0x%08X", req.add_data);
                    halt_on_error(2);
                }
                #endif
               
                // perform fill
                for (auto i = 0; i < req.add_data / 4; ++i)
                    HostWrite(addr/4 + i, req.data1); 
                break;
                
            case SMC_INDIRECTR:             // Indirect read
                
                dbg_io_printf(2, "Requested SMC_INDIRECTR of 0x%08X size %d", addr, req.add_data);

                #if VERIFY_SMC_ALIGNMENT
                if (req.add_data % SMC_OP_ALIGNMENT)
                {
                    dbg_io_printf(1, "SMC_INDIRECTR with unaligned size 0x%08X", req.add_data);
                    halt_on_error(2);
                }
                #endif
           
                answer.length = req.add_data * 4;
                answer.tick = tick + DMM_BREAD_LAT + (DMM_PERWRD_LAT * answer.packet.len);
                answer.addr = addr;
                
                // perform read
                for (auto i = 0; i < req.add_data; ++i)
                {
                    u32 indirect_addr = HostRead(addr/4 + i) & MMD_ADDR_MASK; 
                    dbg_io_printf(3, "Indirectly reading address %08X", indirect_addr);
                    answer.packet.FillDataWord(i, Read(indirect_addr / 4, 0xFFFF, false, true)); 
                }
                break;
                
            #if ENABLE_FE
            case SMC_READ_MODIFY:
            {
                // perform read-modify
                dbg_fe_printf(2, "Requested SMC_READ_MODIFY of 0x%08X by 0x%04X", addr, req.source);
                bool locked;
                u32 value = ReadModify(addr/4, req.source, locked);
                
                answer.tick = 0; 
                if (locked) 
                    answer.length = 0;
                else
                    answer.length = 4;
                answer.packet.data0 = value;                
            }
            break;
            
            case SMC_WRITE_UNBLOCK:
            {
                // perform write-fill
                dbg_fe_printf(2, "Requested SMC_WRITE_UNBLOCK of 0x%08X by 0x%04X", addr, req.source);
                std::list<u32> unlocked;
                u32 rm_len;
                WriteFill(addr/4, S(req.data1), req.source, unlocked, rm_len);
                if (rm_len) {
                    answer.packet.target = unlocked.back();
                    unlocked.pop_back();
                    answer.tick = 0; 
                    answer.length = 4;
                    answer.packet.data0 = S(req.data1);
                }
                dbg_fe_printf(2, "Unlocked read queue size = %lu\n", (unsigned long)unlocked.size());
                for(std::list<u32>::iterator i = unlocked.begin(); i != unlocked.end(); ++i)
                     soc.GetCoreByNum(*i).GetBusCtrl().SetFELock(false, req.data1);
            }
            break;
            #endif
            
            default:
                dbg_io_printf(1, "Requested unknown extended memory operation: 0x%04X", req.type);
        }
        
        
        // Check if answer is required
        if (answer.length != 0)
        {
            answer.packet.len = DIVUP((answer.length - (SPB_PACKET_BYTES-SPB_PACKET_HBYTES)), SPB_LENGTH_BYTES) + SPB_PACKET_LEN; // round up...
            answer_queue.push_back(answer);
            dbg_ic_printf(2, "Reply sent to core 0x%04x (%d bytes)\n", answer.packet.target, answer.length);
        }
        
    }
    
    if (!answer_queue.empty() && answer_queue.front().tick <= tick)
    {
        // Send reply
        McAnswer answer = answer_queue.front();
        dbg_ic_printf(3, "Sending memory reply to 0x%04X %d words in length", answer.packet.target, answer.length);
        
        soc.GetSPbusByNum(answer.packet.target).SPbusRecvLong(answer.packet);
        answer_queue.pop_front();
    }
}
