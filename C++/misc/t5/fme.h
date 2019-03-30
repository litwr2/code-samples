#ifndef FME_H
#define FME_H

struct DirTree {
    struct DirTreeNode {
        std::map<std::string, DirTreeNode*> descendants;
    } *root = new DirTreeNode;
    static const std::pair<DirTreeNode*, DirTreeNode*> nullpair;
    std::pair<DirTreeNode*, DirTreeNode*> checkPath(const std::vector<std::string> &path, int skips = 1) const;
};

void deleteDir(DirTree::DirTreeNode* p);
void copyDir(DirTree::DirTreeNode* p1, DirTree::DirTreeNode* p2);
void cleanDir(const DirTree::DirTreeNode *root);
void cleanDir();

bool checkFile(const std::vector<std::string> &path, const DirTree& dirTree);

#endif

