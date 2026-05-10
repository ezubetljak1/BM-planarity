#include "bm/SeparatedDfsChildLists.hpp"

#include <stdexcept>

namespace bm {

SeparatedDfsChildLists::SeparatedDfsChildLists(const DfsInfo& dfsInfo)
    : head_(dfsInfo.vertexCount, -1), tail_(dfsInfo.vertexCount, -1),
      nodeOfChild_(dfsInfo.vertexCount, -1) {
    nodes_.reserve(dfsInfo.vertexCount);

    for (int parent = 0; parent < dfsInfo.vertexCount; ++parent) {
        for (int child : dfsInfo.childrenSortedByLowpoint[parent])
            appendChild(parent, child);
    }
}

bool SeparatedDfsChildLists::empty(int parent) const {
    validateVertex(parent);
    return head_[parent] == -1;
}

int SeparatedDfsChildLists::frontChild(int parent) const {
    validateVertex(parent);

    const int headNode = head_[parent];

    if (headNode == -1)
        return -1;

    return nodes_[headNode].child;
}

bool SeparatedDfsChildLists::containsChild(int child) const {
    validateVertex(child);

    const int nodeId = nodeOfChild_[child];

    if (nodeId == -1)
        return false;

    return nodes_[nodeId].linked;
}

void SeparatedDfsChildLists::removeChild(int parent, int child) {
    validateVertex(parent);
    validateVertex(child);

    const int nodeId = nodeOfChild_[child];

    if (nodeId == -1)
        throw std::logic_error("Separated DFS child does not have a representative node.");

    SeparatedDfsChildNode& node = nodes_[nodeId];

    if (!node.linked)
        throw std::logic_error("Separated DFS child is already removed.");

    if (node.parent != parent)
        throw std::logic_error("Separated DFS child belongs to a different parent.");

    const int previousNode = node.previous;
    const int nextNode = node.next;

    if (previousNode == nodeId && nextNode == nodeId) {
        head_[parent] = -1;
        tail_[parent] = -1;
    } else {
        nodes_[previousNode].next = nextNode;
        nodes_[nextNode].previous = previousNode;

        if (head_[parent] == nodeId)
            head_[parent] = nextNode;

        if (tail_[parent] == nodeId)
            tail_[parent] = previousNode;
    }

    node.previous = -1;
    node.next = -1;
    node.linked = false;
}

std::vector<int> SeparatedDfsChildLists::toVector(int parent) const {
    validateVertex(parent);

    std::vector<int> result;

    const int headNode = head_[parent];

    if (headNode == -1)
        return result;

    int currentNode = headNode;

    do {
        const auto& node = nodes_[currentNode];
        result.push_back(node.child);
        currentNode = node.next;
    } while (currentNode != headNode);

    return result;
}

void SeparatedDfsChildLists::appendChild(int parent, int child) {
    validateVertex(parent);
    validateVertex(child);

    if (nodeOfChild_[child] != -1)
        throw std::logic_error("DFS child already has a representative node.");

    const int nodeId = nodes_.size();

    SeparatedDfsChildNode node;
    node.parent = parent;
    node.child = child;
    node.linked = true;

    const int oldTail = tail_[parent];

    if (oldTail == -1) {
        node.previous = nodeId;
        node.next = nodeId;

        head_[parent] = nodeId;
        tail_[parent] = nodeId;
    } else {
        const int oldHead = head_[parent];

        node.previous = oldTail;
        node.next = oldHead;

        nodes_[oldTail].next = nodeId;
        nodes_[oldHead].previous = nodeId;

        tail_[parent] = nodeId;
    }

    nodes_.push_back(node);
    nodeOfChild_[child] = nodeId;
}

void SeparatedDfsChildLists::validateVertex(int vertex) const {
    if (vertex < 0 || vertex >= head_.size())
        throw std::out_of_range("Invalid vertex id.");
}

} // namespace bm