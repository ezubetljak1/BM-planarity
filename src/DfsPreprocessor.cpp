#include "bm/DfsPreprocessor.hpp"

#include <stdexcept>

namespace bm {

DfsInfo DfsPreprocessor::run(const Graph& graph) {
    graph_ = &graph;
    initialize();

    for (int v = 0; v < graph.vertexCount(); v++) {
        if (info_.dfsIndex[v] == -1) {
            info_.roots.push_back(v);
            dfs(v, nextComponentId_);
            ++nextComponentId_;
        }
    }

    buildChildrenSortedByLowpoint();

    return info_;
}

void DfsPreprocessor::initialize() {
    const int n = graph_->vertexCount();

    info_ = DfsInfo();
    info_.vertexCount = n;

    info_.dfsIndex.assign(n, -1);
    info_.vertexAtDfsIndex.assign(n, -1);

    info_.parent.assign(n, -1);
    info_.parentEdgeId.assign(n, -1);
    info_.children.assign(n, {});

    info_.treeEdgeIds.clear();
    info_.backEdges.clear();
    info_.backEdgeIndicesFromAncestor.assign(n, {});

    info_.leastAncestorDfi.assign(n, -1);
    info_.leastAncestorVertex.assign(n, -1);
    info_.leastAncestorEdgeId.assign(n, -1);

    info_.lowpointDfi.assign(n, -1);
    info_.lowpointVertex.assign(n, -1);

    info_.childrenSortedByLowpoint.assign(n, {});
    info_.componentId.assign(n, -1);
    info_.roots.clear();

    nextDfsIndex_ = 0;
    nextComponentId_ = 0;
}

void DfsPreprocessor::dfs(int v, int componentId) {
    info_.dfsIndex[v] = nextDfsIndex_;
    info_.vertexAtDfsIndex[nextDfsIndex_] = v;
    ++nextDfsIndex_;

    info_.componentId[v] = componentId;

    info_.leastAncestorDfi[v] = info_.dfsIndex[v];
    info_.leastAncestorVertex[v] = v;
    info_.leastAncestorEdgeId[v] = -1;

    info_.lowpointDfi[v] = info_.dfsIndex[v];
    info_.lowpointVertex[v] = v;

    const auto& adjacency = graph_->adjacencyEdgeIds();

    for (int edgeId : adjacency[v]) {
        const int w = graph_ -> opposite(edgeId, v);

        if (info_.dfsIndex[w] == -1) {
            info_.parent[w] = v;
            info_.parentEdgeId[w] = edgeId;
            info_.children[v].push_back(w);
            info_.treeEdgeIds.push_back(edgeId);

            dfs(w, componentId);

            updateLowpoint(v, info_.lowpointDfi[w], info_.lowpointVertex[w]);
        } else if (edgeId != info_.parentEdgeId[v] && info_.dfsIndex[w] < info_.dfsIndex[v]) {
            // undirected non-tree edge is stored once;
            // from deeper vertex v to ancestor w
            registerBackEdge(edgeId, w, v);
            updateLeastAncestor(v, w, edgeId);
            updateLowpoint(v, info_.dfsIndex[w], w);
        }
    }
}

void DfsPreprocessor::registerBackEdge(int edgeId, int ancestor, int descendant) {
    const int index = info_.backEdges.size();

    DfsBackEdge backEdge;
    backEdge.edgeId = edgeId;
    backEdge.ancestor = ancestor;
    backEdge.descendant = descendant;

    info_.backEdges.push_back(backEdge);
    info_.backEdgeIndicesFromAncestor[ancestor].push_back(index);
}

void DfsPreprocessor::updateLeastAncestor(int v, int ancestorVertex, int edgeId) {
    const int ancestorDfi = info_.dfsIndex[ancestorVertex];

    if (ancestorDfi < info_.leastAncestorDfi[v]) {
        info_.leastAncestorDfi[v] = ancestorDfi;
        info_.leastAncestorVertex[v] = ancestorVertex;
        info_.leastAncestorEdgeId[v] = edgeId;
    }
}

void DfsPreprocessor::updateLowpoint(int v, int candidateDfi, int candidateVertex) {
    if (candidateDfi < info_.lowpointDfi[v]) {
        info_.lowpointDfi[v] = candidateDfi;
        info_.lowpointVertex[v] = candidateVertex;
    }
}

void DfsPreprocessor::buildChildrenSortedByLowpoint() {
    const int n = info_.vertexCount;

    std::vector<std::vector<int>> buckets(n);

    for (int child = 0; child < n; ++child) {
        if (info_.parent[child] == -1)
            continue;

        const int low = info_.lowpointDfi[child];

        if (low < 0 || low >= n)
            throw std::logic_error("Invalid lowpoint value.");

        buckets[low].push_back(child);
    }

    info_.childrenSortedByLowpoint.assign(n, {});

    for (int low = 0; low < n; ++low) {
        for (int child : buckets[low]) {
            const int parent = info_.parent[child];
            info_.childrenSortedByLowpoint[parent].push_back(child);
        }
    }
}

} // namespace bm