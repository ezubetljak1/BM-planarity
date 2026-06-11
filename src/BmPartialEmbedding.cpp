#include "bm/BmPartialEmbedding.hpp"

#include <stdexcept>
#include <string>

namespace bm {

namespace {
int validateOriginalVertexCount(int originalVertexCount) {
    if (originalVertexCount < 0)
        throw std::invalid_argument("Original vertex count cannot be negative.");

    return originalVertexCount;
}
} // namespace

BmPartialEmbedding::BmPartialEmbedding(int originalVertexCount)
    : originalToInternalVertex_(validateOriginalVertexCount(originalVertexCount), -1) {

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

BmInternalVertex& BmPartialEmbedding::internalVertex(int internalVertexId) {
    validateInternalVertex(internalVertexId);
    return internalVertices_[internalVertexId];
}

const BmEmbeddedEdge& BmPartialEmbedding::embeddedEdge(int embeddedEdgeId) const {
    validateEmbeddedEdge(embeddedEdgeId);
    return embeddedEdges_[embeddedEdgeId];
}

BmEmbeddedEdge& BmPartialEmbedding::embeddedEdge(int embeddedEdgeId) {
    validateEmbeddedEdge(embeddedEdgeId);
    return embeddedEdges_[embeddedEdgeId];
}

const BmHalfEdge& BmPartialEmbedding::halfEdge(int halfEdgeId) const {
    validateHalfEdge(halfEdgeId);
    return halfEdges_[halfEdgeId];
}

BmHalfEdge& BmPartialEmbedding::halfEdge(int halfEdgeId) {
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

    const int embeddedEdgeId =
        addEmbeddedEdge(rootInternalVertexId, childInternalVertexId, originalTreeEdgeId, false);

    const BmEmbeddedEdge& edge = embeddedEdges_[embeddedEdgeId];

    const int rootToChildHalfEdgeId = edge.halfEdgeA;
    const int childToRootHalfEdgeId = edge.halfEdgeB;

    // Initial tree bicomp behaves as a two-sided external-face cycle.
    linkExternalFaceHalfEdges(rootToChildHalfEdgeId, childToRootHalfEdgeId);

    linkExternalFaceHalfEdges(childToRootHalfEdgeId, rootToChildHalfEdgeId);

    setExternalFaceHalfEdges(rootInternalVertexId, rootToChildHalfEdgeId, rootToChildHalfEdgeId);

    setExternalFaceHalfEdges(childInternalVertexId, childToRootHalfEdgeId, childToRootHalfEdgeId);

    setExternalFaceNeighbors(rootInternalVertexId, childInternalVertexId, childInternalVertexId);

    setExternalFaceNeighbors(childInternalVertexId, rootInternalVertexId, rootInternalVertexId);

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

int BmPartialEmbedding::addEmbeddedEdge(int fromInternalVertexId, int toInternalVertexId,
                                        int originalEdgeId, bool shortCircuit) {
    const int embeddedEdgeId = createDetachedEmbeddedEdge(fromInternalVertexId, toInternalVertexId,
                                                          originalEdgeId, shortCircuit);

    const BmEmbeddedEdge& edge = embeddedEdges_[embeddedEdgeId];

    insertHalfEdgeIntoAdjacency(fromInternalVertexId, edge.halfEdgeA);

    insertHalfEdgeIntoAdjacency(toInternalVertexId, edge.halfEdgeB);

    return embeddedEdgeId;
}

bool BmPartialEmbedding::adjacencyEmpty(int internalVertexId) const {
    validateInternalVertex(internalVertexId);

    return internalVertices_[internalVertexId].firstIncidentHalfEdge == -1;
}

int BmPartialEmbedding::firstIncidentHalfEdge(int internalVertexId) const {
    validateInternalVertex(internalVertexId);

    return internalVertices_[internalVertexId].firstIncidentHalfEdge;
}

int BmPartialEmbedding::nextAroundVertex(int halfEdgeId) const {
    validateHalfEdge(halfEdgeId);

    return halfEdges_[halfEdgeId].nextAroundVertex;
}

int BmPartialEmbedding::previousAroundVertex(int halfEdgeId) const {
    validateHalfEdge(halfEdgeId);

    return halfEdges_[halfEdgeId].previousAroundVertex;
}

void BmPartialEmbedding::insertHalfEdgeIntoAdjacency(int internalVertexId, int halfEdgeId) {
    validateInternalVertex(internalVertexId);
    validateHalfEdge(halfEdgeId);

    BmInternalVertex& vertex = internalVertices_[internalVertexId];
    BmHalfEdge& inserted = halfEdges_[halfEdgeId];

    if (inserted.from != internalVertexId)
        throw std::logic_error(
            "Half-edge must be inserted into the adjacency list of its source vertex.");

    if (inserted.nextAroundVertex != -1 || inserted.previousAroundVertex != -1)
        throw std::logic_error("Half-edge is already linked into an adjacency list.");

    const int first = vertex.firstIncidentHalfEdge;

    if (first == -1) {
        inserted.nextAroundVertex = halfEdgeId;
        inserted.previousAroundVertex = halfEdgeId;

        vertex.firstIncidentHalfEdge = halfEdgeId;
        return;
    }

    const int last = halfEdges_[first].previousAroundVertex;

    inserted.nextAroundVertex = first;
    inserted.previousAroundVertex = last;

    halfEdges_[last].nextAroundVertex = halfEdgeId;
    halfEdges_[first].previousAroundVertex = halfEdgeId;
}

void BmPartialEmbedding::redirectHalfEdgeEndpoint(int halfEdgeId, int newInternalVertexId) {
    validateHalfEdge(halfEdgeId);
    validateInternalVertex(newInternalVertexId);

    BmHalfEdge& outgoing = halfEdges_[halfEdgeId];
    BmHalfEdge& incomingTwin = halfEdges_[outgoing.twin];

    outgoing.from = newInternalVertexId;
    incomingTwin.to = newInternalVertexId;
}

void BmPartialEmbedding::redirectAdjacencyToVertex(int sourceInternalVertexId,
                                                   int targetInternalVertexId) {
    validateInternalVertex(sourceInternalVertexId);
    validateInternalVertex(targetInternalVertexId);

    const int first = internalVertices_[sourceInternalVertexId].firstIncidentHalfEdge;

    if (first == -1)
        return;

    int current = first;

    do {
        const int next = halfEdges_[current].nextAroundVertex;

        redirectHalfEdgeEndpoint(current, targetInternalVertexId);

        current = next;
    } while (current != first);
}

void BmPartialEmbedding::spliceAdjacencyLists(int targetInternalVertexId,
                                              int sourceInternalVertexId, int targetLinkIndex,
                                              int sourceLinkIndex) {
    validateInternalVertex(targetInternalVertexId);
    validateInternalVertex(sourceInternalVertexId);

    if (targetLinkIndex < 0 || targetLinkIndex > 1)
        throw std::out_of_range("Target link index must be 0 or 1.");

    if (sourceLinkIndex < 0 || sourceLinkIndex > 1)
        throw std::out_of_range("Source link index must be 0 or 1.");

    BmInternalVertex& target = internalVertices_[targetInternalVertexId];

    BmInternalVertex& source = internalVertices_[sourceInternalVertexId];

    if (source.firstIncidentHalfEdge == -1)
        return;

    if (target.firstIncidentHalfEdge == -1) {
        target.firstIncidentHalfEdge = source.firstIncidentHalfEdge;

        target.externalFaceHalfEdges = source.externalFaceHalfEdges;

        source.firstIncidentHalfEdge = -1;
        source.externalFaceHalfEdges = {-1, -1};
        source.externalFaceNeighbors = {-1, -1};

        return;
    }

    const int targetAnchor = target.externalFaceHalfEdges[targetLinkIndex];

    const int sourceAnchor = source.externalFaceHalfEdges[sourceLinkIndex];

    if (targetAnchor == -1 || sourceAnchor == -1)
        throw std::logic_error("Cannot splice adjacency lists without external-face anchors.");

    const int targetNeighbor = adjacencyLink(targetAnchor, targetLinkIndex);

    const int sourceNeighbor = adjacencyLink(sourceAnchor, sourceLinkIndex);

    setAdjacencyLink(targetAnchor, targetLinkIndex, sourceAnchor);

    setAdjacencyLink(sourceAnchor, sourceLinkIndex, targetAnchor);

    setAdjacencyLink(targetNeighbor, 1 - targetLinkIndex, sourceNeighbor);

    setAdjacencyLink(sourceNeighbor, 1 - sourceLinkIndex, targetNeighbor);

    target.externalFaceHalfEdges[targetLinkIndex] =
        source.externalFaceHalfEdges[1 - sourceLinkIndex];

    source.firstIncidentHalfEdge = -1;
    source.externalFaceHalfEdges = {-1, -1};
    source.externalFaceNeighbors = {-1, -1};
}

void BmPartialEmbedding::swapExternalFaceLinks(int internalVertexId) {
    validateInternalVertex(internalVertexId);

    std::swap(internalVertices_[internalVertexId].externalFaceHalfEdges[0],
              internalVertices_[internalVertexId].externalFaceHalfEdges[1]);

    std::swap(internalVertices_[internalVertexId].externalFaceNeighbors[0],
              internalVertices_[internalVertexId].externalFaceNeighbors[1]);
}

void BmPartialEmbedding::reverseAdjacencyOrientation(int internalVertexId) {
    validateInternalVertex(internalVertexId);

    BmInternalVertex& vertex = internalVertices_[internalVertexId];

    const int first = vertex.firstIncidentHalfEdge;

    if (first != -1) {
        int current = first;

        do {
            BmHalfEdge& edge = halfEdges_[current];

            const int oldNext = edge.nextAroundVertex;

            std::swap(edge.nextAroundVertex, edge.previousAroundVertex);

            current = oldNext;
        } while (current != first);
    }

    swapExternalFaceLinks(internalVertexId);
}

void BmPartialEmbedding::setEmbeddedEdgeSign(int embeddedEdgeId, int sign) {
    validateEmbeddedEdge(embeddedEdgeId);

    if (sign != -1 && sign != 1)
        throw std::invalid_argument("Embedded-edge sign must be +1 or -1.");

    embeddedEdges_[embeddedEdgeId].sign = sign;
}

int BmPartialEmbedding::createDetachedEmbeddedEdge(int fromInternalVertexId, int toInternalVertexId,
                                                   int originalEdgeId, bool shortCircuit) {
    validateInternalVertex(fromInternalVertexId);
    validateInternalVertex(toInternalVertexId);

    if (shortCircuit && originalEdgeId != -1)
        throw std::invalid_argument(
            "Short-circuit edge must not reference an original graph edge.");

    if (!shortCircuit && originalEdgeId < 0)
        throw std::invalid_argument("Non-helper edge must reference an original graph edge.");

    const int embeddedEdgeId = embeddedEdges_.size();
    const int halfEdgeAId = halfEdges_.size();
    const int halfEdgeBId = halfEdgeAId + 1;

    BmEmbeddedEdge embeddedEdge;
    embeddedEdge.id = embeddedEdgeId;
    embeddedEdge.originalEdgeId = originalEdgeId;
    embeddedEdge.halfEdgeA = halfEdgeAId;
    embeddedEdge.halfEdgeB = halfEdgeBId;
    embeddedEdge.shortCircuit = shortCircuit;
    embeddedEdge.sign = 1;
    embeddedEdge.active = true;

    BmHalfEdge halfEdgeA;
    halfEdgeA.id = halfEdgeAId;
    halfEdgeA.embeddedEdgeId = embeddedEdgeId;
    halfEdgeA.from = fromInternalVertexId;
    halfEdgeA.to = toInternalVertexId;
    halfEdgeA.twin = halfEdgeBId;

    BmHalfEdge halfEdgeB;
    halfEdgeB.id = halfEdgeBId;
    halfEdgeB.embeddedEdgeId = embeddedEdgeId;
    halfEdgeB.from = toInternalVertexId;
    halfEdgeB.to = fromInternalVertexId;
    halfEdgeB.twin = halfEdgeAId;

    embeddedEdges_.push_back(embeddedEdge);
    halfEdges_.push_back(halfEdgeA);
    halfEdges_.push_back(halfEdgeB);

    return embeddedEdgeId;
}

int BmPartialEmbedding::adjacencyLink(int halfEdgeId, int linkIndex) const {
    validateHalfEdge(halfEdgeId);

    if (linkIndex < 0 || linkIndex > 1)
        throw std::out_of_range("Adjacency link index must be 0 or 1.");

    if (linkIndex == 0)
        return halfEdges_[halfEdgeId].previousAroundVertex;

    return halfEdges_[halfEdgeId].nextAroundVertex;
}

void BmPartialEmbedding::setAdjacencyLink(int halfEdgeId, int linkIndex, int neighborHalfEdgeId) {
    validateHalfEdge(halfEdgeId);
    validateHalfEdge(neighborHalfEdgeId);

    if (linkIndex < 0 || linkIndex > 1)
        throw std::out_of_range("Adjacency link index must be 0 or 1.");

    if (linkIndex == 0) {
        halfEdges_[halfEdgeId].previousAroundVertex = neighborHalfEdgeId;
    } else {
        halfEdges_[halfEdgeId].nextAroundVertex = neighborHalfEdgeId;
    }
}

void BmPartialEmbedding::insertHalfEdgeAtExternalFaceSide(int internalVertexId, int halfEdgeId,
                                                          int linkIndex) {
    validateInternalVertex(internalVertexId);
    validateHalfEdge(halfEdgeId);

    if (linkIndex < 0 || linkIndex > 1)
        throw std::out_of_range("External-face link index must be 0 or 1.");

    BmInternalVertex& vertex = internalVertices_[internalVertexId];
    BmHalfEdge& inserted = halfEdges_[halfEdgeId];

    if (inserted.from != internalVertexId)
        throw std::logic_error("Half-edge source does not match adjacency-list vertex.");

    if (inserted.nextAroundVertex != -1 || inserted.previousAroundVertex != -1)
        throw std::logic_error("Half-edge is already linked into an adjacency list.");

    if (vertex.firstIncidentHalfEdge == -1) {
        inserted.nextAroundVertex = halfEdgeId;
        inserted.previousAroundVertex = halfEdgeId;

        vertex.firstIncidentHalfEdge = halfEdgeId;
        vertex.externalFaceHalfEdges = {halfEdgeId, halfEdgeId};

        return;
    }

    const int oldExternalHalfEdge = vertex.externalFaceHalfEdges[linkIndex];

    if (oldExternalHalfEdge == -1)
        throw std::logic_error("Existing adjacency list has no external-face anchor.");

    const int neighbor = adjacencyLink(oldExternalHalfEdge, linkIndex);

    setAdjacencyLink(oldExternalHalfEdge, linkIndex, halfEdgeId);

    setAdjacencyLink(halfEdgeId, 1 - linkIndex, oldExternalHalfEdge);

    setAdjacencyLink(halfEdgeId, linkIndex, neighbor);

    setAdjacencyLink(neighbor, 1 - linkIndex, halfEdgeId);

    vertex.externalFaceHalfEdges[linkIndex] = halfEdgeId;
}

int BmPartialEmbedding::addExternalFaceEdge(int firstInternalVertexId, int firstLinkIndex,
                                            int secondInternalVertexId, int secondLinkIndex,
                                            int originalEdgeId, bool shortCircuit) {
    const int embeddedEdgeId = createDetachedEmbeddedEdge(
        firstInternalVertexId, secondInternalVertexId, originalEdgeId, shortCircuit);

    const BmEmbeddedEdge& edge = embeddedEdges_[embeddedEdgeId];

    insertHalfEdgeAtExternalFaceSide(firstInternalVertexId, edge.halfEdgeA, firstLinkIndex);

    insertHalfEdgeAtExternalFaceSide(secondInternalVertexId, edge.halfEdgeB, secondLinkIndex);

    setExternalFaceNeighbor(firstInternalVertexId, firstLinkIndex, secondInternalVertexId);

    setExternalFaceNeighbor(secondInternalVertexId, secondLinkIndex, firstInternalVertexId);

    return embeddedEdgeId;
}

int BmPartialEmbedding::externalFaceNeighbor(int internalVertexId, int side) const {
    validateInternalVertex(internalVertexId);

    if (side < 0 || side > 1) {
        throw std::out_of_range("External face side must be 0 or 1.");
    }

    return internalVertices_[internalVertexId].externalFaceNeighbors[side];
}

int BmPartialEmbedding::externalFaceNeighborLinkIndex(int internalVertexId,
                                                      int neighborInternalVertexId) const {
    validateInternalVertex(internalVertexId);
    validateInternalVertex(neighborInternalVertexId);

    const auto& neighbors = internalVertices_[internalVertexId].externalFaceNeighbors;

    if (neighbors[0] == neighborInternalVertexId) {
        return 0;
    }

    if (neighbors[1] == neighborInternalVertexId) {
        return 1;
    }

    throw std::invalid_argument("Vertex is not an external-face neighbor.");
}

void BmPartialEmbedding::setExternalFaceNeighbors(int internalVertexId, int firstNeighborId,
                                                  int secondNeighborId) {
    validateInternalVertex(internalVertexId);

    if (firstNeighborId != -1) {
        validateInternalVertex(firstNeighborId);
    }

    if (secondNeighborId != -1) {
        validateInternalVertex(secondNeighborId);
    }

    internalVertices_[internalVertexId].externalFaceNeighbors = {firstNeighborId, secondNeighborId};
}

void BmPartialEmbedding::setExternalFaceNeighbor(int internalVertexId, int side,
                                                 int neighborInternalVertexId) {
    validateInternalVertex(internalVertexId);
    validateInternalVertex(neighborInternalVertexId);

    if (side < 0 || side > 1) {
        throw std::out_of_range("External face side must be 0 or 1.");
    }

    internalVertices_[internalVertexId].externalFaceNeighbors[side] = neighborInternalVertexId;
}

void BmPartialEmbedding::shortcutExternalFacePath(int rootInternalVertexId, int rootSide,
                                                  int stoppingInternalVertexId,
                                                  int stoppingIncomingLink) {
    validateInternalVertex(rootInternalVertexId);
    validateInternalVertex(stoppingInternalVertexId);

    if (rootSide < 0 || rootSide > 1) {
        throw std::out_of_range("Root side must be 0 or 1.");
    }

    if (stoppingIncomingLink < 0 || stoppingIncomingLink > 1) {
        throw std::out_of_range("Stopping incoming link must be 0 or 1.");
    }

    int targetVertex = stoppingInternalVertexId;

    int targetIncomingLink = stoppingIncomingLink;

    // Preserve at least three vertices on the external face.
    // If the opposite root side already points to the stopping
    // vertex, move the shortcut endpoint back by one vertex.
    if (externalFaceNeighbor(rootInternalVertexId, 1 - rootSide) == targetVertex) {
        const int oldTarget = targetVertex;

        targetVertex = externalFaceNeighbor(targetVertex, targetIncomingLink);

        targetIncomingLink = externalFaceNeighbor(targetVertex, 0) == oldTarget ? 1 : 0;
    }

    setExternalFaceNeighbor(rootInternalVertexId, rootSide, targetVertex);

    setExternalFaceNeighbor(targetVertex, targetIncomingLink, rootInternalVertexId);
}

} // namespace bm