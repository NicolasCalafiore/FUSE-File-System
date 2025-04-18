//
// Created by nicol on 4/17/2025.
//

#ifndef WAD_H
#define WAD_H
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <regex>
#include "TreeNode.h"
#include <cstring>

#include "Wad.h"
using namespace std;

vector<string> Wad::splitPath(string& path)
{
    vector<string> parts;
    string current;

    for (char c : path) {
        if (c == '/' || c == '\\') {
            if (!current.empty()) {
                parts.push_back(current);
                current.clear();
            }
        } else {
            current.push_back(c);
        }
    }
    if (!current.empty())
        parts.push_back(current);

    return parts;
}

string Wad::getParentDirectory(string path){
  if(path.empty() || path == "/")
        return "";

    vector<string> parts = Wad::splitPath(path);
     return getParentDir(parts);
}

bool Wad::validParentDirectory(string path){
  vector<string> parts = Wad::splitPath(path);
  string parentDir = getParentDir(parts);
  vector<string> parent_parts = Wad::splitPath(parentDir);
  TreeNode* node = TreeNode::getNode(root, parent_parts);

  return node != nullptr;
}

string Wad::getParentDir(vector<string> parts) {
    string parentDir = "";
    for (int i = 0; i < parts.size() - 1; ++i) {
        parentDir += parts[i];
        if (i != parts.size() - 2)
            parentDir += "/";
    }

    return parentDir;
}

bool Wad::isME(const string &name) {

    if (name.size() < 3 || name[0] != 'E') return false;

    auto posM = name.find('M', 1);
    if (posM == string::npos || posM == 1 || posM + 1 >= name.size())
        return false;

    for (size_t i = 1; i < posM; ++i)
        if (!isdigit(static_cast<unsigned char>(name[i])))
            return false;

    for (size_t i = posM + 1; i < name.size(); ++i)
        if (!isdigit(static_cast<unsigned char>(name[i])))
            return false;

    return true;
}

bool Wad::isSTART(const string &name) {
    return name.size() > 6 &&
        name.compare(name.size() - 6,
        6, "_START") == 0;
}

 string Wad::generateEndElement(const string &startName) {
     return startName.substr(0, startName.size() - 6) + "_END";
 }

int Wad::addMEToTree(int i, TreeNode *parent) {
    for (int k = 0; k < 10 && i < descripters.size(); k++, i++) {
        const auto &d = descripters[i];
        bool start = isSTART(d.name);
        string dname = d.name;
        string nodeName = start
            ? dname.substr(0, dname.size() - 6)
            : dname;
        int nodeSize = start ? 0 : d.size;

        auto *child = new TreeNode(parent, nodeName, nodeSize);
        parent->addChild(child);

        if (start) {
            i = addStartEndToTree(i + 1, child, generateEndElement(dname));
        }
        else if (isME(d.name)) {
            i = addMEToTree(i + 1, child);
        }
    }
    return i;
}

int Wad::addStartEndToTree(int i, TreeNode *parent, const string &endTag){
    while (i < descripters.size() &&
           string(descripters[i].name) != endTag) {
        const auto &d = descripters[i];
        bool start = isSTART(d.name);
        string dname = d.name;
        string nodeName = start
            ? dname.substr(0, dname.size() - 6)
            : dname;
        int nodeSize = start ? 0 : d.size;

        auto *child = new TreeNode(parent, nodeName, nodeSize);
        parent->addChild(child);

        if (start) {
            i = addStartEndToTree(i + 1, child, generateEndElement(dname));
        }
        else if (isME(d.name)) {
            i = addMEToTree(i + 1, child);
        }
        else {
            i++;
        }
           }

    return i + 1;
}

Wad::Wad(const string &path) {
    file.open(path, ios::in | ios::out | ios::binary);

    magic[4] = '\0';
    file.read(magic, 4);
    file.read((char*)&numberOfDescripters, 4);
    file.read((char*)&FlatLumpDescriptors, 4);

    file.seekg(FlatLumpDescriptors, ios::beg);

    descripters.reserve(numberOfDescripters);

    for (uint32_t i = 0; i < numberOfDescripters; ++i) {
        LumpDesc d{};
        file.read((char*)&d.offset, 4);
        file.read((char*)&d.size,   4);
        file.read(d.name, 8);
        d.name[8] = '\0';

        descripters.push_back(d);
    }

    root = new TreeNode(nullptr, "/", -1);
    int i = 0;
    while (i < descripters.size()) {
        const auto &d = descripters[i];
        // detect namespace‐start
        bool start = isSTART(d.name);
        string dname = d.name;
        string nodeName = start
            ? dname.substr(0, dname.size() - 6)  // strip "_START"
            : dname;
        int nodeSize = start ? 0 : d.size;

        auto *node = new TreeNode(root, nodeName, nodeSize);
        root->addChild(node);

        if (start) {
            i = addStartEndToTree(i + 1, node, generateEndElement(dname));
        }
        else if (isME(d.name)) {
            i = addMEToTree(i + 1, node);
        }
        else {
            i++;
        }
    }
}


