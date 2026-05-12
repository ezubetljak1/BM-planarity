#include "bm/BmPartialEmbedding.hpp"

#include <stdexcept>

namespace bm {

BmPartialEmbedding::BmPartialEmbedding(int originalVertexCount)
    : originalToInternalVertex_(originalVertexCount, -1) {
    if (originalVertexCount < 0)
        throw std::invalid_argument("Original vertex count cannot be negative.");

    internalVertices_.reserve(originalVertexCount);

    for (int vertex = 0; vertex < originalVertexCount; ++vertex) {
        BmInternalVertex internalVertex;
        internalVertex.id = internalVertices_.size();
        internalVertex.kind = BmInternalVertexKind::Original;
        internalVertex.originalVertex = vertex;
        internalVertex.bicompRootId = -1;

        internalVertices_.push_back(internalVertex);
        originalToInternalVertex_[vertex] = internalVertex.id;
    }
}

int BmPartialEmbedding::internalVertexCount() const {
    return internalVertices_.size();
}

int BmPartialEmbedding::embeddedEdgeCount() const {
    return embeddedEdges_.size();
}

int BmPartialEmbedding::halfEdgeCount() const {
    return halfEdges_.size();
}

int BmPartialEmbedding::originalInternalVertex(int originalVertex) const {
    validateOriginalVertex(originalVertex);
    return originalToInternalVertex_[originalVertex];
}

const BmInternalVertex& BmPartialEmbedding::internalVertex(int internalVertexId) const {
    validateInternalVertex(internalVertexId);
    return internalVertices_[internalVertexId];
}

const BmEmbeddedEdge& BmPartialEmbedding::embeddedEdge(int embeddedEdgeId) const {
    validateEmbeddedEdge(embeddedEdgeId);
    return embeddedEdges_[embeddedEdgeId];
}

const BmHalfEdge& BmPartialEmbedding::halfEdge(int halfEdgeId) const {
    validateHalfEdge(halfEdgeId);
    return halfEdges_[halfEdgeId];
}

BmTreeBicompEmbedding BmPartialEmbedding::createTreeEdgeBicomp(int bicompRootId, int parentVertex,
                                                               int childVertex,
                                                               int originalTreeEdgeId) {
    validateOriginalVertex(parentVertex);
    validateOriginalVertex(childVertex);

    if (originalTreeEdgeId < 0)
        throw std::invalid_argument("Original tree edge id cannot be negative.");

    const int rootInternalVertexId = createBicompRootVertex(bicompRootId, parentVertex);
    const int childInternalVertexId = originalInternalVertex(childVertex);

    const int embeddedEdgeId = embeddedEdges_.size();
    const int rootToChildHalfEdgeId = halfEdges_.size();
    const int childToRootHalfEdgeId = rootToChildHalfEdgeId + 1;

    BmEmbeddedEdge embeddedEdge;
    embeddedEdge.id = embeddedEdgeId;
    embeddedEdge.originalEdgeId = originalTreeEdgeId;
    embeddedEdge.halfEdgeA = rootToChildHalfEdgeId;
    embeddedEdge.halfEdgeB = childToRootHalfEdgeId;
    embeddedEdge.shortCircuit = false;

    BmHalfEdge rootToChild;
    rootToChild.id = rootToChildHalfEdgeId;
    rootToChild.embeddedEdgeId = embeddedEdgeId;
    rootToChild.from = rootInternalVertexId;
    rootToChild.to = childInternalVertexId;
    rootToChild.twin = childToRootHalfEdgeId;
    rootToChild.nextOnExternalFace = childToRootHalfEdgeId;
    rootToChild.previousOnExternalFace = childToRootHalfEdgeId;

    BmHalfEdge childToRoot;
    childToRoot.id = childToRootHalfEdgeId;
    childToRoot.embeddedEdgeId = embeddedEdgeId;
    childToRoot.from = childInternalVertexId;
    childToRoot.to = rootInternalVertexId;
    childToRoot.twin = rootToChildHalfEdgeId;
    childToRoot.nextOnExternalFace = rootToChildHalfEdgeId;
    childToRoot.previousOnExternalFace = rootToChildHalfEdgeId;

    embeddedEdges_.push_back(embeddedEdge);
    halfEdges_.push_back(rootToChild);
    halfEdges_.push_back(childToRoot);

    internalVertices_[rootInternalVertexId].externalFaceHalfEdges = {rootToChildHalfEdgeId,
                                                                     rootToChildHalfEdgeId};

    internalVertices_[childInternalVertexId].externalFaceHalfEdges = {childToRootHalfEdgeId,
                                                                      childToRootHalfEdgeId};

    BmTreeBicompEmbedding result;
    result.rootInternalVertexId = rootInternalVertexId;
    result.childInternalVertexId = childInternalVertexId;
    result.embeddedEdgeId = embeddedEdgeId;
    result.rootToChildHalfEdgeId = rootToChildHalfEdgeId;
    result.childToRootHalfEdgeId = childToRootHalfEdgeId;

    return result;
}

std::vector<int> BmPartialEmbedding::externalFaceVertices(int startHalfEdgeId, int maxSteps) const {
    validateHalfEdge(startHalfEdgeId);

    if (maxSteps < 0)
        throw std::invalid_argument("Maximum number of steps cannot be negative.");

    std::vector<int> vertices;

    int currentHalfEdgeId = startHalfEdgeId;

    for (int step = 0; step < maxSteps; ++step) {
        const BmHalfEdge& current = halfEdge(currentHalfEdgeId);

        vertices.push_back(current.from);

        currentHalfEdgeId = current.nextOnExternalFace;

        if (currentHalfEdgeId == startHalfEdgeId)
            break;
    }

    return vertices;
}

int BmPartialEmbedding::createBicompRootVertex(int bicompRootId, int parentVertex) {
    validateOriginalVertex(parentVertex);

    if (bicompRootId < 0)
        throw std::invalid_argument("Bicomp root id cannot be negative.");

    BmInternalVertex rootVertex;
    rootVertex.id = internalVertices_.size();
    rootVertex.kind = BmInternalVertexKind::BicompRoot;
    rootVertex.originalVertex = parentVertex;
    rootVertex.bicompRootId = bicompRootId;

    internalVertices_.push_back(rootVertex);

    return rootVertex.id;
}

int BmPartialEmbedding::nextOnExternalFace(int halfEdgeId) const {
    validateHalfEdge(halfEdgeId);
    return halfEdges_[halfEdgeId].nextOnExternalFace;
}

int BmPartialEmbedding::previousOnExternalFace(int halfEdgeId) const {
    validateHalfEdge(halfEdgeId);
    return halfEdges_[halfEdgeId].previousOnExternalFace;
}

int BmPartialEmbedding::twinHalfEdge(int halfEdgeId) const {
    validateHalfEdge(halfEdgeId);
    return halfEdges_[halfEdgeId].twin;
}

int BmPartialEmbedding::externalFaceHalfEdge(int internalVertexId, int side) const {
    validateInternalVertex(internalVertexId);

    if (side < 0 || side > 1)
        throw std::out_of_range("External face side must be 0 or 1.");

    return internalVertices_[internalVertexId].externalFaceHalfEdges[side];
}

int BmPartialEmbedding::externalFaceLinkIndex(int internalVertexId, int halfEdgeId) const {
    validateInternalVertex(internalVertexId);
    validateHalfEdge(halfEdgeId);

    const auto& links = internalVertices_[internalVertexId].externalFaceHalfEdges;

    if (links[0] == halfEdgeId)
        return 0;

    if (links[1] == halfEdgeId)
        return 1;

    throw std::invalid_argument("Half-edge is not an external-face link of this vertex.");
}

bool BmPartialEmbedding::isBicompRootVertex(int internalVertexId) const {
    validateInternalVertex(internalVertexId);

    return internalVertices_[internalVertexId].kind == BmInternalVertexKind::BicompRoot;
}

int BmPartialEmbedding::oppositeExternalFaceHalfEdge(int internalVertexId, int halfEdgeId) const {
    const int linkIndex = externalFaceLinkIndex(internalVertexId, halfEdgeId);
    const int oppositeIndex = 1 - linkIndex;

    return internalVertices_[internalVertexId].externalFaceHalfEdges[oppositeIndex];
}

int BmPartialEmbedding::bicompRootIdForInternalVertex(int internalVertexId) const {
    validateInternalVertex(internalVertexId);

    const auto& vertex = internalVertices_[internalVertexId];

    if (vertex.kind != BmInternalVertexKind::BicompRoot)
        throw std::invalid_argument("Internal vertex is not a bicomp root vertex.");

    return vertex.bicompRootId;
}

int BmPartialEmbedding::originalVertexForInternalVertex(int internalVertexId) const {
    validateInternalVertex(internalVertexId);

    return internalVertices_[internalVertexId].originalVertex;
}

void BmPartialEmbedding::setExternalFaceHalfEdges(int internalVertexId, int firstHalfEdgeId,
                                                  int secondHalfEdgeId) {
    validateInternalVertex(internalVertexId);

    if (firstHalfEdgeId != -1)
        validateHalfEdge(firstHalfEdgeId);

    if (secondHalfEdgeId != -1)
        validateHalfEdge(secondHalfEdgeId);

    internalVertices_[internalVertexId].externalFaceHalfEdges = {firstHalfEdgeId, secondHalfEdgeId};
}

void BmPartialEmbedding::linkExternalFaceHalfEdges(int fromHalfEdgeId, int toHalfEdgeId) {
    validateHalfEdge(fromHalfEdgeId);
    validateHalfEdge(toHalfEdgeId);

    halfEdges_[fromHalfEdgeId].nextOnExternalFace = toHalfEdgeId;
    halfEdges_[toHalfEdgeId].previousOnExternalFace = fromHalfEdgeId;
}

void BmPartialEmbedding::validateOriginalVertex(int vertex) const {
    if (vertex < 0 || vertex >= originalToInternalVertex_.size())
        throw std::out_of_range("Invalid original vertex id.");
}

void BmPartialEmbedding::validateInternalVertex(int internalVertexId) const {
    if (internalVertexId < 0 || internalVertexId >= internalVertices_.size())
        throw std::out_of_range("Invalid internal vertex id.");
}

void BmPartialEmbedding::validateEmbeddedEdge(int embeddedEdgeId) const {
    if (embeddedEdgeId < 0 || embeddedEdgeId >= embeddedEdges_.size())
        throw std::out_of_range("Invalid embedded edge id.");
}

void BmPartialEmbedding::validateHalfEdge(int halfEdgeId) const {
    if (halfEdgeId < 0 || halfEdgeId >= halfEdges_.size())
        throw std::out_of_range("Invalid half-edge id.");
}

} // namespace bm