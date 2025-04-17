#pragma once
// Wad.h  – declarations only
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <regex>
#include <map>
#include <cstring>
#include "TreeNode.h"

using namespace std;
class Wad {
private:
    struct LumpDesc {
        uint32_t offset;
        uint32_t size;
        char     name[9]; // 8-char + null
    };

    TreeNode *root;

    uint32_t numberOfDescripters{};
    uint32_t FlatLumpDescriptors{};
    char magic[5]{};

    vector<LumpDesc> descripters;

    std::fstream file;

    std::regex ME_Pattern{R"(^E\d+M\d+$)"};
    std::regex START_END_Pattern{R"(^(.+)_START$)"};

    std::map<std::string, TreeNode*> pathNodeMap;

    static std::vector<std::string> splitPath(std::string& path);
    static std::string getParentDir(std::vector<std::string> parts);
    bool isME(const string &name);
    static bool isSTART(const std::string &name);

    static std::string generateEndElement(const std::string &startName);

    int addMEToTree(int i, TreeNode *parent);

    int addStartEndToTree(int i, TreeNode *parent, const std::string &endTag);

    Wad(const string &path);

public:
    static Wad* loadWad(const std::string &path);

    std::string getMagic();

    bool isContent(const std::string& path);

    bool isDirectory(const std::string& const_path);

    int getSize(const std::string &path);

    // In libWad.h, inside class libWad:

    int getContents(const std::string &path, char *buffer, int length, int offset = 0);

    int getDirectory(const std::string &path, std::vector<std::string> *directory);

    void createDirectory(const std::string &path);
    void createFile(const std::string &path);
    int writeToFile(const std::string &path, const char *buffer, int length, int offset = 0);


    void printHeader();
    string loadLump(const LumpDesc &d);
    static void printTree(TreeNode *node, const std::string &indent = "");
    TreeNode* getRoot();

};