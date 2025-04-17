//
// Created by nicol on 4/17/2025.
//

#ifndef TREENODE_H
#define TREENODE_H
#include <vector>
#include <string>
//
// Created by nicol on 4/17/2025.
//
class TreeNode {
public:
    TreeNode* parent;
    std::string name;
    int size;
    std::vector<TreeNode*> children;

    TreeNode(TreeNode* parent, std::string name, int size) {
        this->parent = parent;
        this->name = name;
        this->size = size;
    }

    void addChild(TreeNode* child) {
        children.push_back(child);
    }

    std::string getPath() {
        std::string path = "";
        TreeNode* current = this;
        while (current != nullptr) {
            path = current->name + "/" + path;
            current = current->parent;
        }
        return path;
    }

    static TreeNode* getNode(TreeNode* start, const std::vector<std::string>& parts) {
        TreeNode* node = start;
        for (const auto& name : parts) {
            bool found = false;
            for (TreeNode* child : node->children) {
                if (child->name == name) {
                    node = child;
                    found = true;
                    break;
                }
            }
            if (!found) return nullptr;
        }
        return node;
    }




};
#endif //TREENODE_H
