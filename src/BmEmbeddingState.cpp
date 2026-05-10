#include "bm/BmEmbeddingState.hpp"

#include <stdexcept>

namespace bm {

BmEmbeddingState::BmEmbeddingState(const Graph& graph, const DfsInfo& dfsInfo)
    : graph_(&graph), dfsInfo_(&dfsInfo), vertexStates_(graph.vertexCount()),
      separatedDfsChildLists_(dfsInfo), partialEmbedding_(graph.vertexCount()),
      rootForChild_(graph.vertexCount(), -1), childRoots_(graph.vertexCount()) {

    for (int v = 0; v < graph.vertexCount(); ++v)
        vertexStates_[v].vertex = v;
}

const Graph& BmEmbeddingState::graph() const {
    return *graph_;
}

const DfsInfo& BmEmbeddingState::dfsInfo() const {
    return *dfsInfo_;
}

BmVertexState& BmEmbeddingState::vertexState(int vertex) {
    validateVertex(vertex);
    return vertexStates_[vertex];
}

const BmVertexState& BmEmbeddingState::vertexState(int vertex) const {
    validateVertex(vertex);
    return vertexStates_[vertex];
}

const BmPartialEmbedding& BmEmbeddingState::partialEmbedding() const {
    return partialEmbedding_;
}

bool BmEmbeddingState::isExternallyActive(int vertex, int currentVertex) const {
    validateVertex(vertex);
    validateVertex(currentVertex);

    const int currentDfi = dfsInfo_->dfsIndex[currentVertex];

    if (dfsInfo_->leastAncestorDfi[vertex] < currentDfi)
        return true;

    const int firstChild = separatedDfsChildLists_.frontChild(vertex);

    if (firstChild != -1 && dfsInfo_->lowpointDfi[firstChild] < currentDfi)
        return true;

    return false;
}

int BmEmbeddingState::firstSeparatedDfsChild(int vertex) const {
    validateVertex(vertex);
    return separatedDfsChildLists_.frontChild(vertex);
}

std::vector<int> BmEmbeddingState::separatedDfsChildren(int vertex) const {
    validateVertex(vertex);
    return separatedDfsChildLists_.toVector(vertex);
}

void BmEmbeddingState::removeSeparatedDfsChild(int parent, int child) {
    validateVertex(parent);
    validateVertex(child);
    separatedDfsChildLists_.removeChild(parent, child);
}

int BmEmbeddingState::createTreeEdgeBicomp(int parentVertex, int childVertex) {
    validateVertex(parentVertex);
    validateVertex(childVertex);

    if (dfsInfo_->parent[childVertex] != parentVertex)
        throw std::invalid_argument("Vertices are not in a DFS parent-child relation.");

    const int treeEdgeId = dfsInfo_->parentEdgeId[childVertex];

    if (treeEdgeId == -1)
        throw std::invalid_argument("Child vertex does not have a DFS parent edge.");

    int& existingRoot = rootForChild_[childVertex];

    if (existingRoot != -1)
        return existingRoot;

    BmBicompRoot root;
    root.id = bicompRoots_.size();
    root.parentVertex = parentVertex;
    root.childVertex = childVertex;
    root.treeEdgeId = treeEdgeId;
    root.active = true;
    root.rootEdgeSign = 1;

    BmTreeBicompEmbedding treeEmbedding =
        partialEmbedding_.createTreeEdgeBicomp(root.id, parentVertex, childVertex, treeEdgeId);

    root.internalRootVertexId = treeEmbedding.rootInternalVertexId;
    root.internalChildVertexId = treeEmbedding.childInternalVertexId;
    root.embeddedTreeEdgeId = treeEmbedding.embeddedEdgeId;
    root.rootToChildHalfEdgeId = treeEmbedding.rootToChildHalfEdgeId;
    root.childToRootHalfEdgeId = treeEmbedding.childToRootHalfEdgeId;

    bicompRoots_.push_back(root);

    existingRoot = root.id;
    childRoots_[parentVertex].push_back(root.id);

    return root.id;
}

int BmEmbeddingState::bicompRootCount() const {
    return bicompRoots_.size();
}

const BmBicompRoot& BmEmbeddingState::bicompRoot(int rootId) const {
    if (rootId < 0 || rootId >= bicompRoots_.size())
        throw std::out_of_range("Invalid bicomp root id.");

    return bicompRoots_[rootId];
}

int BmEmbeddingState::rootForChild(int childVertex) const {
    validateVertex(childVertex);
    return rootForChild_[childVertex];
}

const std::vector<int>& BmEmbeddingState::childRoots(int vertex) const {
    validateVertex(vertex);
    return childRoots_[vertex];
}

void BmEmbeddingState::validateVertex(int vertex) const {
    if (vertex < 0 || vertex >= vertexStates_.size())
        throw std::out_of_range("Invalid vertex id.");
}

} // namespace bm