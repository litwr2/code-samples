#include <iostream>
#include <map>
#include <vector>
#include <string>
#include <functional>
#include <exception>
#include <memory>
#include <utility>
#include "fme.h"
#include "parse.h"

namespace parser {

std::vector<std::string> split(std::string input, const std::string &delimiters) { //can be taken from boost
//for general path format it has to be specialized to work with leading (root) slashes
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
        throw std::runtime_error("md command has no argument");
    if (cmd.size() > 2)
        throw std::runtime_error("md - extra argument(s)");
    auto path = split(cmd[1], "/");
    auto p = dirTree.checkPath(path);
    if (p == DirTree::nullpair)
        throw std::runtime_error("can't create a directory, wrong path - " + cmd[1]);
    if (p.first->descendants.find(path.back()) != p.first->descendants.end())
        throw std::runtime_error("attempt to overwrite an existing file or directory - " + cmd[1]);
    p.first->descendants[path.back()] = new DirTreeNode;
    return true;
}

bool mf(const std::vector<std::string> &cmd, DirTree& dirTree) {
    if (cmd.size() < 2)
        throw std::runtime_error("mf command has no argument");
    if (cmd.size() > 2)
        throw std::runtime_error("mf - extra argument(s)");
    auto path = split(cmd[1], "/");
    auto p = dirTree.checkPath(path);
    if (p == DirTree::nullpair)
        throw std::runtime_error("can't create a file, wrong path - " + cmd[1]);
    if (p.first->descendants.find(path.back()) != p.first->descendants.end())
        if (p.first->descendants[path.back()] != nullptr)
            throw std::runtime_error("attempt to overwrite an existing directory -" + cmd[1]);
        else
            return false;
    p.first->descendants[path.back()] = nullptr;
    return true;
}

bool rm(const std::vector<std::string> &cmd, DirTree& dirTree) {
    if (cmd.size() < 2)
        throw std::runtime_error("rm command has no argument");
    if (cmd.size() > 2)
        throw std::runtime_error("rm - extra argument(s)");
    auto path = split(cmd[1], "/");
    if (path.empty())
        throw std::runtime_error("can't remove the root directorty");
    auto p = dirTree.checkPath(path, 0);
    if (p == DirTree::nullpair)
        throw std::runtime_error("no such file or directory - " + cmd[1]);
    DirTree::deleteDir(p.first);
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
        throw std::runtime_error("1st arg - no such file or directory - " + cmd[1]);
    auto p2 = dirTree.checkPath(path2);
    if (p2 == DirTree::nullpair)
        throw std::runtime_error("2nd arg - no such directory - " + cmd[2]);
    if (dirTree.checkFile(path2) && dirTree.checkFile(path1)) {
        mf({"mf", cmd[2]}, dirTree);
        return true;
    }
    if (!dirTree.checkFile(path2) && dirTree.checkFile(path1)) {
        mf({"mf", cmd[2] + "/" + path1.back()}, dirTree);
        return true;
    }
    if (dirTree.checkFile(path2) && !dirTree.checkFile(path1))
        throw std::runtime_error("can't copy a directory to a file - " + cmd[2]);
    p2 = dirTree.checkPath(path2, 0);
    if (!path1.empty()) {
        auto it = p2.first->descendants.find(path1.back());
        if (it != p2.first->descendants.end() && it->second == nullptr)
            throw std::runtime_error("can't copy a directory to a file - " + cmd[2]);
    }
    auto pn = new DirTreeNode;
    DirTree::copyDir(p1.first, pn);
    if (path1.empty()) {
        p2.first->descendants.insert(pn->descendants.begin(), pn->descendants.end());
        delete pn;  //handles the root directory
    }
    else
        p2.first->descendants[path1.back()] = pn;
    return true;
}

bool mv(const std::vector<std::string> &cmd, DirTree& dirTree) {
if (cmd.size() < 3)
        throw std::runtime_error("mv command has no enough arguments");
    if (cmd.size() > 3)
        throw std::runtime_error("mv - extra argument(s)");
    auto path1 = split(cmd[1], "/");
    auto path2 = split(cmd[2], "/");
    if (path1 == path2) return true; //error?
    auto p1 = dirTree.checkPath(path1, 0);
    if (p1 == DirTree::nullpair)
        throw std::runtime_error("1st arg - no such file or directory - " + cmd[1]);
    auto p2 = dirTree.checkPath(path2);
    if (p2 == DirTree::nullpair)
        throw std::runtime_error("2nd arg - no such directory - " + cmd[1]);
    if (dirTree.checkFile(path2) && dirTree.checkFile(path1)) {
        mf({"mf", cmd[2]}, dirTree);
        rm({"rm", cmd[1]}, dirTree);
        return true;
    }
    if (!dirTree.checkFile(path2) && dirTree.checkFile(path1)) {
        mf({"mf", cmd[2] + "/" + path1.back()}, dirTree);
        rm({"rm", cmd[1]}, dirTree);
        return true;
    }
    if (dirTree.checkFile(path2) && !dirTree.checkFile(path1))
        throw std::runtime_error("can't move a directory to a file - " + cmd[1]);
    p2 = dirTree.checkPath(path2, 0);
    if (!path1.empty()) {
        auto it = p2.first->descendants.find(path1.back());
        if (it != p2.first->descendants.end() && it->second == nullptr)
            throw std::runtime_error("can't move a directory to a file - " + cmd[1]);
    }
    auto pn = new DirTreeNode;
    DirTree::copyDir(p1.first, pn);
    if (path1.empty())
        throw std::runtime_error("can't move an upper directory to a lower level (" + cmd[1] + " to " + cmd[2] + ")");
    else
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

}

