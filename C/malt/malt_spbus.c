/******************************************************************************\
    MALT interconnect msg send-receive functions
\******************************************************************************/

#include <libmalt.h>

#if !LEGACY_SPB

int malt_spbus_status() {
    extern u32_t __soft_spb_fixed_data;
    if ((int)*(&__soft_spb_fixed_data + 2) != 0) return 1; //malt_trace("Clash with the stack detected!");    
    return 0;
}

u32_t malt_spbus_get_hard_limit() {
    extern u32_t __soft_spb_fixed_data;
    return *(&__soft_spb_fixed_data + 3);
}

u32_t malt_spbus_get_soft_limit() {
    extern u32_t __soft_spb_fixed_data;
    return *(&__soft_spb_fixed_data + 4);
}

void malt_spbus_set_hard_limit(u32_t n) {
    extern u32_t __soft_spb_fixed_data, __tls_end;
    *(&__soft_spb_fixed_data + 3) = (u32_t)((spbus_packet*)&__tls_end + n);
}

void malt_spbus_set_soft_limit(u32_t n) {
	extern u32_t __soft_spb_fixed_data, __tls_end;
    *(&__soft_spb_fixed_data + 4) = (u32_t)((spbus_packet*)&__tls_end + n);
}
#define MALT_SPBUS_SOFT_DISTANCE 6

/* Receive MALT packet (returns core number) */
int malt_spbus_recv_ext(spbus_packet *pack, u8_t type, u16_t core_addr)
{
    extern u32_t __soft_spb_fixed_data, __tls_end;
    spbus_packet *start_pack_ptr = (void*)*(&__soft_spb_fixed_data + 1);
    u32_t sis;
//if ((int)*(&__soft_spb_fixed_data + 2) != 0) for(;;);
    malt_save_interrupt_state(sis);
    malt_disable_interrupts();
    spbus_packet *end_pack_ptr = (void*)__soft_spb_fixed_data;
    u32_t pack_source;
    if (start_pack_ptr == end_pack_ptr) {
        malt_restore_interrupt_state(sis);
        return -1;
    }
    spbus_packet *current_pack_ptr = start_pack_ptr;
    for (;;) {
        if (current_pack_ptr == end_pack_ptr) {
            malt_restore_interrupt_state(sis);
            return -2;
        }
        if (current_pack_ptr->type == SPB_TYPE_INVALID) {
            if (current_pack_ptr++ == start_pack_ptr)
                ++start_pack_ptr;
        }
        else if ((type != SPB_TYPE_ANY && current_pack_ptr->type != type) || (core_addr != SPB_ADDR_ANY && current_pack_ptr->source != core_addr))
            ++current_pack_ptr;
        else
            break;
    }
    pack_source = current_pack_ptr->source;
    if (pack != 0)
        *pack = *current_pack_ptr;  //it is fast if the proper alignment is set
    if (start_pack_ptr == current_pack_ptr)
        *(&__soft_spb_fixed_data + 1) = (u32_t)(start_pack_ptr = ++current_pack_ptr);
    else //check with end_pack_ptr?
        current_pack_ptr->type = SPB_TYPE_INVALID;
    if (current_pack_ptr == end_pack_ptr)
        __soft_spb_fixed_data = *(&__soft_spb_fixed_data + 1) = (u32_t)&__tls_end;  //the queue is empty now
    else if (end_pack_ptr > (spbus_packet*)*(&__soft_spb_fixed_data + 3) || (end_pack_ptr > (spbus_packet*)*(&__soft_spb_fixed_data + 4) && end_pack_ptr - start_pack_ptr < MALT_SPBUS_SOFT_DISTANCE)) {
        u32_t k = 0;
        //check the collision with the stack here?
        for (u32_t i = 0; i < (u32_t*)end_pack_ptr - (u32_t*)start_pack_ptr; i += 4)
            if (((spbus_packet*)((u32_t*)start_pack_ptr + i))->type != SPB_TYPE_INVALID)
                *(spbus_packet*)(&__tls_end + i - k) = *(spbus_packet*)((u32_t*)start_pack_ptr + i); //check the alignment!
            else
                k += 4;
        *(&__soft_spb_fixed_data + 1) = (u32_t)&__tls_end; //start
        __soft_spb_fixed_data = (u32_t)(&__tls_end + ((u32_t*)end_pack_ptr - (u32_t*)start_pack_ptr) - k); //end
    }
    malt_restore_interrupt_state(sis);
    return pack_source;
}
#endif

/* Receive MALT packet 1.7 (returns core number) */
int malt_spbus_recv(spbus_packet *pack, u8_t type) //make macro!
{
#if !LEGACY_SPB
    return malt_spbus_recv_ext(pack, type, SPB_ADDR_ANY);
#else
    spbus_packet *rec_pack = (void*)malt_read_reg(BUSC_SPB_SCNT_ADDR);

    // check that the packet buffer is full
    if ((u32_t)rec_pack >= 0x7FFFFFFF) {
        return -1;
    }
    if (rec_pack->type == SPB_TYPE_INVALID || (type != SPB_TYPE_ANY && rec_pack->type != type)) {
        malt_trace_print("%s: no matched packet %p, rec_pack->type=0x%X, type=0x%X",
                         __func__, rec_pack, rec_pack->type, type);
        return -2;
    }

    if (pack != NULL)
        *pack = *rec_pack;

    rec_pack->type = SPB_TYPE_INVALID;  // mark as invalid to ignore it on the next pass
    return rec_pack->source;
#endif
}

int malt_spbus_uart_putchar(int c) {
    static malt_mutex __attribute__ ((__aligned__ (16))) mx;
    spbus_packet data;
    malt_inc_mutex_lock(&mx);
    for(;;) {
        malt_send_ic_data(SPB_ADDR_UART, 0x2, 0);
        while(malt_rec_ic_packet(&data) < 0);
        if ((data.data0&0x14000000) == 0)
            break;
//else malt_write_reg(TRACER_TRACE_ADDR, (u32_t)"BO");
    }
    malt_send_ic_data(SPB_ADDR_UART, 0x83, c);
    malt_inc_mutex_unlock(&mx);
    return c & 0xff;
}

void malt_spbus_uart_setbitrate(int b) {
    int c = malt_get_core_freq()*1000;
    c = c/b + (c%b > b >> 1);
    malt_send_ic_data(SPB_ADDR_UART, 0x81, c);
}
