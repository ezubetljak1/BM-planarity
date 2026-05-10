#include "bm/BmEmbeddingState.hpp"

#include <stdexcept>

namespace bm {

BmEmbeddingState::BmEmbeddingState(const Graph& graph, const DfsInfo& dfsInfo)
    : graph_(&graph), dfsInfo_(&dfsInfo), vertexStates_(graph.vertexCount()) {

    for (int v = 0; v < graph.vertexCount(); ++v) {
        vertexStates_[v].vertex = v;
        vertexStates_[v].separatedDfsChildList = dfsInfo.childrenSortedByLowpoint[v];
    }
}

const Graph& BmEmbeddingState::graph() const {
    return *graph_;
}

const DfsInfo& BmEmbeddingState::dfsInfo() const {
    return *dfsInfo_;
}

BmVertexState& BmEmbeddingState::vertexState(int vertex) {
    if (vertex < 0 || vertex >= vertexStates_.size())
        throw std::out_of_range("Invalid vertex id.");

    return vertexStates_[vertex];
}

const BmVertexState& BmEmbeddingState::vertexState(int vertex) const {
    if (vertex < 0 || vertex >= static_cast<int>(vertexStates_.size())) {
        throw std::out_of_range("Invalid vertex id.");
    }

    return vertexStates_[vertex];
}

bool BmEmbeddingState::isExternallyActive(int vertex, int currentVertex) const {
    const int currentDfi = dfsInfo_->dfsIndex[currentVertex];

    // Case 1: vertex has a direct back edge to an ancestor of currentVertex.
    if (dfsInfo_->leastAncestorDfi[vertex] < currentDfi)
        return true;

    // Case 2: vertex has a separated DFS child whose lowpoint reaches
    // above currentVertex.
    const auto& separatedChildren = vertexStates_[vertex].separatedDfsChildList;

    if (!separatedChildren.empty()) {
        const int firstChild = separatedChildren.front();

        if (dfsInfo_->lowpointDfi[firstChild] < currentDfi)
            return true;
    }

    return false;
}

} // namespace bm