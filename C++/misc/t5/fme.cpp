/*
File Manager Emulator

File Manager Emulator (FME) emulates the details of creating, removing, copying and moving files and directories. FME shall be capable to read and execute a batch file with different kind of commands. After the batch file execution it shall generate and print out formatted directory structure or an error message if something went wrong to standard output. Note that program should do nothing with the real file structure on local hard drives and shall only emulate these activities. Your goal is to write such File Manager Emulator.

FME Commands	

	md – creates a directory.
Command format: md <path>
Notes: md should not create any intermediate directories in the path.
Examples:
	md /Test – creates a directory called Test in the root directory.
	md /Dir1/Dir2/NewDir – creates a subdirectory “NewDir” if directory “/Dir1/Dir2” exists.

	mf – creates a file.
Command format: mf <path>
Notes: if such file already exists with the given path then FME should continue to the next command in the batch file without any error rising.
Examples: 
mf /Dir2/Dir3/file.txt – creates a file named file.txt in “/Dir2/Dir3” subdirectory.

	rm – removes a file or a directory with all its contents.
Command format: rm <path>
Examples:
	rm /Dir2/Dir3 – removes the directory “/Dir2/Dir3”.
	rm /Dir2/Dir3/file.txt – removes the file “file.txt” from the directory “/Dir2/Dir3”.

	cp – copy an existed directory/file to another location.
Command format: cp <source> <destination>
Notes: Program should copy directory with all its content. Destination path should not contain any file name except base-name otherwise FME should raise error (Base-name of “/dir/file.txt” is “file.txt”).
Examples:
	cp /Dir2/Dir3 /Dir1 – copies directory Dir3 in /Dir2 to /Dir1.
	cp /Dir2/Dir3/file.txt /Dir1 – copies file “file.txt” from /Dir2/Dir3 to /Dir1.
	cp /Dir2/Dir3/file.txt /Dir1/newfile.txt – copies file “file.txt” from /Dir2/Dir3 to /Dir1 and renames it to “newfile.txt”.

	mv – moves an existing directory/file to another location
Command format: mv <source> <destination>
Notes: Program should move directory with all its content.

Additional Implementation Notes

	You should use std C++ (program will be compiled and tested in Linux OS).
	Initially file system contains only the root directory marked as “/”.
	Commands, file and directory names are case sensitive.
	Any action shall not change the current directory.
	In case of any error occurs, the program shall stop and output a descriptive error message beginning with “ERROR: ”.
	In case of no errors the program should print out directory tree in any readable form (file/directory names should be organized in alphabetical ascending order). For example, as follows:

/
| _Dir1
|   |_Dir2
|   |  |_Dir3
|   |  |_readme.txt
|   |_EDir4
|   |  |_temp.dat

*/

#include <iostream>
#include <map>
#include <vector>
#include <string>
#include <functional>
#include <exception>
#include <utility>
#include "fme.h"
#include "parse.h"

const std::pair<DirTree::DirTreeNode*, DirTree::DirTreeNode*> DirTree::nullpair{nullptr, nullptr};

std::pair<DirTree::DirTreeNode*, DirTree::DirTreeNode*> DirTree::checkPath(const std::vector<std::string> &path, int skips) const {
//returns pointers to the final element found and its predecessor
    DirTree::DirTreeNode *p = root, *pp = 0;
    for (int i = 0; i < path.size() - skips; ++i) {
        if (p == nullptr)
            return DirTree::nullpair;
        auto it = p->descendants.find(path[i]);
        if (it == p->descendants.end())
            return DirTree::nullpair;
        pp = p;
        p = it->second;
    }
    return std::make_pair(p, pp);
}

bool checkFile(const std::vector<std::string> &path, const DirTree& dirTree) {
    auto p = dirTree.checkPath(path, 0);
    if (p.first == nullptr)
        return true;
    else
        return false;
}

void deleteDir(DirTree::DirTreeNode* p) {
   if (p == nullptr) return;
   for (auto it = p->descendants.begin(); it != p->descendants.end(); ++it)
       deleteDir(it->second);
   delete p;
}

void copyDir(DirTree::DirTreeNode* p1, DirTree::DirTreeNode* p2) {
    for (auto it = p1->descendants.begin(); it != p1->descendants.end(); ++it)
       if (it->second == nullptr)
           p2->descendants[it->first] = nullptr;  //copying a file
       else {
           p2->descendants[it->first] = new DirTree::DirTreeNode;
           copyDir(it->second, p2->descendants[it->first]);
       }
}

void cleanDir(const DirTree::DirTreeNode *root) {
    if (root == nullptr) return;
    for (auto it = root->descendants.begin(); it != root->descendants.end(); ++it)
        if (it->second != nullptr) {
            cleanDir(it->second);
            delete it->second;
        }
}

void cleanDir(DirTree &dirTree) {
    cleanDir(dirTree.root);
    delete dirTree.root;
}

void printDirTree(std::ostream& out, const DirTree::DirTreeNode *root, unsigned level1) {
    for (auto it = root->descendants.begin(); it != root->descendants.end(); ++it) {
        out << " |";
        for (int i = 0; i < level1; ++i) out << "    |";
        out << "_" << it->first << "\n";
        if (it->second != nullptr)
            printDirTree(out, it->second, level1 + 1);
    }
}

std::ostream& operator<<(std::ostream& out, const DirTree &dirTree) {
    out << "/\n";
    printDirTree(out, dirTree.root, 0);
    //out << "\n";
    return out;
}

int main() {
    try {
        DirTree dirTree;
        parse(dirTree);
        std::cout << dirTree;
        cleanDir(dirTree);
    }
    catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << std::endl;
    }
    return 0;
}

