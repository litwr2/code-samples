#ifndef FME_H
#define FME_H

struct DirTreeNode {
    std::map<std::string, DirTreeNode*> descendants;
};

struct DirTree {
    DirTreeNode *root = new DirTreeNode;
    static const std::pair<DirTreeNode*, DirTreeNode*> nullpair;
    std::pair<DirTreeNode*, DirTreeNode*> checkPath(const std::vector<std::string> &path, int skips = 1) const;
    bool checkFile(const std::vector<std::string> &path);
    static void deleteDir(DirTreeNode* p);
    static void copyDir(DirTreeNode* p1, DirTreeNode* p2);
    static void clearDir(const DirTreeNode *root);
    static void printDirTree(std::ostream& out, const DirTreeNode *root, unsigned level = 0, std::vector<bool> lastFlag = {});
    ~DirTree();
};

std::ostream& operator<<(std::ostream& out, const DirTree &dirTree);

#endif

