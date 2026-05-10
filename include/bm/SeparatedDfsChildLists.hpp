#pragma once

#include <vector>

#include "bm/DfsPreprocessor.hpp"

namespace bm {

struct SeparatedDfsChildNode {
    int parent = -1;
    int child = -1;

    int previous = -1;
    int next = -1;

    bool linked = false;
};

class SeparatedDfsChildLists {
public:
    explicit SeparatedDfsChildLists(const DfsInfo& dfsInfo);

    bool empty(int parent) const;
    int frontChild(int parent) const;

    bool containsChild(int child) const;
    void removeChild(int parent, int child);

    std::vector<int> toVector(int parent) const;

private:
    std::vector<int> head_;
    std::vector<int> tail_;
    std::vector<int> nodeOfChild_;
    std::vector<SeparatedDfsChildNode> nodes_;

    void appendChild(int parent, int child);
    void validateVertex(int vertex) const;
};

} // namespace bm