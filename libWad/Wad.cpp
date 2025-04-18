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
        // otherwise just advance (k loop will also advance)
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
            // nested namespace
            i = addStartEndToTree(i + 1, child, generateEndElement(dname));
        }
        else if (isME(d.name)) {
            // map marker inside a namespace
            i = addMEToTree(i + 1, child);
        }
        else {
            i++;
        }
           }
    // skip over the matching _END descriptor
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
            // recurse until the matching _END
            i = addStartEndToTree(i + 1, node, generateEndElement(dname));
        }
        else if (isME(d.name)) {
            // map marker: consumes next 10 entries (or until recursion unwinds)
            i = addMEToTree(i + 1, node);
        }
        else {
            // plain file
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

    // Trim any trailing slash, then split
    std::string p = path;
    if (p.back() == '/') p.pop_back();
    auto parts = splitPath(p);
    if (parts.empty())
        return false;

    // Must exist in the in‐memory tree
    TreeNode* node = TreeNode::getNode(root, parts);
    if (!node)
        return false;

    // Look up the exact descriptor by leaf name
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

    // Exclude namespace markers and map markers
    if (isME(leaf)                  // “E#M#” map marker
     || isSTART(leaf)             // “FOO_START”
     || (leaf.size()>4 && leaf.substr(leaf.size()-4)== "_END"))  // “FOO_END”
    {
        return false;
    }

    // Anything else with a matching descriptor is file content
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
    if (!isContent(path))     // no longer check isDirectory
        return -1;
    auto parts = splitPath(const_cast<std::string&>(path));
    TreeNode* node = TreeNode::getNode(root, parts);
    return node ? node->size : -1;
}


// In libWad.h, inside class libWad:

int Wad::getContents(const std::string &path,
                     char *buffer,
                     int length,
                     int offset)
{
    // 1) path must be non‐empty and represent content
    if (path.empty() || path == "/")     return -1;
    if (!isContent(path))                return -1;

    // 2) locate the node & its size
    auto parts = splitPath(const_cast<std::string&>(path));
    TreeNode* node = TreeNode::getNode(root, parts);
    if (!node)                           return -1;
    int fileSize = node->size;

    // 3) if offset past EOF, copy nothing
    if (offset >= fileSize)              return 0;

    // 4) how many bytes can we actually read?
    int toRead = std::min(length, fileSize - offset);

    // 5) find the descriptor for this lump
    const std::string &leaf = parts.back();
    const LumpDesc *desc = nullptr;
    for (auto &d : descripters) {
        if (leaf == d.name) {
            desc = &d;
            break;
        }
    }
    if (!desc)                           return -1;

    // 6) seek & read
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
    parentPath = "/";                // ← assume root

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

    // 5) Build the two new namespace markers
    LumpDesc startD{0,0,{0}};
    strncpy(startD.name, (newName + "_START").c_str(), 8);
    LumpDesc   endD{0,0,{0}};
    strncpy(  endD.name, (newName + "_END"  ).c_str(), 8);

    // insert them and bump the count
    descripters.insert(descripters.begin() + insertIdx, startD);
    descripters.insert(descripters.begin() + insertIdx + 1, endD);
    numberOfDescripters += 2;

    // 6) Update the in‐memory TreeNode
    auto *dirNode = new TreeNode(parent, newName, /*size=*/0);
    parent->addChild(dirNode);

    // 7) Persist to disk:
    //    Rewrite header (magic + count + offset)
    file.seekp(0, ios::beg);
    file.write(magic, 4);
    file.write(reinterpret_cast<char*>(&numberOfDescripters), sizeof(numberOfDescripters));
    file.write(reinterpret_cast<char*>(&FlatLumpDescriptors), sizeof(FlatLumpDescriptors));
    //    Then rewrite the whole descriptor array at FlatLumpDescriptors
    file.seekp(FlatLumpDescriptors, ios::beg);
    for (auto &d : descripters) {
        file.write(reinterpret_cast<char*>(&d.offset), 4);
        file.write(reinterpret_cast<char*>(&d.size),   4);
        file.write(d.name,                            8);
    }
    file.flush();

}

void Wad::createFile(const string &path) {
    // 1) Basic sanity checks
    if (path.empty() || path == "/") return;
    string p = path;
    if (p.back() == '/') p.pop_back();
    if (p.front() != '/') return;

    // 2) Split into parent + newName
    auto parts   = splitPath(p);
    if (parts.empty()) return;
    string newName = parts.back();
    // file names must fit in 8 chars
    if (newName.size() > 8) return;
	if (isME(newName)) return;
    // 3) Parent must be a directory
string parentPath = getParentDir(parts);
    if (parentPath.empty())      // no parent means “/”
        parentPath = "/";
    if (!isDirectory(parentPath))
        return;
    auto parentParts = splitPath(parentPath);
    TreeNode* parent = TreeNode::getNode(root, parentParts);
    if (!parent) return;
    // cannot drop new files in a map marker
    if (isME(parent->name)) return;
    // no duplicate in parent
    for (auto *c : parent->children)
        if (c->name == newName)
            return;

    // 4) Find where to insert in the descriptor list
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

    // 6) Mirror in the TreeNode
    auto *fileNode = new TreeNode(parent, newName, /*size=*/0);
    parent->addChild(fileNode);

    // 7) Persist header + updated descriptor list
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
    // 1) Path must refer to a file (even if it's zero‐length)
   if (!isContent(path)) return -1;
    auto parts = splitPath(const_cast<string&>(path));
    TreeNode* node = TreeNode::getNode(root, parts);
    if (!node) return -1;

    // 2) If it already has contents (either an original lump or a prior write), fail with 0
    if (node->size > 0) return 0;
    // we only support offset==0 for brand‑new files
    if (offset != 0)   return -1;

    // 3) Locate its descriptor in our vector
    size_t descIdx = 0;
    bool found = false;
    for (; descIdx < descripters.size(); ++descIdx) {
        if (path == (string("/") + descripters[descIdx].name)) {
            found = true;
            break;
        }
        // also handle non‑root: compare last part
        if (descripters[descIdx].name == parts.back()) {
            // make sure parent matches
            auto pd = getParentDir(parts);
            // split pd and compare TreeNode
            TreeNode* pnode = TreeNode::getNode(root, splitPath(pd));
            if (pnode && pnode->name == node->parent->name) {
                found = true;
                break;
            }
        }
    }
    if (!found) return -1;

    // 4) Append the raw bytes just before the descriptor table
    uint32_t writeAt = FlatLumpDescriptors;
    file.seekp(writeAt, ios::beg);
    file.write(buffer, length);

    // 5) Update our in‑memory descriptor + tree
    descripters[descIdx].offset = writeAt;
    descripters[descIdx].size   = length;
    node->size = length;

    // 6) Slide the descriptor table out by 'length'
    FlatLumpDescriptors += length;
    file.seekp(0, ios::beg);
    file.write(magic, 4);
    file.write(reinterpret_cast<char*>(&numberOfDescripters), 4);
    file.write(reinterpret_cast<char*>(&FlatLumpDescriptors), 4);

    // 7) Rewrite the full descriptor list at its new offset
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

