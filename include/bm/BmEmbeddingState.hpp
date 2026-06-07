#pragma once

#include <vector>
#include <deque>

#include "bm/BmPartialEmbedding.hpp"
#include "bm/DfsPreprocessor.hpp"
#include "bm/Graph.hpp"
#include "bm/SeparatedDfsChildLists.hpp"

namespace bm {

struct BmVertexState {
    int vertex = -1;

    int backedgeFlagDfi = -1;

    // visitedInStep == current DFI means visited.
    // This avoids clearing the array after each BM step.
    int visitedInStep = -1;

    // Roots of child biconnected components that are pertinent.
    // Walkup will fill this later.
    std::deque<int> pertinentRoots;
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

    // Internal embedding objects.
    int internalRootVertexId = -1;
    int internalChildVertexId = -1;
    int embeddedTreeEdgeId = -1;
    int rootToChildHalfEdgeId = -1;
    int childToRootHalfEdgeId = -1;
};

class BmEmbeddingState {
public:
    BmEmbeddingState(const Graph& graph, const DfsInfo& dfsInfo);

    const Graph& graph() const;
    const DfsInfo& dfsInfo() const;

    BmVertexState& vertexState(int vertex);
    const BmVertexState& vertexState(int vertex) const;

    const BmPartialEmbedding& partialEmbedding() const;
    BmPartialEmbedding& partialEmbedding();

    bool isExternallyActive(int vertex, int currentVertex) const;

    int firstSeparatedDfsChild(int vertex) const;
    std::vector<int> separatedDfsChildren(int vertex) const;
    void removeSeparatedDfsChild(int parent, int child);

    int createTreeEdgeBicomp(int parentVertex, int childVertex);

    int bicompRootCount() const;
    const BmBicompRoot& bicompRoot(int rootId) const;
    BmBicompRoot& bicompRoot(int rootId);


    int rootForChild(int childVertex) const;
    const std::vector<int>& childRoots(int vertex) const;

    bool hasBackedgeFlag(int vertex, int currentVertex) const;
    void markBackedgeFlag(int vertex, int currentVertex);
    void clearBackedgeFlag(int vertex);

    bool isVisitedInStep(int vertex, int currentVertex) const;
    void markVisitedInStep(int vertex, int currentVertex);

    bool isPertinent(int vertex, int currentVertex) const;
    bool isInternallyActive(int vertex, int currentVertex) const;
    bool isInactive(int vertex, int currentVertex) const;

    bool isRootExternallyActive(int rootId, int currentVertex) const;
    void addPertinentRoot(int vertex, int rootId, int currentVertex);

    bool hasPertinentRoots(int vertex) const;
    int firstPertinentRoot(int vertex) const;
    void removeFirstPertinentRoot(int vertex);

    bool isInternalBicompRootVertex(int internalVertexId) const;
    int bicompRootIdForInternalVertex(int internalVertexId) const;
    int originalVertexForInternalVertex(int internalVertexId) const;

    bool isInternalVertexVisitedInStep(int internalVertexId, int currentVertex) const;
    void markInternalVertexVisitedInStep(int internalVertexId, int currentVertex);
    void validateVertex(int vertex) const;
    bool isBackedgeEndpointForCurrentVertex(int vertex, int currentVertex) const;

    void deactivateBicompRoot(int rootId);

    void removeExpectedFirstPertinentRoot(int vertex, int expectedRootId);

private:
    const Graph* graph_ = nullptr;
    const DfsInfo* dfsInfo_ = nullptr;

    std::vector<BmVertexState> vertexStates_;
    SeparatedDfsChildLists separatedDfsChildLists_;

    BmPartialEmbedding partialEmbedding_;

    std::vector<BmBicompRoot> bicompRoots_;

    // DFS child c -> root id for virtual root parent^c.
    std::vector<int> rootForChild_;

    // Non-virtual vertex v -> roots v^c of its child biconnected components.
    std::vector<std::vector<int>> childRoots_;

    std::vector<int> internalVertexVisitedInStep_;

    void ensureInternalVisitedCapacity();
    void validateInternalVertex(int internalVertexId) const;
};

} // namespace bm