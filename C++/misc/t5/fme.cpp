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

struct DirTreeNode {
    std::map<std::string, DirTreeNode*> descendants;
};

struct DirTree {
    DirTreeNode *root = new DirTreeNode;
};

std::vector<std::string> split(std::string input, const std::string &delimiters) {
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

const std::pair<DirTreeNode*, DirTreeNode*> nullpair{nullptr, nullptr};

std::pair<DirTreeNode*, DirTreeNode*> checkPath(const std::vector<std::string> &path, const DirTree& dirTree, int skips = 1) {
    DirTreeNode *p = dirTree.root, *pp = 0;
    for (int i = 0; i < path.size() - skips; ++i) {
        if (p == nullptr)
            return std::make_pair(nullptr, nullptr);
        auto it = p->descendants.find(path[i]);
        if (it == p->descendants.end())
            return std::make_pair(nullptr, nullptr);
        pp = p;
        p = it->second;
    }
    return std::make_pair(p, pp);
}

bool checkFile(const std::vector<std::string> &path, const DirTree& dirTree) {
    auto p = checkPath(path, dirTree, 0);
    if (p.first == nullptr)
        return true;
    else
        return false;
}

void deleteDir(DirTreeNode* p) {
   if (p == nullptr) return;
   for (auto it = p->descendants.begin(); it != p->descendants.end(); ++it)
       deleteDir(it->second);
   delete p;
}

void copyDir(DirTreeNode* p1, DirTreeNode* p2) {
    for (auto it = p1->descendants.begin(); it != p1->descendants.end(); ++it)
       if (it->second == nullptr)
           p2->descendants[it->first] = nullptr;  //copying a file
       else {
           p2->descendants[it->first] = new DirTreeNode;
           copyDir(it->second, p2->descendants[it->first]);
       }
}

bool md(const std::vector<std::string> &cmd, DirTree& dirTree) {
    if (cmd.size() < 2)
        throw std::runtime_error("command has no argument");
    if (cmd.size() > 2)
        throw std::runtime_error("extra argument(s)");
    auto path = split(cmd[1], "/");
    auto p = checkPath(path, dirTree);
    if (p == nullpair)
        throw std::runtime_error("can't create a directory, wrong path");
    if (p.first->descendants.find(path.back()) != p.first->descendants.end())
        throw std::runtime_error("attempt to overwrite an existing file or directory");
    p.first->descendants[path.back()] = new DirTreeNode;
    return true;
}

bool mf(const std::vector<std::string> &cmd, DirTree& dirTree) {
    if (cmd.size() < 2)
        throw std::runtime_error("command has no argument");
    if (cmd.size() > 2)
        throw std::runtime_error("extra argument(s)");
    auto path = split(cmd[1], "/");
    auto p = checkPath(path, dirTree);
    if (p == nullpair)
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
    auto p = checkPath(path, dirTree, 0);
    if (p == nullpair)
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
    auto p1 = checkPath(path1, dirTree, 0);
    if (p1 == nullpair)
        throw std::runtime_error("1st arg - no such file or directory");
    auto p2 = checkPath(path2, dirTree);
    if (p2 == nullpair)
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
    p2 = checkPath(path2, dirTree, 0);
    auto it = p2.first->descendants.find(path1.back());
    if (it != p2.first->descendants.end() && it->second == nullptr)
        throw std::runtime_error("can't copy a directory to a file");
    auto pn = new DirTreeNode;
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
    auto p1 = checkPath(path1, dirTree, 0);
    if (p1 == nullpair)
        throw std::runtime_error("1st arg - no such file or directory");
    auto p2 = checkPath(path2, dirTree);
    if (p2 == nullpair)
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
    p2 = checkPath(path2, dirTree, 0);
    auto it = p2.first->descendants.find(path1.back());
    if (it != p2.first->descendants.end() && it->second == nullptr)
        throw std::runtime_error("can't move a directory to a file");
    auto pn = new DirTreeNode;
    copyDir(p1.first, pn);
    p2.first->descendants[path1.back()] = pn;
    rm({"rm", cmd[1]}, dirTree);
    return true;
}

std::map<std::string, std::function<bool(const std::vector<std::string> &cmd, DirTree&)>> lex{{"md", md}, {"mf", mf}, {"rm", rm}, {"cp", cp}, {"mv", mv}};

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

void cleanDir(const DirTreeNode *root) {
    if (root == nullptr) return;
    for (auto it = root->descendants.begin(); it != root->descendants.end(); ++it)
        if (it->second != nullptr) {
            cleanDir(it->second);
            delete it->second;
        }
}

int cleanDir(DirTree &dirTree) {
    cleanDir(dirTree.root);
    delete dirTree.root;
}

void printDirTree(std::ostream& out, const DirTreeNode *root, unsigned level1) {
    for (auto it = root->descendants.begin(); it != root->descendants.end(); ++it) {
        out << "|";
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

