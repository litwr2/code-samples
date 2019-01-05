#include <map>
#include <iterator>
#include <limits>
#include <iostream>

#define H 'A'

#if 0
#define A 10000
#define C 10000
#define N 5
#define S 11
#endif

template<typename K, typename V>
class interval_map {
    std::map<K, V> m_map;

    // it is an alternative way to establish the cannonical format of the interval map
    void fix_intervals() {
        auto p = m_map.begin();
        for(auto i = std::next(p); i != m_map.end();)
           if (p->second == i->second)
              i = m_map.erase(i);
           else
              p = i++;
    }
public:
    // constructor associates whole range of K with val by inserting (K_min, val)
    // into the map
    interval_map(V const& val) {
        m_map.insert(m_map.end(), std::make_pair(std::numeric_limits<K>::lowest(), val));
    }

    // Assign value val to interval [keyBegin, keyEnd).
    // Overwrite previous values in this interval.
    // If !( keyBegin < keyEnd ), this designates an empty interval,
    // and assign must do nothing.
    void assign(K const& keyBegin, K const& keyEnd, V const& val) {
        if (!(keyBegin < keyEnd)) return;
        auto p = m_map.lower_bound(keyBegin);
        V v = std::prev(p)->second, v1 = v;
        while (p != m_map.end() && !(keyEnd < p->first))
            v1 = p->second, p = m_map.erase(p);
        if (!(v1 == val))
            p = m_map.insert(p, std::make_pair(keyEnd, v1));
        if (!(v == val))
            m_map.insert(p, std::make_pair(keyBegin, val));
    }

    // look-up of the value associated with key
    V const& operator[]( K const& key ) const {
        return ( --m_map.upper_bound(key) )->second;
    }

    typename std::map<K, V>::iterator begin() {
        return m_map.begin();
    }

    typename std::map<K, V>::iterator end() {
        return m_map.end();
    }
};

interval_map<int, char> m(H);

void printm() {
    for(auto i = m.begin(); i != m.end(); ++i)
        if (std::next(i) != m.end())
            std::cout << i->first << '-' << std::next(i)->first << ':' << i->second << std::endl;
        else
            std::cout << i->first << "-end:" << i->second << std::endl;
#if 0
    std::cout << "*";
    for (int i = 0; i < 20; ++i)
        if (m[i] != '*')
           std::cout << m[i];
    std::cout << "*\n\n";
#endif
}

int main() {
#if 0
    printm();
    m.assign(3, 17, 'B');
    printm();
    m.assign(4, 15, 'A');
    printm();
    m.assign(6, 12, 'C');
    printm();
    m.assign(8, 10, 'B');
    printm();
    m.assign(7, 11, 'C');
    printm();
#else
    srand(S);
    for (int i = 0; i < C; i++) {
       int a = rand()%(A - 2) + 1;
       int b = a + rand()%(A - a - 1) + 1;
       int c = rand()%N + 'A';
       m.assign(a, b, c);
    }
    printm();
#endif
    return 0;
}

