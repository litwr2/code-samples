#include <iostream>
#include <map>
#include <vector>
#include <string>
#include <functional>
#include <exception>
#include <utility>
#include <iterator>
#include "fme.h"
#include "parse.h"

#define PRINT_DIR_SLASH 1

const std::pair<DirTreeNode*, DirTreeNode*> DirTree::nullpair{nullptr, nullptr};

std::pair<DirTreeNode*, DirTreeNode*> DirTree::checkPath(const std::vector<std::string> &path, int skips) const {
//returns pointers to the final element found and its predecessor
    DirTreeNode *p = root, *pp = 0;
    for (int i = 0; i + skips < path.size(); ++i) {
        if (p == nullptr)
            return nullpair;
        auto it = p->descendants.find(path[i]);
        if (it == p->descendants.end())
            return nullpair;
        pp = p;
        p = it->second;
    }
    return std::make_pair(p, pp);
}

bool DirTree::checkFile(const std::vector<std::string> &path) {
//it assumes that checkPath will be successful
    auto p = checkPath(path, 0);
    if (p.first == nullptr)
        return true;
    return false;
}

void DirTree::copyDir(DirTreeNode* p1, DirTreeNode* p2) {
    for (auto it = p1->descendants.begin(); it != p1->descendants.end(); ++it)
       if (it->second == nullptr)
           p2->descendants[it->first] = nullptr;  //copying a file
       else {
           p2->descendants[it->first] = new DirTreeNode;
           copyDir(it->second, p2->descendants[it->first]);
       }
}

void DirTree::clearDir(DirTreeNode *p) {
    if (p == nullptr) return;
    for (auto it = p->descendants.begin(); it != p->descendants.end(); ++it) {
        clearDir(it->second);
        delete it->second;
        it->second = nullptr;
    }
    p->descendants.clear();
}

DirTree::~DirTree() {
    DirTree::clearDir(root);
    delete root;
}

void DirTree::printDirTree(std::ostream& out, const DirTreeNode *root, unsigned level, std::vector<bool> lastFlag) {
    for (auto it = root->descendants.begin(); it != root->descendants.end(); ++it) {
        if (lastFlag[0]) out << "  "; else out << " |";
        for (int i = 1; i <= level; ++i) out << (lastFlag[i] ? "    " : "   |");
        out << "_ " << it->first
#if PRINT_DIR_SLASH
            << (it->second != nullptr ? "/" : "")
#endif
            << "\n";
        lastFlag[level] = std::next(it) == root->descendants.end();
        if (it->second != nullptr) {
            lastFlag.push_back(false);
            printDirTree(out, it->second, level + 1, lastFlag);
            lastFlag.pop_back();
        }
    }
}

std::ostream& operator<<(std::ostream& out, const DirTree &dirTree) {
    out << "/\n";
    DirTree::printDirTree(out, dirTree.root, 0, {false});
    return out;
}

