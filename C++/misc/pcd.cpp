#include <unistd.h>
#include <signal.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cmath>
using namespace std;

int pipefd[2], retst = 1;
pid_t cpid;

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
   Producer();
   int produce();
   ~Producer();
};

Producer::Producer() {
   counter = 0;
   close(pipefd[0]);
}

Producer::~Producer() {
   close(pipefd[1]);
}

int Producer::produce() {
   unsigned long n = rand();
   counter++;
   write(pipefd[1], &n, sizeof(n));
   if (counter%10000 == 0) sleep(1);
   return retst;
}

struct Consumer {
   unsigned long counter, primes;
   Consumer();
   int consume();
   ~Consumer();
};

Consumer::Consumer() {
   counter = primes = 0;
   close(pipefd[1]);
}

Consumer::~Consumer() {
   close(pipefd[0]);
}

int Consumer::consume() {
   unsigned long n;
   char buf[64];
   counter++;
   for (int i = 0; i < sizeof(n); i++)
      while (read(pipefd[0], (char*)&n + i, 1) != 1)
         if (retst == 0) return 0;
   if (prime(n)) primes++;
   if (counter%101 == 0) {
      sprintf(buf, "%10lu%11lu%7.4f", counter, primes, double(primes)/counter);
      for (int i = 0; i < 28; i++) strcat(buf, "\b");
      write(1, buf, 56);
   }
   return retst;
}

struct Dispatcher {
   pid_t pcpid, ccpid;
   Dispatcher();
   void makeProducer();
   void makeConsumer();
   ~Dispatcher();
};

Dispatcher::Dispatcher() {
   if (pipe(pipefd) == -1) {
      fputs("pipe\n", stderr);
      exit(1);
   }
   puts("press Control-C to stop this program");
   double r = log(RAND_MAX);
   printf("Approximate part of primes for this range is%7.4f\n", (1 + 1/r + 2/r/r)/r);
   makeProducer();
   makeConsumer();
}

Dispatcher::~Dispatcher() {
   puts("\nThe end");
}

void Dispatcher::makeProducer() {
   if ((cpid = pcpid = fork()) == -1) {
      fputs("fork error\n", stderr);
      exit(2);
   }
   if (pcpid != 0) return;
   Producer *p = new Producer;
   while (p->produce());
   delete p;
   exit(0);
}

void Dispatcher::makeConsumer() {
   if ((cpid = ccpid = fork()) == -1) {
      fputs("fork error\n", stderr);
      exit(2);
   }
   if (ccpid != 0) return;
   Consumer *p = new Consumer;
   while (p->consume());
   delete p;
   exit(0);
}

main() {
   signal(SIGINT, signalHandler);
   Dispatcher dispatcher;
   while (retst);
}

