#pragma once

#include <vector>

#include "bm/Graph.hpp"

namespace bm {

struct DfsBackEdge {
    int edgeId = -1;
    int ancestor = -1;   // vertex closer to DFS root
    int descendant = -1; // vertex deeper in DFS tree
};

struct DfsInfo {
    int vertexCount = 0;

    // vertex -> DFS index
    std::vector<int> dfsIndex;

    // DFS index -> vertex
    std::vector<int> vertexAtDfsIndex;

    std::vector<int> parent;
    std::vector<int> parentEdgeId;

    // Inclusive maximum DFS index in each DFS subtree.
    // DFS preorder makes every subtree a contiguous interval.
    std::vector<int> subtreeEndDfi;

    std::vector<std::vector<int>> children;

    std::vector<int> treeEdgeIds;

    // All back edges stored once; always as descendant -> ancestor relation
    std::vector<DfsBackEdge> backEdges;

    // for BM main loop:
    // backEdgeIndicesFromAncestor[v] gives back eges (v, descendant)
    std::vector<std::vector<int>> backEdgeIndicesFromAncestor;

    // Direct ancestor reachable by one back edge from this vertex
    // if none exists, value is the vertex itself
    std::vector<int> leastAncestorDfi;
    std::vector<int> leastAncestorVertex;
    std::vector<int> leastAncestorEdgeId;

    // BM lowpoint: smallest DFS index reachable through
    // zero or more tree edges down + one back edge up
    std::vector<int> lowpointDfi;
    std::vector<int> lowpointVertex;

    // Children of each vertex ordered by lowpointDfi, built in linear time
    std::vector<std::vector<int>> childrenSortedByLowpoint;

    // DFS forest support
    std::vector<int> componentId;
    std::vector<int> roots;
};

class DfsPreprocessor {
public:
    DfsInfo run(const Graph& graph);

private:
    const Graph* graph_ = nullptr;
    DfsInfo info_;
    int nextDfsIndex_ = 0;
    int nextComponentId_ = 0;

    void initialize();
    void dfs(int v, int componentId);

    void registerBackEdge(int edgeId, int ancestor, int descendant);
    void updateLeastAncestor(int v, int ancestorVertex, int edgeId);
    void updateLowpoint(int v, int candidateDfi, int candidateVertex);

    void buildChildrenSortedByLowpoint();
};

} // namespace bm