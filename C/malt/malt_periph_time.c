/******************************************************************************\
    MALT timing functions
\******************************************************************************/

#include <libmalt.h>

#define TASIC_TIMER_FREQ 500000

/*
 * Set timer interrupt period in ms
 */
void malt_set_interrupt_timer_period(u32_t period)
{
#if TASIC
    malt_write_reg(TIMER_PERIOD_ADDR, (u32_t)(period*(u64_t)malt_get_core_freq()/TASIC_TIMER_FREQ));
#else
    malt_write_reg(TIMER_PERIOD_ADDR, period);
#endif
}

/* Get 32-bit usecond counter */
u32_t malt_get_time_us()
{
    u32_t lo = malt_read_reg(TIMER_US_LO_ADDR);
#if TASIC
    return (u32_t)(lo*(u64_t)TASIC_TIMER_FREQ/malt_get_core_freq());
#else
    return lo;
#endif
}

/* Get 64-bit usecond counter */
u64_t malt_get_time_us_64()
{
    if (malt_is_smaster()) {
        u64_t hi = (u64_t)malt_read_reg(TIMER_US_HI_ADDR);
        u64_t lo = (u64_t)malt_read_reg(TIMER_US_LO_ADDR);
        if (hi != (u64_t)malt_read_reg(TIMER_US_HI_ADDR)) {
           hi = (u64_t)malt_read_reg(TIMER_US_HI_ADDR);
           lo = (u64_t)malt_read_reg(TIMER_US_LO_ADDR);
        }
#if TASIC
        return (hi*0x100000000ULL + lo)*(u64_t)TASIC_TIMER_FREQ/malt_get_core_freq();
#else
        return hi*0x100000000ULL + lo;
#endif
    }
    else {
        u64_t time;
        u32_t freq;
        malt_read_pcnt(0, 0, &time);
        freq = malt_get_core_freq();
        return (time*1000 + (freq >> 1))/freq;
    }
}

/* returns an approximation of processor time used by the program */
u64_t malt_clock() {
    extern u64_t start_clock;
    return (malt_get_time_us_64() - start_clock)/1000; // !!FIXME: remove division
}
