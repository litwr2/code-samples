/*
2. TASK 2

Given a collection of intervals, merge all overlapping intervals.

For example,

Given [1,3],[8,10],[2,6],[15,18],[18,21]

return [1,6],[8,10],[15,21].
*/

#include <map>
#include <string>
#include <iostream>

std::map<int, int> task(std::multimap<int, int> m) {
    std::map<int, int> r;
    for (auto it2 = m.begin(); it2 != m.end(); it2++) {
        r[it2->first] = it2->second;
        for (auto it = std::next(it2); it != m.end(); it++)
            if (r.rbegin()->second >= it->first) {
                if (r.rbegin()->second < it->second)
                    r.rbegin()->second = it->second;
                m.erase(it--);
            }
    }
    return r;
}

int main() {
    std::multimap<int, int> m{{1,3}, {8,10}, {2,6}, {15,18}, {18,21}};
    for (auto i: task(m)) std::cout << "[" << i.first << ", " << i.second << "]\n"; 
    return 0;
}

