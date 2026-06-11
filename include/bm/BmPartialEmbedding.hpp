#pragma once

#include <array>
#include <vector>

namespace bm {

enum class BmInternalVertexKind { Original, BicompRoot };

struct BmInternalVertex {
    int id = -1;
    BmInternalVertexKind kind = BmInternalVertexKind::Original;

    // if kind == Original, this is the graph vertex
    // if kind == BicompRoot, this is the non-virtual root vertex r from r^c
    int originalVertex = -1;

    // only valid for BicompRoot
    int bicompRootId = -1;

    // the two external-face links incident to this internal vertex
    // for a single tree-edge bicomp both sides may initially refer to the same half-edge
    std::array<int, 2> externalFaceHalfEdges = {-1, -1};

    // Representative node of the circular adjacency list
    int firstIncidentHalfEdge = -1;

    // external-face traversal links
    // these are separate from adjacency anchors,
    // allowing constant-time root-to-stopping-vertex shortcuts without fake edges
    std::array<int, 2> externalFaceNeighbors = {-1, -1};
};

struct BmEmbeddedEdge {
    int id = -1;

    // original graph edge id; short-circuit/helper edges can later use -1
    int originalEdgeId = -1;

    int halfEdgeA = -1;
    int halfEdgeB = -1;

    // Used for lazy flip recovery.
    int sign = 1;
};

struct BmHalfEdge {
    int id = -1;
    int embeddedEdgeId = -1;

    int from = -1;
    int to = -1;

    int twin = -1;

    // circular adjacency-list links around from
    int nextAroundVertex = -1;
    int previousAroundVertex = -1;
};

struct BmTreeBicompEmbedding {
    int rootInternalVertexId = -1;
    int childInternalVertexId = -1;

    int embeddedEdgeId = -1;

    int rootToChildHalfEdgeId = -1;
    int childToRootHalfEdgeId = -1;
};

class BmPartialEmbedding {
public:
    explicit BmPartialEmbedding(int originalVertexCount);

    int internalVertexCount() const;
    int embeddedEdgeCount() const;
    int halfEdgeCount() const;

    int originalInternalVertex(int originalVertex) const;

    const BmInternalVertex& internalVertex(int internalVertexId) const;
    BmInternalVertex& internalVertex(int internalVertexId);

    const BmEmbeddedEdge& embeddedEdge(int embeddedEdgeId) const;
    BmEmbeddedEdge& embeddedEdge(int embeddedEdgeId);

    const BmHalfEdge& halfEdge(int halfEdge) const;
    BmHalfEdge& halfEdge(int halfEdge);

    BmTreeBicompEmbedding createTreeEdgeBicomp(int bicompRootId, int parentVertex, int childVertex,
                                               int originalTreeEdgeId);

    int twinHalfEdge(int halfEdgeId) const;

    int externalFaceHalfEdge(int internalVertexId, int side) const;

    bool isBicompRootVertex(int internalVertexId) const;
    int bicompRootIdForInternalVertex(int internalVertexId) const;
    int originalVertexForInternalVertex(int internalVertexId) const;

    void setExternalFaceHalfEdges(int internalVertexId, int firstHalfEdgeId, int secondHalfEdgeId);


    int addEmbeddedEdge(int fromInternalVertexId, int toInternalVertexId, int originalEdgeId);
    void insertHalfEdgeIntoAdjacency(int internalVertexId, int halfEdgeId);
    void redirectHalfEdgeEndpoint(int halfEdgeId, int newInternalVertexId);
    void redirectAdjacencyToVertex(int sourceInternalVertexId, int targetInternalVertexId);
    void spliceAdjacencyLists(int targetInternalVertexId, int sourceInternalVertexId,
                              int targetLinkIndex, int sourceLinkIndex);
    void swapExternalFaceLinks(int internalVertexId);
    void reverseAdjacencyOrientation(int internalVertexId);
    void setEmbeddedEdgeSign(int embeddedEdgeId, int sign);

    bool adjacencyEmpty(int internalVertexId) const;

    int firstIncidentHalfEdge(int internalVertexId) const;
    int nextAroundVertex(int halfEdgeId) const;
    int previousAroundVertex(int halfEdgeId) const;

    int addExternalFaceEdge(int firstInternalVertexId, int firstLinkIndex,
                            int secondInternalVertexId, int secondLinkIndex, int originalEdgeId);

    int externalFaceNeighbor(int internalVertexId, int side) const;


    void setExternalFaceNeighbors(int internalVertexId, int firstNeighborId, int secondNeighborId);

    void setExternalFaceNeighbor(int internalVertexId, int side, int neighborInternalVertexId);

    void shortcutExternalFacePath(int rootInternalVertexId, int rootSide,
                                  int stoppingInternalVertexId, int stoppingIncomingLink);

private:
    std::vector<BmInternalVertex> internalVertices_;
    std::vector<BmEmbeddedEdge> embeddedEdges_;
    std::vector<BmHalfEdge> halfEdges_;
    std::vector<int> originalToInternalVertex_;

    int createBicompRootVertex(int bicompRootId, int parentVertex);

    void validateOriginalVertex(int vertex) const;
    void validateInternalVertex(int internalVertexId) const;
    void validateEmbeddedEdge(int embeddedEdgeId) const;
    void validateHalfEdge(int halfEdgeId) const;

    int createDetachedEmbeddedEdge(int fromInternalVertexId, int toInternalVertexId,
                                   int originalEdgeId);

    void insertHalfEdgeAtExternalFaceSide(int internalVertexId, int halfEdgeId, int linkIndex);

    int adjacencyLink(int halfEdgeId, int linkIndex) const;

    void setAdjacencyLink(int halfEdgeId, int linkIndex, int neighborHalfEdgeId);
};

} // namespace bm