Wad* Wad::loadWad(const string &path) {
    return new Wad(path);
}

string Wad::getMagic() {
    return magic;
}

bool Wad::isContent(const std::string& path) {
    if (path.empty() || path == "/")
        return false;

    std::string p = path;
    if (p.back() == '/') p.pop_back();
    auto parts = splitPath(p);
    if (parts.empty())
        return false;

    TreeNode* node = TreeNode::getNode(root, parts);
    if (!node)
        return false;

    const std::string &leaf = parts.back();
    bool found = false;
    for (auto &d : descripters) {
        if (leaf == d.name) {
            found = true;
            break;
        }
    }
    if (!found)
        return false;

    if (isME(leaf)
     || isSTART(leaf)
     || (leaf.size()>4 && leaf.substr(leaf.size()-4)== "_END"))  // “FOO_END”
    {
        return false;
    }

    return true;
}



bool Wad::isDirectory(const std::string& path) {
    if (path.empty()) return false;
    if (path == "/")
        return true;
    if (isContent(path))
        return false;

    auto parts = splitPath(const_cast<std::string&>(path));
    return TreeNode::getNode(root, parts) != nullptr;
}


int Wad::getSize(const std::string &path) {
    if (!isContent(path))
        return -1;
    auto parts = splitPath(const_cast<std::string&>(path));
    TreeNode* node = TreeNode::getNode(root, parts);
    return node ? node->size : -1;
}



int Wad::getContents(const std::string &path, char *buffer, int length, int offset)
{

    if (path.empty() || path == "/")     return -1;
    if (!isContent(path))                return -1;

    auto parts = splitPath(const_cast<std::string&>(path));
    TreeNode* node = TreeNode::getNode(root, parts);
    if (!node)                           return -1;
    int fileSize = node->size;

    if (offset >= fileSize)              return 0;

    int toRead = std::min(length, fileSize - offset);

    const std::string &leaf = parts.back();
    const LumpDesc *desc = nullptr;
    for (auto &d : descripters) {
        if (leaf == d.name) {
            desc = &d;
            break;
        }
    }
    if (!desc)                           return -1;

    file.seekg(desc->offset + offset, ios::beg);
    file.read(buffer, toRead);
    return toRead;
}


int Wad::getDirectory(const string &path, vector<string> *directory)
{
    if (!directory || !isDirectory(path))
        return -1;

    auto parts = splitPath(const_cast<string&>(path));
    TreeNode* node = TreeNode::getNode(root, parts);
    if (!node) return -1;

    for (TreeNode* child : node->children)
        directory->push_back(child->name);

    return static_cast<int>(node->children.size());
}

void Wad::createDirectory(const string &path) {
    if (path.empty() || path == "/") return;

    string p = path;
    if (p.back() == '/') p.pop_back();
    if (p.front() != '/') return;

    auto parts    = splitPath(p);
    if (parts.empty()) return;

    string newName = parts.back();
    if (newName.size() > 2) return;

   string parentPath = getParentDir(parts);
if (parentPath.empty())
    parentPath = "/";

if (!isDirectory(parentPath))
    return;

    auto parentParts = splitPath(parentPath);
    TreeNode* parent = TreeNode::getNode(root, parentParts);
    if (!parent) return;
    if (isME(parent->name)) return;
    for (auto *c : parent->children)
        if (c->name == newName)
            return;


    size_t insertIdx = descripters.size();
    if (!parentPath.empty()) {
        string endTag = parent->name + "_END";
        for (size_t i = 0; i < descripters.size(); ++i) {
            if (endTag == descripters[i].name) {
                insertIdx = i;
                break;
            }
        }
    }

    LumpDesc startD{0,0,{0}};
    strncpy(startD.name, (newName + "_START").c_str(), 8);
    LumpDesc   endD{0,0,{0}};
    strncpy(  endD.name, (newName + "_END"  ).c_str(), 8);

    descripters.insert(descripters.begin() + insertIdx, startD);
    descripters.insert(descripters.begin() + insertIdx + 1, endD);
    numberOfDescripters += 2;

    auto *dirNode = new TreeNode(parent, newName, /*size=*/0);
    parent->addChild(dirNode);

    file.seekp(0, ios::beg);
    file.write(magic, 4);
    file.write(reinterpret_cast<char*>(&numberOfDescripters), sizeof(numberOfDescripters));
    file.write(reinterpret_cast<char*>(&FlatLumpDescriptors), sizeof(FlatLumpDescriptors));

    file.seekp(FlatLumpDescriptors, ios::beg);
    for (auto &d : descripters) {
        file.write(reinterpret_cast<char*>(&d.offset), 4);
        file.write(reinterpret_cast<char*>(&d.size),   4);
        file.write(d.name,                            8);
    }
    file.flush();

}

