#ifndef PARSE_H
#define PARSE_H

std::vector<std::string> split(std::string input, const std::string &delimiters);
bool md(const std::vector<std::string> &cmd, DirTree& dirTree);
bool mf(const std::vector<std::string> &cmd, DirTree& dirTree);
bool rm(const std::vector<std::string> &cmd, DirTree& dirTree);
bool cp(const std::vector<std::string> &cmd, DirTree& dirTree);
bool mv(const std::vector<std::string> &cmd, DirTree& dirTree);
int parse(DirTree &dirTree);

extern std::map<std::string, std::function<bool(const std::vector<std::string> &cmd, DirTree&)>> lex;
#endif

