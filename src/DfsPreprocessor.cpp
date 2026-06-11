#include "bm/DfsPreprocessor.hpp"

#include <cstddef>
#include <stdexcept>
#include <vector>

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

void DfsPreprocessor::dfs(int startVertex, int componentId) {
    struct DfsFrame {
        int vertex = -1;
        std::size_t nextAdjacencyIndex = 0;
    };

    const auto& adjacency = graph_->adjacencyEdgeIds();

    auto discoverVertex = [&](int vertex) {
        info_.dfsIndex[vertex] = nextDfsIndex_;
        info_.vertexAtDfsIndex[nextDfsIndex_] = vertex;
        ++nextDfsIndex_;

        info_.componentId[vertex] = componentId;

        info_.leastAncestorDfi[vertex] = info_.dfsIndex[vertex];
        info_.leastAncestorVertex[vertex] = vertex;
        info_.leastAncestorEdgeId[vertex] = -1;

        info_.lowpointDfi[vertex] = info_.dfsIndex[vertex];
        info_.lowpointVertex[vertex] = vertex;
    };

    std::vector<DfsFrame> stack;

    discoverVertex(startVertex);
    stack.push_back({startVertex, 0});

    while (!stack.empty()) {
        DfsFrame& frame = stack.back();
        const int vertex = frame.vertex;

        if (frame.nextAdjacencyIndex >= adjacency[vertex].size()) {
            stack.pop_back();

            const int parent = info_.parent[vertex];

            if (parent != -1) {
                updateLowpoint(parent, info_.lowpointDfi[vertex], info_.lowpointVertex[vertex]);
            }

            continue;
        }

        const int edgeId = adjacency[vertex][frame.nextAdjacencyIndex];
        ++frame.nextAdjacencyIndex;

        const int neighbor = graph_->opposite(edgeId, vertex);

        if (info_.dfsIndex[neighbor] == -1) {
            info_.parent[neighbor] = vertex;
            info_.parentEdgeId[neighbor] = edgeId;
            info_.children[vertex].push_back(neighbor);
            info_.treeEdgeIds.push_back(edgeId);

            discoverVertex(neighbor);
            stack.push_back({neighbor, 0});
        } else if (edgeId != info_.parentEdgeId[vertex] &&
                   info_.dfsIndex[neighbor] < info_.dfsIndex[vertex]) {
            // Undirected non-tree edge is stored once, from descendant to ancestor.
            registerBackEdge(edgeId, neighbor, vertex);
            updateLeastAncestor(vertex, neighbor, edgeId);
            updateLowpoint(vertex, info_.dfsIndex[neighbor], neighbor);
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