void Wad::createFile(const string &path) {

    if (path.empty() || path == "/") return;
    string p = path;
    if (p.back() == '/') p.pop_back();
    if (p.front() != '/') return;

    auto parts   = splitPath(p);
    if (parts.empty()) return;
    string newName = parts.back();
    if (newName.size() > 8) return;
	if (isME(newName)) return;
string parentPath = getParentDir(parts);
    if (parentPath.empty())
        parentPath = "/";
    if (!isDirectory(parentPath))
        return;
    auto parentParts = splitPath(parentPath);
    TreeNode* parent = TreeNode::getNode(root, parentParts);
    if (!parent) return;
    if (isME(parent->name)) return;
    for (auto *c : parent->children)
        if (c->name == newName)
            return;

    size_t insertIdx = descripters.size();
    if (!parentPath.empty()) {
        string endTag = parent->name + "_END";
        for (size_t i = 0; i < descripters.size(); ++i) {
            if (endTag == descripters[i].name) {
                insertIdx = i;
                break;
            }
        }
    }

    LumpDesc d{0, 0, {0}};
    strncpy(d.name, newName.c_str(), 8);

    descripters.insert(descripters.begin() + insertIdx, d);
    numberOfDescripters++;

    auto *fileNode = new TreeNode(parent, newName, /*size=*/0);
    parent->addChild(fileNode);

    file.seekp(0, ios::beg);
    file.write(magic, 4);
    file.write(reinterpret_cast<char*>(&numberOfDescripters), 4);
    file.write(reinterpret_cast<char*>(&FlatLumpDescriptors), 4);
    file.seekp(FlatLumpDescriptors, ios::beg);
    for (auto &ld : descripters) {
        file.write(reinterpret_cast<char*>(&ld.offset), 4);
        file.write(reinterpret_cast<char*>(&ld.size),   4);
        file.write(ld.name,                            8);
    }
    file.flush();
}

int Wad::writeToFile(const string &path, const char *buffer, int length, int offset) {

   if (!isContent(path)) return -1;
    auto parts = splitPath(const_cast<string&>(path));
    TreeNode* node = TreeNode::getNode(root, parts);
    if (!node){
      cout << "write 1" << endl;
      return -1;
      }

    if (node->size > 0){
      cout << "write 2" << endl;
      return 0;
     }
    if (offset != 0){
      cout << "write 3" << endl;
      return -1;
      }

    size_t descIdx = 0;
    bool found = false;
    for (; descIdx < descripters.size(); ++descIdx) {
        if (path == (string("/") + descripters[descIdx].name)) {
            found = true;
            break;
        }

        if (descripters[descIdx].name == parts.back()) {

            auto pd = getParentDir(parts);

            TreeNode* pnode = TreeNode::getNode(root, splitPath(pd));
            if (pnode && pnode->name == node->parent->name) {
                found = true;
                break;
            }
        }
    }
    if (!found){
      cout << "write 5" << endl;
      return -1;
      }

    uint32_t writeAt = FlatLumpDescriptors;
    file.seekp(writeAt, ios::beg);
    file.write(buffer, length);

    descripters[descIdx].offset = writeAt;
    descripters[descIdx].size   = length;
    node->size = length;

    FlatLumpDescriptors += length;
    file.seekp(0, ios::beg);
    file.write(magic, 4);
    file.write(reinterpret_cast<char*>(&numberOfDescripters), 4);
    file.write(reinterpret_cast<char*>(&FlatLumpDescriptors), 4);

    file.seekp(FlatLumpDescriptors, ios::beg);
    for (auto &ld : descripters) {
        file.write(reinterpret_cast<char*>(&ld.offset), 4);
        file.write(reinterpret_cast<char*>(&ld.size),   4);
        file.write(ld.name,                            8);
    }
    file.flush();

    return length;
}

void Wad::printHeader() {
    cout << "Magic: " << magic << endl;
    cout << "Number of Descripters: " << numberOfDescripters << endl;
    cout << "Descripter Offset: " << FlatLumpDescriptors << endl;
}
string Wad::loadLump(const LumpDesc &d) {
    string data(d.size, '\0');
    file.seekg(d.offset, ios::beg);
    file.read(&data[0],    d.size);
    return data;
}
void Wad::printTree(TreeNode *node, const string &indent)
{
    if (!node) return;

    cout << indent << node->name;

    cout << "  (" << node->size << " bytes)";

    cout << '\n';

    for (TreeNode *child : node->children)
        printTree(child, indent + "  ");
}

TreeNode* Wad::getRoot() {
    return root;
}

#endif //WAD_H

