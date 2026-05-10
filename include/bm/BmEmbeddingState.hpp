#pragma once

#include <vector>

#include "bm/DfsPreprocessor.hpp"
#include "bm/Graph.hpp"
#include "bm/SeparatedDfsChildLists.hpp"

namespace bm {

struct BmVertexState {
    int vertex = -1;

    bool backedgeFlag = false;

    // visitedInStep == current DFI means visited.
    // This avoids clearing the array after each BM step.
    int visitedInStep = -1;

    // Roots of child biconnected components that are pertinent.
    // Walkup will fill this later.
    std::vector<int> pertinentRoots;
};

struct BmBicompRoot {
    int id = -1;

    // Non-virtual root vertex r.
    int parentVertex = -1;

    // DFS child c from the root edge (r, c).
    int childVertex = -1;

    // Original tree edge id for (r, c).
    int treeEdgeId = -1;

    bool active = true;

    // Later used when implementing lazy flipping.
    int rootEdgeSign = 1;
};

class BmEmbeddingState {
public:
    BmEmbeddingState(const Graph& graph, const DfsInfo& dfsInfo);

    const Graph& graph() const;
    const DfsInfo& dfsInfo() const;

    BmVertexState& vertexState(int vertex);
    const BmVertexState& vertexState(int vertex) const;

    bool isExternallyActive(int vertex, int currentVertex) const;

    int firstSeparatedDfsChild(int vertex) const;
    std::vector<int> separatedDfsChildren(int vertex) const;
    void removeSeparatedDfsChild(int parent, int child);

    int createTreeEdgeBicomp(int parentVertex, int childVertex);

    int bicompRootCount() const;
    const BmBicompRoot& bicompRoot(int rootId) const;

    int rootForChild(int childVertex) const;
    const std::vector<int>& childRoots(int vertex) const;

private:
    const Graph* graph_ = nullptr;
    const DfsInfo* dfsInfo_ = nullptr;

    std::vector<BmVertexState> vertexStates_;
    SeparatedDfsChildLists separatedDfsChildLists_;

    std::vector<BmBicompRoot> bicompRoots_;

    // DFS child c -> root id for virtual root parent^c.
    std::vector<int> rootForChild_;

    // Non-virtual vertex v -> roots v^c of its child biconnected components.
    std::vector<std::vector<int>> childRoots_;

    void validateVertex(int vertex) const;
};

} // namespace bm