#pragma once

#include <vector>

#include "bm/DfsPreprocessor.hpp"

namespace bm {

struct BmVertexState {
    int vertex = -1;

    // BM pertinence helper
    bool backedgeFlag = false;

    // Umjesto bool visited koji se mora cistiti
    // visitedInStep == currentDfi znaci visited
    int visitedInStep = -1;

    // Rootovi child BCC koji su pertinent
    // kasnije Walkup puni ovu listu
    std::vector<int> pertinentRoots;

    // separatedDFSChildList za external activity
    // TODO: replace vector with linked-list representation before implementing merge
    // BM requires O(1) deletion from separatedDfsChildList
    std::vector<int> separatedDfsChildList;
};

class BmEmbeddingState {
public:
    BmEmbeddingState(const Graph& graph, const DfsInfo& dfsInfo);

    const Graph& graph() const;
    const DfsInfo& dfsInfo() const;

    BmVertexState& vertexState(int vertex);
    const BmVertexState& vertexState(int vertex) const;

    bool isExternallyActive(int vertex, int currentVertex) const;

private:
    const Graph* graph_ = nullptr;
    const DfsInfo* dfsInfo_ = nullptr;

    std::vector<BmVertexState> vertexStates_;
};

} // namespace bm