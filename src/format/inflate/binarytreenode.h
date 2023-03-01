#ifndef BINARYTREENODE_H
#define BINARYTREENODE_H

#include <functional>
#include <string>

// Serialize a general binary tree with preorder traversal
template <typename treetype>
std::string serialize(treetype* root, const std::string delimiter="#") {
    std::string data = "";
    std::vector<const treetype*> path(1, root);

    // preorder traversal with a stack
    while(!path.empty()) {
        const long * curr = (const long*)path.back();
        path.pop_back();
        if (curr == nullptr) {
            data += delimiter + " ";
        }
        else {
            data += std::to_string((int)curr[0]) + " ";
            path.push_back((treetype*)curr[1]);
            path.push_back((treetype*)curr[2]);
        }
    }
    return data;
}


// Deserialize data into a binary tree
template <typename treetype>
treetype* deserialize(
        const std::string data, const std::string delimiter="#") {
    typedef std::reference_wrapper<treetype*> BTNodeRef;
    treetype* root;
    std::vector<BTNodeRef> openpoints(1, static_cast<BTNodeRef>(root));

    std::stringstream datain(data);
    std::string symbol;
    while(datain >> symbol) {
        if (openpoints.empty()) {
            throw std::invalid_argument("Corrupted data: " + data);
        }
        auto& curr = openpoints.back().get();
        openpoints.pop_back();
        if (symbol != delimiter) {
            int value = std::stoi(symbol, nullptr);
            curr = new treetype(value);
            openpoints.push_back(static_cast<BTNodeRef>(curr->right));
            openpoints.push_back(static_cast<BTNodeRef>(curr->left));
        }
    }
    return root;
}


class BinaryTreeNode {
public:
    int value;
    BinaryTreeNode *left, *right;

    BinaryTreeNode(int value=0):
        value(value),left(nullptr),right(nullptr) {}

    BinaryTreeNode(
            const std::string data, const std::string delimiter="#");

    BinaryTreeNode* insert_left(int value);
    BinaryTreeNode* insert_right(int value);
};

BinaryTreeNode::BinaryTreeNode(
            const std::string data, const std::string delimiter)
        : BinaryTreeNode(*deserialize<BinaryTreeNode>(data, delimiter)) {
}

BinaryTreeNode* BinaryTreeNode::insert_left(int value) {
    return left = new BinaryTreeNode(value);
}

BinaryTreeNode* BinaryTreeNode::insert_right(int value) {
    return right = new BinaryTreeNode(value);
}

#endif
