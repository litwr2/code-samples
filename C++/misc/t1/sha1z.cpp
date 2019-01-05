#include <cstdlib>
#include <iostream>
#include <string>
#include <openssl/sha.h>
#include <cstring>
#include <thread>
#include <mutex>

#define x_ini "0000000"
static std::mutex mx;
static volatile int end_calc;
static const int mlx = strlen(x_ini);
static unsigned threads = std::thread::hardware_concurrency() ? std::thread::hardware_concurrency()/2 + 1 : 1;
static char r[mlx + 1];

inline void random_string(char *s) {
    int p = mlx, of = threads*7;
    while (--p >= 0)
        if ((unsigned char)s[p] >= 127 - of) {
            s[p] = 33 + s[p] - 127 + of;
            of = 1;
        }
        else {
            s[p] += of;
            break;
        }
}

static void SHA1prefix0s(const char *s, unsigned digits, unsigned id) {
    unsigned char digest[SHA_DIGEST_LENGTH];
    char x[mlx + 1] = x_ini, zeros[digits] = {'\0'};
    x[mlx - 1] += id;
    SHA_CTX ctx, ctx1;
    SHA1_Init(&ctx);
    SHA1_Update(&ctx, s, strlen((char*)s));
    ctx1 = ctx;
    while (!end_calc) {
        random_string(x);
        SHA1_Update(&ctx, x, mlx);
        SHA1_Final(digest, &ctx);
        ctx = ctx1;
        if (memcmp(zeros, digest, digits >> 1))
             continue;
        else if (digits&1) {
             if (digest[digits >> 1] < 10)
                 break;
             else
                 continue;
        }
        else
             break;
    }
    std::lock_guard<std::mutex> lk(mx);
    if (end_calc) return;
    end_calc = 1;
    strcpy(r, x);
}

char* getsuffix(const char *s, unsigned difficulty) {
    std::thread t[threads];
    for (int i = 0; i < threads; ++i)
        t[i] = std::thread(SHA1prefix0s, s, difficulty, i);
    for (int i = 0; i < threads; ++i)
        t[i].join();
    return r;
}

