/******************************************************************************\
    MALT interconnect thread control functions
\******************************************************************************/

#include <stdarg.h>
#include <stdlib.h>
#include <libmalt.h>

#define ALWAYS_USE_GLOBAL_STACK 0

#if ALWAYS_USE_GLOBAL_STACK
int malt_start_thread(int(*fp)(void), u32_t iseg, u32_t dseg) {
    return malt_start_thr_gl_stack(fp, 0);
#else
/* Starts first avaliable slave core */
int malt_start_thread(int(*fp)(void), u32_t iseg, u32_t dseg)
{
    int i, core;

    for(i = 1; i <= malt_get_lines_count(); ++i)
        if ((core = malt_start_thread_line(i, fp, iseg, dseg)) > 0)
            return core;

    return -1;
#endif
}

#define CORE_GLOBAL_STACK_SZ 16384

/* Starts a function execution which uses the global memory as its stack */
static int start_thr_gl_stack_helper(void) {
//    static malt_mutex mx;
    spbus_packet data;
    void *p;
    u32_t saved_sp;

    malt_trace_func();
    while (malt_rec_ic_packet(&data) < 0);
//    malt_inc_mutex_lock(&mx);
    p = malloc(data.data1);
//    malt_inc_mutex_unlock(&mx);
#if 0
//Use only the serial calls to it!!!
    static volatile u32_t gfp;
    gfp = data.data0;
    asmi("  sw r1,r0,%0\n\t\
            addk r1,r0,%1"::"r"(&saved_sp),"r"((char*)p + data.data1):);
    (*(void(*)(void))gfp)();
    asmi("  lw r1,r0,%0"::"r"(&saved_sp):);
#else
    asmi("  sw r1,r0,%0\n\t\
            addk r1,r0,%1\n\t\
            sw %0,r0,r1\n\t\
            swi r20,r1,4\n\t\
            brald r15,%2\n\t\
            nop\n\t\
            lwi r20,r1,4\n\t\
            lw %0,r0,r1\n\t\
            lw r1,r0,%0"::"r"(&saved_sp),"r"((char*)p + data.data1 - 8),"r"(data.data0):"r3", "r4", "r5", "r6", "r7", "r8", "r9", "r10", "r11","r12"); //are there more registers to clobber?
#endif
//    malt_inc_mutex_lock(&mx);
    free(p);
//    malt_inc_mutex_unlock(&mx);
    return 0;
}

/* Starts a first avaliable slave core using global memory as the stack  */
int malt_start_thr_gl_stack(void* fp, unsigned stack_sz) {
    int i, core = -1;

    malt_trace_func();
    for(i = 1; i <= malt_get_lines_count(); ++i)
        if ((core = malt_start_thread_line(i, start_thr_gl_stack_helper, 0, 0)) > 0)
            break;
    if (core < 0)
        return -1;
    malt_send_ic_data(core, (u32_t)fp, stack_sz == 0 ? CORE_GLOBAL_STACK_SZ : stack_sz);
    return core;
}


int malt_start_thr_helper0(void) {
    spbus_packet data;

    malt_trace_func();
    while (malt_rec_ic_packet(&data) < 0);
    (*(void(*)(void))data.data0)();
    
    return 0;
}

int malt_start_thr_helper1(void) {
    void *fp;
    spbus_packet data;
    
    malt_trace_func();
    while (malt_rec_ic_packet(&data) < 0);
    fp = (void*) data.data0;
    while (malt_rec_ic_packet(&data) < 0);
    (*(void(*)(void*))fp)((void*)data.data0);
    
    return 0;
}

int malt_start_thr_helper2(void) {
    void* arg0, *fp;
    spbus_packet data;
    
    malt_trace_func();
    while (malt_rec_ic_packet(&data) < 0);
    fp = (void*) data.data0;
    while (malt_rec_ic_packet(&data) < 0);
    arg0 = (void*)data.data0;
    while (malt_rec_ic_packet(&data) < 0);
    (*(void(*)(void*, void*))fp)(arg0, (void*)data.data0);
    
    return 0;
}

int malt_start_thr_helper3(void) {
    void* args[2], *fp;
    spbus_packet data;

    malt_trace_func();
    while (malt_rec_ic_packet(&data) < 0);
    fp = (void*) data.data0;
    for (u32_t i = 0; i < 2; i++) {
        while (malt_rec_ic_packet(&data) < 0);
        args[i] = (void*)data.data0;
    }
    while (malt_rec_ic_packet(&data) < 0);
    (*(void(*)(void*, void*, void*))fp)(args[0], args[1], (void*)data.data0);
    
    return 0;
}

int malt_start_thr_helper4(void) {
    void* args[3], *fp;
    spbus_packet data;
    
    malt_trace_func();
    while (malt_rec_ic_packet(&data) < 0);
    fp = (void*) data.data0;
    for (u32_t i = 0; i < 3; i++) {
        while (malt_rec_ic_packet(&data) < 0);
        args[i] = (void*)data.data0;
    }
    while (malt_rec_ic_packet(&data) < 0);
    (*(void(*)(void*, void*, void*, void*))fp)(args[0], args[1], args[2], (void*)data.data0);
    
    return 0;
}

int malt_start_thr_helper5(void) {
    void* args[4], *fp;
    spbus_packet data;
    
    malt_trace_func();
    while (malt_rec_ic_packet(&data) < 0);
    fp = (void*) data.data0;
    for (u32_t i = 0; i < 4; i++) {
        while (malt_rec_ic_packet(&data) < 0);
        args[i] = (void*)data.data0;
    }
    while (malt_rec_ic_packet(&data) < 0);
    (*(void(*)(void*, void*, void*, void*, void*))fp)(args[0], args[1], args[2], args[3], (void*)data.data0);
    
    return 0;
}

int malt_start_thr_helper6(void) {
    void* args[5], *fp;
    spbus_packet data;
    
    malt_trace_func();
    while (malt_rec_ic_packet(&data) < 0);
    fp = (void*) data.data0;
    for (u32_t i = 0; i < 5; i++) {
        while (malt_rec_ic_packet(&data) < 0);
        args[i] = (void*)data.data0;
    }
    while (malt_rec_ic_packet(&data) < 0);
    (*(void(*)(void*, void*, void*, void*, void*, void*))fp)(args[0], args[1], args[2], args[3], args[4], (void*)data.data0);
    
    return 0;
}

int malt_start_thr_helper7(void) {
    void* args[6], *fp;
    spbus_packet data;
    
    malt_trace_func();
    while (malt_rec_ic_packet(&data) < 0);
    fp = (void*) data.data0;
    for (u32_t i = 0; i < 6; i++) {
        while (malt_rec_ic_packet(&data) < 0);
        args[i] = (void*)data.data0;
    }
    while (malt_rec_ic_packet(&data) < 0);
    (*(void(*)(void*, void*, void*, void*, void*, void*, void*))fp)(args[0],
        args[1], args[2], args[3], args[4], args[5], (void*)data.data0);
        
    return 0;
}

int malt_start_thr_helper8(void) {
    void* args[7], *fp;
    spbus_packet data;
    
    malt_trace_func();
    while (malt_rec_ic_packet(&data) < 0);
    fp = (void*) data.data0;
    for (u32_t i = 0; i < 7; i++) {
        while (malt_rec_ic_packet(&data) < 0);
        args[i] = (void*)data.data0;
    }
    while (malt_rec_ic_packet(&data) < 0);
    (*(void(*)(void*, void*, void*, void*, void*, void*, void*, void*))fp)(args[0],
        args[1], args[2], args[3], args[4], args[5], args[6], (void*)data.data0);
    
    return 0;
}

#if TASIC
#define malt_start_thr_tasic_helper(n) \
    void malt_start_thr_tasic_helper##n() {tasic_slave_stack_fix(); malt_start_thr_helper##n(); tasic_return();}

int malt_start_thr_tasic_helper0();
int malt_start_thr_tasic_helper1();
int malt_start_thr_tasic_helper2();
int malt_start_thr_tasic_helper3();
int malt_start_thr_tasic_helper4();
int malt_start_thr_tasic_helper5();
int malt_start_thr_tasic_helper6();
int malt_start_thr_tasic_helper7();
int malt_start_thr_tasic_helper8();

#define START_THR_HELPER_FUNC(n) malt_start_thr_tasic_helper##n

#else
#define START_THR_HELPER_FUNC(n) malt_start_thr_helper##n
#endif


/* Starts a first avaliable slave core with given parameters */
int _malt_start_thr(void* fp, u32_t n, ...) {
    int i, core = -1;
    va_list p;

    malt_trace_func();
#define caseline(n) case n: for(i = 1; i <= malt_get_lines_count(); ++i) if ((core = malt_start_thread_line(i, START_THR_HELPER_FUNC(n), 0, 0)) > 0) break; break
    switch (n) {
        caseline(0);
        caseline(1);
        caseline(2);
        caseline(3);
        caseline(4);
        caseline(5);
        caseline(6);
        caseline(7);
        caseline(8);
    }
#undef caseline
    if (core < 0)
        return -1;
    malt_send_ic_data(core, (u32_t)fp, n);
    va_start(p, n);
    for (int i = 0; i < n; i++)
        malt_send_ic_data(core, va_arg(p, u32_t), 0);
    va_end(p);
    return core;
}

