#include "bm/BmEmbeddingState.hpp"

#include <stdexcept>

namespace bm {

BmEmbeddingState::BmEmbeddingState(const Graph& graph, const DfsInfo& dfsInfo)
    : graph_(&graph), dfsInfo_(&dfsInfo), vertexStates_(graph.vertexCount()),
      separatedDfsChildLists_(dfsInfo), partialEmbedding_(graph.vertexCount()),
      rootForChild_(graph.vertexCount(), -1), childRoots_(graph.vertexCount()),
      internalVertexVisitedInStep_(graph.vertexCount(), -1),
      embeddedEdgeIdByOriginalEdge_(graph.edgeCount(), -1),
      pendingBackedgeOriginalEdgeId_(graph.vertexCount(), -1) {

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

BmPartialEmbedding& BmEmbeddingState::partialEmbedding() {
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

    registerEmbeddedOriginalEdge(treeEdgeId, treeEmbedding.embeddedEdgeId);

    ensureInternalVisitedCapacity();
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

BmBicompRoot& BmEmbeddingState::bicompRoot(int rootId) {
    if (rootId < 0 || rootId >= bicompRoots_.size())
        throw std::out_of_range("Invalid bicomp root id.");

    return bicompRoots_[rootId];
}

void BmEmbeddingState::deactivateBicompRoot(int rootId) {
    BmBicompRoot& root = bicompRoot(rootId);

    if (!root.active)
        throw std::logic_error("Bicomp root is already inactive.");

    root.active = false;
}

void BmEmbeddingState::removeExpectedFirstPertinentRoot(int vertex, int expectedRootId) {
    validateVertex(vertex);

    auto& roots = vertexStates_[vertex].pertinentRoots;

    if (roots.empty())
        throw std::logic_error("Vertex has no pertinent roots.");

    if (roots.front() != expectedRootId)
        throw std::logic_error("Unexpected pertinent root at front of list.");

    roots.pop_front();
}

int BmEmbeddingState::rootForChild(int childVertex) const {
    validateVertex(childVertex);
    return rootForChild_[childVertex];
}

const std::vector<int>& BmEmbeddingState::childRoots(int vertex) const {
    validateVertex(vertex);
    return childRoots_[vertex];
}

bool BmEmbeddingState::hasBackedgeFlag(int vertex, int currentVertex) const {
    validateVertex(vertex);
    validateVertex(currentVertex);

    return vertexStates_[vertex].backedgeFlagDfi == dfsInfo_->dfsIndex[currentVertex];
}

void BmEmbeddingState::clearBackedgeFlag(int vertex) {
    validateVertex(vertex);
    vertexStates_[vertex].backedgeFlagDfi = -1;
    pendingBackedgeOriginalEdgeId_[vertex] = -1;
}

bool BmEmbeddingState::isVisitedInStep(int vertex, int currentVertex) const {
    validateVertex(vertex);
    validateVertex(currentVertex);

    const int currentDfi = dfsInfo_->dfsIndex[currentVertex];

    return vertexStates_[vertex].visitedInStep == currentDfi;
}

void BmEmbeddingState::markVisitedInStep(int vertex, int currentVertex) {
    validateVertex(vertex);
    validateVertex(currentVertex);

    const int currentDfi = dfsInfo_->dfsIndex[currentVertex];

    vertexStates_[vertex].visitedInStep = currentDfi;
}

bool BmEmbeddingState::isPertinent(int vertex, int currentVertex) const {
    validateVertex(vertex);
    validateVertex(currentVertex);

    const BmVertexState& state = vertexStates_[vertex];

    return state.backedgeFlagDfi == dfsInfo_->dfsIndex[currentVertex] ||
           !state.pertinentRoots.empty();
}

bool BmEmbeddingState::isInternallyActive(int vertex, int currentVertex) const {
    validateVertex(vertex);
    validateVertex(currentVertex);

    return isPertinent(vertex, currentVertex) && !isExternallyActive(vertex, currentVertex);
}

bool BmEmbeddingState::isInactive(int vertex, int currentVertex) const {
    validateVertex(vertex);
    validateVertex(currentVertex);

    return !isPertinent(vertex, currentVertex) && !isExternallyActive(vertex, currentVertex);
}

bool BmEmbeddingState::isRootExternallyActive(int rootId, int currentVertex) const {
    validateVertex(currentVertex);

    const BmBicompRoot& root = bicompRoot(rootId);

    const int child = root.childVertex;
    const int currentDfi = dfsInfo_->dfsIndex[currentVertex];

    return dfsInfo_->lowpointDfi[child] < currentDfi;
}

void BmEmbeddingState::addPertinentRoot(int vertex, int rootId, int currentVertex) {
    validateVertex(vertex);
    validateVertex(currentVertex);

    const BmBicompRoot& root = bicompRoot(rootId);

    if (!root.active)
        throw std::invalid_argument("Cannot add inactive bicomp root as pertinent.");

    if (isRootExternallyActive(rootId, currentVertex))
        vertexStates_[vertex].pertinentRoots.push_back(rootId);
    else
        vertexStates_[vertex].pertinentRoots.push_front(rootId);
}

bool BmEmbeddingState::hasPertinentRoots(int vertex) const {
    validateVertex(vertex);

    return !vertexStates_[vertex].pertinentRoots.empty();
}

int BmEmbeddingState::firstPertinentRoot(int vertex) const {
    validateVertex(vertex);

    const auto& roots = vertexStates_[vertex].pertinentRoots;

    if (roots.empty())
        return -1;

    return roots.front();
}

void BmEmbeddingState::removeFirstPertinentRoot(int vertex) {
    validateVertex(vertex);

    auto& roots = vertexStates_[vertex].pertinentRoots;

    if (roots.empty())
        throw std::logic_error("Vertex has no pertinent roots.");

    roots.pop_front();
}

bool BmEmbeddingState::isInternalBicompRootVertex(int internalVertexId) const {
    return partialEmbedding_.isBicompRootVertex(internalVertexId);
}

int BmEmbeddingState::bicompRootIdForInternalVertex(int internalVertexId) const {
    return partialEmbedding_.bicompRootIdForInternalVertex(internalVertexId);
}

int BmEmbeddingState::originalVertexForInternalVertex(int internalVertexId) const {
    return partialEmbedding_.originalVertexForInternalVertex(internalVertexId);
}

void BmEmbeddingState::validateVertex(int vertex) const {
    if (vertex < 0 || vertex >= vertexStates_.size())
        throw std::out_of_range("Invalid vertex id.");
}

bool BmEmbeddingState::isInternalVertexVisitedInStep(int internalVertexId,
                                                     int currentVertex) const {
    validateInternalVertex(internalVertexId);
    validateVertex(currentVertex);

    const int currentDfi = dfsInfo_->dfsIndex[currentVertex];

    return internalVertexVisitedInStep_[internalVertexId] == currentDfi;
}

void BmEmbeddingState::markInternalVertexVisitedInStep(int internalVertexId, int currentVertex) {
    validateInternalVertex(internalVertexId);
    validateVertex(currentVertex);

    const int currentDfi = dfsInfo_->dfsIndex[currentVertex];

    internalVertexVisitedInStep_[internalVertexId] = currentDfi;
}

void BmEmbeddingState::ensureInternalVisitedCapacity() {
    const int neededSize = partialEmbedding_.internalVertexCount();

    if (internalVertexVisitedInStep_.size() < neededSize) {
        internalVertexVisitedInStep_.resize(neededSize, -1);
    }
}

void BmEmbeddingState::validateInternalVertex(int internalVertexId) const {
    if (internalVertexId < 0 || internalVertexId >= partialEmbedding_.internalVertexCount()) {
        throw std::out_of_range("Invalid internal vertex id.");
    }
}

bool BmEmbeddingState::isBackedgeEndpointForCurrentVertex(int vertex, int currentVertex) const {
    return hasBackedgeFlag(vertex, currentVertex);
}

bool BmEmbeddingState::isOriginalEdgeEmbedded(int originalEdgeId) const {
    if (originalEdgeId < 0 || originalEdgeId >= embeddedEdgeIdByOriginalEdge_.size())
        throw std::out_of_range("Invalid original edge id.");

    return embeddedEdgeIdByOriginalEdge_[originalEdgeId] != -1;
}

int BmEmbeddingState::embeddedEdgeIdForOriginalEdge(int originalEdgeId) const {
    if (originalEdgeId < 0 || originalEdgeId >= embeddedEdgeIdByOriginalEdge_.size())
        throw std::out_of_range("Invalid original edge id.");

    return embeddedEdgeIdByOriginalEdge_[originalEdgeId];
}

void BmEmbeddingState::registerEmbeddedOriginalEdge(int originalEdgeId, int embeddedEdgeId) {
    if (originalEdgeId < 0 || originalEdgeId >= embeddedEdgeIdByOriginalEdge_.size())
        throw std::out_of_range("Invalid original edge id.");

    if (embeddedEdgeId < 0 || embeddedEdgeId >= partialEmbedding_.embeddedEdgeCount())
        throw std::out_of_range("Invalid embedded edge id.");

    int& stored = embeddedEdgeIdByOriginalEdge_[originalEdgeId];

    if (stored != -1 && stored != embeddedEdgeId)
        throw std::logic_error("Original edge is already registered as embedded.");

    stored = embeddedEdgeId;
}

void BmEmbeddingState::markBackedgeFlag(int vertex, int currentVertex) {
    markBackedgeFlag(vertex, currentVertex, -1);
}

void BmEmbeddingState::markBackedgeFlag(int vertex, int currentVertex, int originalEdgeId) {
    validateVertex(vertex);
    validateVertex(currentVertex);

    if (originalEdgeId != -1) {
        (void)graph_->edge(originalEdgeId);
    }

    vertexStates_[vertex].backedgeFlagDfi = dfsInfo_->dfsIndex[currentVertex];

    pendingBackedgeOriginalEdgeId_[vertex] = originalEdgeId;
}

int BmEmbeddingState::pendingBackedgeOriginalEdgeId(int vertex, int currentVertex) const {
    validateVertex(vertex);
    validateVertex(currentVertex);

    if (!hasBackedgeFlag(vertex, currentVertex))
        throw std::logic_error("Vertex is not a pending back-edge endpoint for this step.");

    const int edgeId = pendingBackedgeOriginalEdgeId_[vertex];

    if (edgeId == -1) {
        throw std::logic_error("Pending back-edge endpoint has no original edge id.");
    }

    return edgeId;
}

} // namespace bm