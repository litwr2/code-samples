#include <libmalt.h>
#include <stdio.h>
#include <stdlib.h>

#define MALLOC_LOOPS 128
#define MALLOC_SIZE 32

void malloc_multithread_test() {
   void *p[MALLOC_LOOPS];
   for (int i = 0; i < MALLOC_LOOPS; ++i)
      p[i] = malloc(MALLOC_SIZE);
   for (int i = 0; i < MALLOC_LOOPS; ++i)
      free(p[i]);
}

int main() {
    void* p;
    puts("Starting malloc test");
    malt_start_all_slaves((void*)malloc_multithread_test);
    puts("threads are started");
    p = malloc(1024);
    free(p);
    //malloc_multithread_test();
    puts("waiting slaves...");
    malt_wait_all_lines_free();
    puts("ok");
    return 0;
}
