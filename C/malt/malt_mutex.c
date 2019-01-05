/******************************************************************************\
    MALT mutex functions
\******************************************************************************/
//#define ENABLE_TRACE_LIB
//#define ENABLE_TRACE
#include <libmalt.h>

#define INC_MUTEX_VARIATION 1

void malt_inc_mutex_lock(malt_mutex *mx) {
    malt_trace_func();
#if INC_MUTEX_VARIATION == 0
    while (malt_ext_memop_inc(&mx->mx, 1) > 1)
        while (((volatile malt_mutex*)mx)->mx > 1);
#else
    while (malt_ext_memop_inc(&mx->mx, 1) != 1) {
        malt_ext_memop_inc(&mx->mx, -1);
        while (((volatile malt_mutex*)mx)->mx != 0);
    }
#endif
}

void malt_inc_mutex_unlock(malt_mutex *mx) {
    malt_trace_func();
#if INC_MUTEX_VARIATION == 0
    mx->mx = 0;
#else
    malt_ext_memop_inc(&mx->mx, -1);
#endif
}

void malt_inc_recursive_mutex_lock(malt_mutex *mx) {
    malt_trace_func();
    if (malt_cur_core_num() + 1 == mx->owner) {
        ++mx->counter;
        return;
    }
    malt_inc_mutex_lock(mx);
    mx->owner = malt_cur_core_num() + 1;
}

void malt_inc_recursive_mutex_unlock(malt_mutex *mx) {
    malt_trace_func();
    if (mx->counter) {
        --mx->counter;
        return;
    }
    mx->owner = 0;
    malt_inc_mutex_unlock(mx);
}

void malt_fe_mutex_lock(malt_mutex *mx) {
    malt_trace_func();
    malt_read_modify(mx);
};

void malt_fe_mutex_unlock(malt_mutex *mx) {
    malt_trace_func();
    malt_fill_write(mx, 0);
};

void malt_nohw_mutex_lock(malt_mutex *mx) {
    int wait, cpu = malt_cur_core_num() + 1;
    malt_trace_func();
    do {
        while (((volatile malt_mutex*)mx)->mx != 0);
        wait = 0x1F & (3 + cpu)*77;
        while (wait-- && ((volatile malt_mutex*)mx)->mx == 0);
        if (((volatile malt_mutex*)mx)->mx == 0) {
            mx->mx = cpu;
            wait = 0x1F;
            while (--wait && ((volatile malt_mutex*)mx)->mx == cpu);
        }
    }
    while (((volatile malt_mutex*)mx)->mx != cpu);
}

void malt_nohw_mutex_unlock(malt_mutex *mx) {
    malt_trace_func();
    mx->mx = 0;
}

void malt_nohw_recursive_mutex_lock(malt_mutex *mx) {
    int cpu = malt_cur_core_num() + 1;
    malt_trace_func();
    if (cpu == mx->owner) {
        ++mx->counter;
        return;
    }
    malt_nohw_mutex_lock(mx);
    mx->owner = cpu;
}

void malt_nohw_recursive_mutex_unlock(malt_mutex *mx) {
    malt_trace_func();
    if (mx->counter) {
        --mx->counter;
        return;
    }
    mx->owner = 0;
    mx->mx = 0;
}

//
// Generated by GCC when atomics ops are required
//

static malt_mutex _am;

int __atomic_fetch_add_4(int *obj, int op, int memmodel) {
    malt_nohw_mutex_lock(&_am);
    unsigned v = *obj;
    *obj += op;
    malt_nohw_mutex_unlock(&_am);
    return v;
}

int __atomic_fetch_sub_4(int *obj, int op, int memmodel) {
    malt_nohw_mutex_lock(&_am);
    unsigned v = *obj;
    *obj -= op;
    malt_nohw_mutex_unlock(&_am);
    return v;
}

int extern_is_one(int x) { return x==1; }
 
