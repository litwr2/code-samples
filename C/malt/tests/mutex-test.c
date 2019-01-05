#include <libmalt.h>
#include <stdio.h>

#define N 1000

#define NO_MUTEX 1
#define MUTEX_INC 1
#define MUTEX_FE 1
#define MUTEX_NOHW 1
#define RECURSIVE_MUTEX_INC 1
#define RECURSIVE_MUTEX_NOHW 1

volatile int count;
malt_mutex mx;
volatile int pid;

void mainloop() {
    pid = malt_cur_core_num();
    for (int i = 0; i < N; i++)
        ++count;
    if (pid != malt_cur_core_num()) --count;
}

int thread_nomutex() {
    mainloop();
    return 0;
}

int thread_inc_mutex() {
    malt_inc_mutex_lock(&mx);
    mainloop();
    malt_inc_mutex_unlock(&mx);
    return 0;
}

int thread_fe_mutex() {
    malt_fe_mutex_lock(&mx);
    mainloop();
    malt_fe_mutex_unlock(&mx);
    return 0;
}

int thread_nohw_mutex() {
    malt_nohw_mutex_lock(&mx);
    mainloop();
    malt_nohw_mutex_unlock(&mx);
    return 0;
}

void mainrloop() {
    pid = malt_cur_core_num();
    for (int i = 0; i < N/10; i++)
        ++count;
    if (pid != malt_cur_core_num()) --count;
}

int thread_inc_recursive_mutex() {
    for (int i = 0; i < 5; i++) {
        malt_inc_recursive_mutex_lock(&mx);
        mainrloop();
    }
    for (int i = 0; i < 5; i++) {
        mainrloop();
        malt_inc_recursive_mutex_unlock(&mx);
    }
    return 0;
}

int thread_nohw_recursive_mutex() {
    for (int i = 0; i < 5; i++) {
        malt_nohw_recursive_mutex_lock(&mx);
        mainrloop();
    }
    for (int i = 0; i < 5; i++) {
        mainrloop();
        malt_nohw_recursive_mutex_unlock(&mx);
    }
    return 0;
}

int main() {
    int cores, result = 0;
#if NO_MUTEX
    cores = malt_start_all_slaves(thread_nomutex);
    malt_wait_all_lines_free();
    printf("No mutex: cores = %d, times = %d, counter = %d\n", cores, N, count);
#endif
#if MUTEX_INC
    count = 0;
    cores = malt_start_all_slaves(thread_inc_mutex);
    malt_wait_all_lines_free();
    printf("Atomic inc mutex: cores = %d, times = %d, counter = %d\n", cores, N, count);
    result |= cores*N != count;
#endif
#if MUTEX_FE
    count = 0;
    cores = malt_start_all_slaves(thread_fe_mutex);
    malt_wait_all_lines_free();
    printf("FE-bit mutex: cores = %d, times = %d, counter = %d\n", cores, N, count);
    result |= cores*N != count;
#endif
#if MUTEX_NOHW
    count = 0;
    cores = malt_start_all_slaves(thread_nohw_mutex);
    malt_wait_all_lines_free();
    printf("No special hw mutex: cores = %d, times = %d, counter = %d\n", cores, N, count);
    result |= cores*N != count;
#endif
#if RECURSIVE_MUTEX_INC
    count = 0;
    cores = malt_start_all_slaves(thread_inc_recursive_mutex);
    malt_wait_all_lines_free();
    printf("Atomic inc recursive mutex: cores = %d, times = %d, counter = %d\n", cores, N, count);
    result |= cores*N != count;
#endif
#if RECURSIVE_MUTEX_NOHW
    count = 0;
    cores = malt_start_all_slaves(thread_nohw_recursive_mutex);
    malt_wait_all_lines_free();
    printf("No special hw recursive mutex: cores = %d, times = %d, counter = %d\n", cores, N, count);
    result |= cores*N != count;
#endif
    if (result) puts("Test has failed"); else puts("All tests have passed");
    return result;
}

