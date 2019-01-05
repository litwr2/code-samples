#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cmath>
#include <queue>
using namespace std;

queue<unsigned long> qpipe;
int retst = 1;

void signalHandler(int signal) {
   retst = 0;
}

int prime(unsigned long n) {
   if (n < 2) return 0;
   for (unsigned long i = 2; i <= rint(sqrt(n)); i++)
      if (n%i == 0) return 0;
   return 1;
}

struct Producer {
   unsigned long counter;
   pthread_t thread;
   pthread_mutex_t &mr;
   Producer(pthread_mutex_t&);
   static void* produce(Producer*);
   ~Producer();
};

void* Producer::produce(Producer* self) {
   pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, 0);
   pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, 0);
   for (;;) {
      unsigned long n = rand();
      self->counter++;
      pthread_mutex_lock(&self->mr);
      qpipe.push(n);
      pthread_mutex_unlock(&self->mr);
      if (self->counter%10000 == 0) sleep(1);
   }
}

Producer::Producer(pthread_mutex_t &m): counter(0), mr(m) {
   if (pthread_create(&thread, 0, (void*(*)(void*))produce, (void*)this))
      throw "The producer thread creation error\n";
}

Producer::~Producer() {
   if (pthread_join(thread, 0)) throw "Can't join the producer thread";
}

struct Consumer {
   unsigned long counter, primes;
   pthread_t thread;
   pthread_mutex_t &mr;
   Consumer(pthread_mutex_t&);
   static void* consume(Consumer*);
   ~Consumer();
};

Consumer::Consumer(pthread_mutex_t &m): mr(m) {
   counter = primes = 0;
   if (pthread_create(&thread, 0, (void*(*)(void*))consume, (void*)this))
      throw "The consumer thread creation error\n";
}

Consumer::~Consumer() {
   if (pthread_join(thread, 0)) throw "Can't join the consumer thread";
}

void* Consumer::consume(Consumer* self) {
   char b[64];
   pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, 0);
   pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, 0);
   for (;;) {
      while (qpipe.size() == 0) if (!retst) return 0;
      pthread_mutex_lock(&self->mr);
      unsigned long n = qpipe.front();
      qpipe.pop();
      pthread_mutex_unlock(&self->mr);
      self->counter++;
      if (prime(n)) self->primes++;
      if (self->counter%1001 == 0) {
         sprintf(b, "%10lu%11lu%7.4f\r", self->counter, self->primes,
                                            double(self->primes)/self->counter);
         write(1, b, 29);
      }
   }
}

struct Dispatcher {
   pthread_mutex_t mr;
   Dispatcher();
   Producer *pp;
   Consumer *pc;
   ~Dispatcher();
};

Dispatcher::Dispatcher() {
   puts("press Control-C to stop this program");
   double r = log(RAND_MAX);
   printf("Approximate part of primes for this range is%7.4f\n", (1 + 1/r + 2/r/r)/r);
   if (pthread_mutex_init(&mr, 0)) throw "Can't create mutex";
   Producer *pp = new Producer(mr);
   Consumer *pc = new Consumer(mr);
   while (retst);
   if (pthread_cancel(pp->thread)) throw "Can't cancel the producer thread";
   if (pthread_cancel(pc->thread)) cerr << "Can't cancel the consumer thread";
   delete pp;
   delete pc;
}

Dispatcher::~Dispatcher() {
   puts("\nThe end");
}

main() {
   signal(SIGINT, signalHandler);
   try {
      Dispatcher dispatcher;
   }
   catch (const char* s) {
      puts(s);
   }
}

