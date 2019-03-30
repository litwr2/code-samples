#include <iostream>
#include <map>
#include <vector>
#include <string>
#include <functional>
#include <exception>
#include <utility>
#include "fme.h"
#include "parse.h"

std::vector<std::string> split(std::string input, const std::string &delimiters) { //can be taken from boost
    std::vector<std::string> v;
    for (;;) {
        while (delimiters.find(input[0]) != std::string::npos)
            input = input.substr(1);
        if (input == "") return v;
        auto pos = input.find_first_of(delimiters);
        if (pos == std::string::npos) {
            v.push_back(input);
            return v;
        }
        v.push_back(input.substr(0, pos));
        input = input.substr(pos);
    }
}

std::map<std::string, std::function<bool(const std::vector<std::string> &cmd, DirTree&)>>
    lex{{"md", md}, {"mf", mf}, {"rm", rm}, {"cp", cp}, {"mv", mv}};

bool md(const std::vector<std::string> &cmd, DirTree& dirTree) {
    if (cmd.size() < 2)
        throw std::runtime_error("command has no argument");
    if (cmd.size() > 2)
        throw std::runtime_error("extra argument(s)");
    auto path = split(cmd[1], "/");
    auto p = dirTree.checkPath(path);
    if (p == DirTree::nullpair)
        throw std::runtime_error("can't create a directory, wrong path");
    if (p.first->descendants.find(path.back()) != p.first->descendants.end())
        throw std::runtime_error("attempt to overwrite an existing file or directory");
    p.first->descendants[path.back()] = new DirTree::DirTreeNode;
    return true;
}

bool mf(const std::vector<std::string> &cmd, DirTree& dirTree) {
    if (cmd.size() < 2)
        throw std::runtime_error("command has no argument");
    if (cmd.size() > 2)
        throw std::runtime_error("extra argument(s)");
    auto path = split(cmd[1], "/");
    auto p = dirTree.checkPath(path);
    if (p == DirTree::nullpair)
        throw std::runtime_error("can't create a file, wrong path");
    if (p.first->descendants.find(path.back()) != p.first->descendants.end())
        if (p.first->descendants[path.back()] != nullptr)
            throw std::runtime_error("attempt to overwrite an existing directory");
        else
            return false;
    p.first->descendants[path.back()] = nullptr;
    return true;
}

bool rm(const std::vector<std::string> &cmd, DirTree& dirTree) {
    if (cmd.size() < 2)
        throw std::runtime_error("command has no argument");
    if (cmd.size() > 2)
        throw std::runtime_error("extra argument(s)");
    auto path = split(cmd[1], "/");
    auto p = dirTree.checkPath(path, 0);
    if (p == DirTree::nullpair)
        throw std::runtime_error("no such file or directory");
    deleteDir(p.first);
    p.second->descendants.erase(path.back());
    return true;
}

bool cp(const std::vector<std::string> &cmd, DirTree& dirTree) {
    if (cmd.size() < 3)
        throw std::runtime_error("command has no enough arguments");
    if (cmd.size() > 3)
        throw std::runtime_error("extra argument(s)");
    auto path1 = split(cmd[1], "/");
    auto path2 = split(cmd[2], "/");
    if (path1 == path2) return true; //error?
    auto p1 = dirTree.checkPath(path1, 0);
    if (p1 == DirTree::nullpair)
        throw std::runtime_error("1st arg - no such file or directory");
    auto p2 = dirTree.checkPath(path2);
    if (p2 == DirTree::nullpair)
        throw std::runtime_error("2nd arg - no such directory");
    if (checkFile(path2, dirTree) && checkFile(path1, dirTree)) {
        mf({"mf", cmd[2]}, dirTree);
        return true;
    }
    if (!checkFile(path2, dirTree) && checkFile(path1, dirTree)) {
        mf({"mf", cmd[2] + "/" + path1.back()}, dirTree);
        return true;
    }
    if (checkFile(path2, dirTree) && !checkFile(path1, dirTree))
        throw std::runtime_error("can't copy a directory to a file");
    p2 = dirTree.checkPath(path2, 0);
    auto it = p2.first->descendants.find(path1.back());
    if (it != p2.first->descendants.end() && it->second == nullptr)
        throw std::runtime_error("can't copy a directory to a file");
    auto pn = new DirTree::DirTreeNode;
    copyDir(p1.first, pn);
    p2.first->descendants[path1.back()] = pn;
    return true;
}

bool mv(const std::vector<std::string> &cmd, DirTree& dirTree) {
if (cmd.size() < 3)
        throw std::runtime_error("command has no enough arguments");
    if (cmd.size() > 3)
        throw std::runtime_error("extra argument(s)");
    auto path1 = split(cmd[1], "/");
    auto path2 = split(cmd[2], "/");
    if (path1 == path2) return true; //error?
    auto p1 = dirTree.checkPath(path1, 0);
    if (p1 == DirTree::nullpair)
        throw std::runtime_error("1st arg - no such file or directory");
    auto p2 = dirTree.checkPath(path2);
    if (p2 == DirTree::nullpair)
        throw std::runtime_error("2nd arg - no such directory");
    if (checkFile(path2, dirTree) && checkFile(path1, dirTree)) {
        mf({"mf", cmd[2]}, dirTree);
        rm({"rm", cmd[1]}, dirTree);
        return true;
    }
    if (!checkFile(path2, dirTree) && checkFile(path1, dirTree)) {
        mf({"mf", cmd[2] + "/" + path1.back()}, dirTree);
        rm({"rm", cmd[1]}, dirTree);
        return true;
    }
    if (checkFile(path2, dirTree) && !checkFile(path1, dirTree))
        throw std::runtime_error("can't move a directory to a file");
    p2 = dirTree.checkPath(path2, 0);
    auto it = p2.first->descendants.find(path1.back());
    if (it != p2.first->descendants.end() && it->second == nullptr)
        throw std::runtime_error("can't move a directory to a file");
    auto pn = new DirTree::DirTreeNode;
    copyDir(p1.first, pn);
    p2.first->descendants[path1.back()] = pn;
    rm({"rm", cmd[1]}, dirTree);
    return true;
}

int parse(DirTree &dirTree) {
    std::string input;
    do {
        std::getline(std::cin, input);
        auto cmd = split(input, " \t");
        if (cmd.size() == 0) return 1;
        if (lex.find(cmd[0]) == lex.end())
            throw std::runtime_error("wrong command");
        if (lex[cmd[0]](cmd, dirTree) == false) {
            //throw std::runtime_error("command failed");
        }
    }
    while (std::cin);
    return 1;
}

