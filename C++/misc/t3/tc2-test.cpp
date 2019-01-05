#include <map>
#include <iterator>
#include <vector>
#include <limits>
#include <iostream>
#include <cstdlib>

#define H 'A'

#if 0
#define A 10000
#define C 10000
#define N 5
#define S 11
#endif

std::vector<char> s;

void prints() {
    char c = H;
    std::cout << std::numeric_limits<int>::lowest() << '-';
    for(int i = 0; i < A; ++i) {
        if (s[i] == c) continue;
        std::cout << i << ':' << c << std::endl;
        c = s[i];
        std::cout << i << '-';
    }
    std::cout << "end:" << c << std::endl;
#if 0
    auto k = s.begin();
    std::cout << H;
    for (int i = 0; i < 10000; ++i) {
        ++k;
        if (i > 0 && i < 40 && *k != H) std::cout << *k;
    }
    std::cout << H << "\n\n";
#endif
}

void s_assign(int a, int b, char c) {
   for (int i = a; i < b; i++) s[i] = c;
}

int main() {
    for (int i = 0; i < A; ++i) s.push_back(H);
#if 0
    prints();
    s_assign(3, 17, 'B');
    prints();
    s_assign(4, 15, 'A');
    prints();
    s_assign(6, 12, 'C');
    prints();
    s_assign(8, 10, 'B');
    prints();
    s_assign(7, 11, 'C');
    prints();
#else
    srand(S);
    for (int i = 0; i < C; i++) {
       int a = rand()%(A - 2) + 1;
       int b = a + rand()%(A - a - 1) + 1;
       int c = rand()%N + 'A';
       s_assign(a, b, c);
    }
    prints();
#endif
    return 0;
}

