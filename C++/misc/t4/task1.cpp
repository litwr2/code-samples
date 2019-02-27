/*
1. TASK 1

Given a non-empty string s and a list wordList containing a list of non-empty words, determine if s can be segmented into a space-separated sequence of one or more dictionary words. You may assume the dictionary does not contain duplicate words.

For example, given

s = "whataniceday",

wordList = ["a", "what", "an", "nice", "day"].

Return true because "whataniceday" can be segmented as "what a nice day".

Words from the dictionary may be used multiple times.
*/

#include <set>
#include <string>
#include <iostream>

bool check(const std::string &s, const std::set<std::string> &wordList) {
    if (s.empty())
        return true;
    for (auto it = wordList.begin(); it != wordList.end(); it++)
       if (s.find(*it) == 0)
           if (check(s.substr(it->length()), wordList))
               return true;
    return false;
}

int main() {
    std::string s = "whataniceday";
    std::set<std::string> wordList{"a", "what", "an", "nice", "day"};
    if (check(s, wordList))
        std::cout << "the given string CAN be segmented into a space-separated sequence of one or more dictionary words\n";
    else
        std::cout << "the given string CAN'T be segmented into a space-separated sequence of one or more dictionary words\n";
    return 0;
}

