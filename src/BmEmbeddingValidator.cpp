#include "bm/BmEmbeddingValidator.hpp"

#include "bm/BmExternalFaceTraversal.hpp"

#include <stdexcept>
#include <string>
#include <vector>

namespace bm {

namespace {

void require(bool condition, const std::string& message) {
    if (!condition) {
        throw std::logic_error(message);
    }
}

bool validIndex(int index, int size) {
    return index >= 0 && index < size;
}

} // namespace

void BmEmbeddingValidator::validatePartialEmbedding(const BmPartialEmbedding& embedding) {
    const int internalVertexCount = embedding.internalVertexCount();
    const int embeddedEdgeCount = embedding.embeddedEdgeCount();
    const int halfEdgeCount = embedding.halfEdgeCount();

    require(halfEdgeCount == 2 * embeddedEdgeCount,
            "Every embedded edge must own exactly two half-edges.");

    for (int edgeId = 0; edgeId < embeddedEdgeCount; ++edgeId) {
        const BmEmbeddedEdge& edge = embedding.embeddedEdge(edgeId);

        require(edge.id == edgeId, "Embedded-edge id does not match its storage index.");
        require(edge.originalEdgeId >= 0,
                "Decision-core embedded edge must reference an original graph edge.");
        require(edge.sign == -1 || edge.sign == 1, "Embedded-edge sign must be +1 or -1.");
        require(validIndex(edge.halfEdgeA, halfEdgeCount), "Invalid first half-edge id.");
        require(validIndex(edge.halfEdgeB, halfEdgeCount), "Invalid second half-edge id.");
        require(edge.halfEdgeA != edge.halfEdgeB, "Embedded edge cannot reuse one half-edge twice.");

        const BmHalfEdge& first = embedding.halfEdge(edge.halfEdgeA);
        const BmHalfEdge& second = embedding.halfEdge(edge.halfEdgeB);

        require(first.embeddedEdgeId == edgeId && second.embeddedEdgeId == edgeId,
                "Half-edge points to the wrong embedded edge.");
        require(first.twin == edge.halfEdgeB && second.twin == edge.halfEdgeA,
                "Embedded-edge twins are inconsistent.");
        require(first.from == second.to && first.to == second.from,
                "Twin half-edge endpoints are inconsistent.");
    }

    std::vector<int> adjacencyOwner(halfEdgeCount, -1);

    for (int vertexId = 0; vertexId < internalVertexCount; ++vertexId) {
        const BmInternalVertex& vertex = embedding.internalVertex(vertexId);

        require(vertex.id == vertexId, "Internal-vertex id does not match its storage index.");

        const int first = vertex.firstIncidentHalfEdge;

        if (first == -1) {
            require(vertex.externalFaceHalfEdges[0] == -1 && vertex.externalFaceHalfEdges[1] == -1,
                    "Vertex without adjacency list cannot keep external-face anchors.");
            require(vertex.externalFaceNeighbors[0] == -1 && vertex.externalFaceNeighbors[1] == -1,
                    "Vertex without adjacency list cannot keep external-face neighbors.");
            continue;
        }

        require(validIndex(first, halfEdgeCount), "Invalid first incident half-edge.");

        int current = first;
        int steps = 0;

        do {
            require(validIndex(current, halfEdgeCount), "Adjacency list contains invalid half-edge.");
            require(adjacencyOwner[current] == -1,
                    "Half-edge appears multiple times in circular adjacency lists.");

            adjacencyOwner[current] = vertexId;

            const BmHalfEdge& edge = embedding.halfEdge(current);

            require(edge.from == vertexId, "Half-edge is stored in the wrong adjacency list.");
            require(validIndex(edge.nextAroundVertex, halfEdgeCount),
                    "Adjacency list contains invalid next link.");
            require(validIndex(edge.previousAroundVertex, halfEdgeCount),
                    "Adjacency list contains invalid previous link.");
            require(embedding.halfEdge(edge.nextAroundVertex).previousAroundVertex == current,
                    "Adjacency next/previous links are inconsistent.");
            require(embedding.halfEdge(edge.previousAroundVertex).nextAroundVertex == current,
                    "Adjacency previous/next links are inconsistent.");

            current = edge.nextAroundVertex;
            ++steps;

            require(steps <= halfEdgeCount,
                    "Circular adjacency traversal did not return to its starting half-edge.");
        } while (current != first);

        for (int side = 0; side <= 1; ++side) {
            const int anchor = vertex.externalFaceHalfEdges[side];

            require(validIndex(anchor, halfEdgeCount), "External-face anchor is invalid.");
            require(adjacencyOwner[anchor] == vertexId,
                    "External-face anchor is not incident to its vertex.");
        }
    }

    for (int halfEdgeId = 0; halfEdgeId < halfEdgeCount; ++halfEdgeId) {
        const BmHalfEdge& halfEdge = embedding.halfEdge(halfEdgeId);

        require(halfEdge.id == halfEdgeId, "Half-edge id does not match its storage index.");
        require(validIndex(halfEdge.embeddedEdgeId, embeddedEdgeCount),
                "Half-edge references invalid embedded edge.");
        require(validIndex(halfEdge.twin, halfEdgeCount), "Half-edge twin id is invalid.");
        require(embedding.halfEdge(halfEdge.twin).twin == halfEdgeId,
                "Half-edge twin relation is not symmetric.");
        require(validIndex(halfEdge.from, internalVertexCount), "Half-edge source vertex is invalid.");
        require(validIndex(halfEdge.to, internalVertexCount), "Half-edge target vertex is invalid.");
        require(adjacencyOwner[halfEdgeId] == halfEdge.from,
                "Half-edge is missing from the source adjacency list.");
    }

    for (int vertexId = 0; vertexId < internalVertexCount; ++vertexId) {
        const BmInternalVertex& vertex = embedding.internalVertex(vertexId);

        for (int side = 0; side <= 1; ++side) {
            const int neighbor = vertex.externalFaceNeighbors[side];

            if (neighbor != -1) {
                require(validIndex(neighbor, internalVertexCount),
                        "External-face neighbor is invalid.");
            }
        }
    }
}

void BmEmbeddingValidator::validateState(const BmEmbeddingState& state) {
    const Graph& graph = state.graph();
    const DfsInfo& dfsInfo = state.dfsInfo();
    const BmPartialEmbedding& embedding = state.partialEmbedding();

    validatePartialEmbedding(embedding);

    for (int rootId = 0; rootId < state.bicompRootCount(); ++rootId) {
        const BmBicompRoot& root = state.bicompRoot(rootId);

        require(root.id == rootId, "Bicomp-root id does not match its storage index.");
        require(root.parentVertex >= 0 && root.parentVertex < graph.vertexCount(),
                "Bicomp root has invalid parent vertex.");
        require(root.childVertex >= 0 && root.childVertex < graph.vertexCount(),
                "Bicomp root has invalid child vertex.");
        require(dfsInfo.parent[root.childVertex] == root.parentVertex,
                "Bicomp root does not match the DFS parent-child relation.");
        require(root.treeEdgeId == dfsInfo.parentEdgeId[root.childVertex],
                "Bicomp root references the wrong DFS tree edge.");
        require(root.rootEdgeSign == -1 || root.rootEdgeSign == 1,
                "Bicomp-root edge sign must be +1 or -1.");

        const BmInternalVertex& internalRoot = embedding.internalVertex(root.internalRootVertexId);

        require(internalRoot.kind == BmInternalVertexKind::BicompRoot,
                "Bicomp root does not reference a virtual internal vertex.");
        require(internalRoot.bicompRootId == rootId,
                "Virtual internal vertex references the wrong bicomp root.");
        require(root.internalChildVertexId == embedding.originalInternalVertex(root.childVertex),
                "Bicomp root references the wrong internal child vertex.");
        require(embedding.embeddedEdge(root.embeddedTreeEdgeId).originalEdgeId == root.treeEdgeId,
                "Bicomp root references the wrong embedded tree edge.");
        require(embedding.embeddedEdge(root.embeddedTreeEdgeId).sign == root.rootEdgeSign,
                "Bicomp-root sign and tree-edge sign differ.");

        if (root.active) {
            require(!embedding.adjacencyEmpty(root.internalRootVertexId),
                    "Active bicomp root cannot have an empty adjacency list.");

            BmExternalFaceTraversal traversal(embedding);

            for (int incomingLink = 0; incomingLink <= 1; ++incomingLink) {
                BmExternalFacePosition start;
                start.internalVertexId = root.internalRootVertexId;
                start.linkIndex = incomingLink;

                BmExternalFacePosition current = traversal.successor(start);
                int steps = 1;

                while (current.internalVertexId != start.internalVertexId) {
                    current = traversal.successor(current);
                    ++steps;

                    require(steps <= embedding.internalVertexCount() + 1,
                            "Active external-face traversal did not return to its bicomp root.");
                }
            }
        } else {
            require(embedding.adjacencyEmpty(root.internalRootVertexId),
                    "Merged bicomp root must have an empty adjacency list.");
        }
    }

    for (int vertex = 0; vertex < graph.vertexCount(); ++vertex) {
        if (dfsInfo.parent[vertex] == -1) {
            require(state.rootForChild(vertex) == -1,
                    "DFS root cannot have a parent bicomp-root mapping.");
        } else {
            const int rootId = state.rootForChild(vertex);

            require(rootId >= 0 && rootId < state.bicompRootCount(),
                    "DFS child is missing its bicomp-root mapping.");
            require(state.bicompRoot(rootId).childVertex == vertex,
                    "DFS-child mapping points to the wrong bicomp root.");
        }

        for (int rootId : state.vertexState(vertex).pertinentRoots) {
            require(rootId >= 0 && rootId < state.bicompRootCount(),
                    "Pertinent-root list contains invalid root id.");

            const BmBicompRoot& root = state.bicompRoot(rootId);

            require(root.active, "Pertinent-root list contains an already merged root.");
            require(root.parentVertex == vertex,
                    "Pertinent-root list contains a root of a different cut vertex.");
        }
    }

    for (int originalEdgeId = 0; originalEdgeId < graph.edgeCount(); ++originalEdgeId) {
        if (!state.isOriginalEdgeEmbedded(originalEdgeId)) {
            continue;
        }

        const int embeddedEdgeId = state.embeddedEdgeIdForOriginalEdge(originalEdgeId);

        require(embeddedEdgeId >= 0 && embeddedEdgeId < embedding.embeddedEdgeCount(),
                "Original-edge mapping contains invalid embedded-edge id.");
        require(embedding.embeddedEdge(embeddedEdgeId).originalEdgeId == originalEdgeId,
                "Original-edge mapping points to the wrong embedded edge.");
    }

    for (int embeddedEdgeId = 0; embeddedEdgeId < embedding.embeddedEdgeCount(); ++embeddedEdgeId) {
        const int originalEdgeId = embedding.embeddedEdge(embeddedEdgeId).originalEdgeId;

        require(originalEdgeId >= 0 && originalEdgeId < graph.edgeCount(),
                "Embedded edge references invalid original edge.");
        require(state.isOriginalEdgeEmbedded(originalEdgeId),
                "Embedded edge is missing from original-edge mapping.");
        require(state.embeddedEdgeIdForOriginalEdge(originalEdgeId) == embeddedEdgeId,
                "Original-edge mapping is not one-to-one.");
    }
}

} // namespace bm
