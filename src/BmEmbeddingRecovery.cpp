#include "bm/BmEmbeddingRecovery.hpp"

#include "bm/PlanarEmbeddingValidator.hpp"

#include <stdexcept>
#include <vector>

namespace bm {

namespace {

struct OrientationFrame {
    int internalVertexId = -1;
    bool inverted = false;
};

bool isTreeEdgeFromParentToChild(
    const BmEmbeddingState& state,
    int fromInternalVertexId,
    const BmHalfEdge& halfEdge
) {
    const BmPartialEmbedding& embedding = state.partialEmbedding();

    if (embedding.isBicompRootVertex(halfEdge.to)) {
        return false;
    }

    const int parentVertex =
        embedding.originalVertexForInternalVertex(fromInternalVertexId);
    const int childVertex =
        embedding.originalVertexForInternalVertex(halfEdge.to);
    const DfsInfo& dfsInfo = state.dfsInfo();

    return dfsInfo.parent[static_cast<std::size_t>(childVertex)] == parentVertex
        && dfsInfo.parentEdgeId[static_cast<std::size_t>(childVertex)]
            == embedding.embeddedEdge(halfEdge.embeddedEdgeId).originalEdgeId;
}

void clearRecoveredTreeEdgeSign(
    BmEmbeddingState& state,
    int childVertex,
    int embeddedEdgeId
) {
    BmPartialEmbedding& embedding = state.partialEmbedding();
    embedding.setEmbeddedEdgeSign(embeddedEdgeId, 1);

    const int childRootId = state.rootForChild(childVertex);
    if (childRootId != -1) {
        state.bicompRoot(childRootId).rootEdgeSign = 1;
    }
}

} // namespace

void BmEmbeddingRecovery::orientForIsolation(BmEmbeddingState& state) {
    orientRemainingBicomps(state);
}

PlanarEmbedding BmEmbeddingRecovery::recover(BmEmbeddingState& state) {
    orientRemainingBicomps(state);
    joinRemainingBicomps(state);

    PlanarEmbedding embedding = buildPublicEmbedding(state);
    PlanarEmbeddingValidator::validate(state.graph(), embedding);

    return embedding;
}

void BmEmbeddingRecovery::orientRemainingBicomps(BmEmbeddingState& state) {
    std::vector<bool> visited(
        static_cast<std::size_t>(state.partialEmbedding().internalVertexCount()), false);

    for (int rootId = 0; rootId < state.bicompRootCount(); ++rootId) {
        if (state.bicompRoot(rootId).active) {
            orientBicomp(state, rootId, visited);
        }
    }
}

void BmEmbeddingRecovery::orientBicomp(
    BmEmbeddingState& state,
    int rootId,
    std::vector<bool>& visited
) {
    BmPartialEmbedding& embedding = state.partialEmbedding();
    const BmBicompRoot& root = state.bicompRoot(rootId);

    if (!root.active) {
        throw std::logic_error("Embedding recovery can orient only an active bicomp root.");
    }

    std::vector<OrientationFrame> stack;
    stack.push_back({root.internalRootVertexId, false});

    while (!stack.empty()) {
        const OrientationFrame frame = stack.back();
        stack.pop_back();

        if (visited[static_cast<std::size_t>(frame.internalVertexId)]) {
            continue;
        }
        visited[static_cast<std::size_t>(frame.internalVertexId)] = true;

        if (frame.inverted) {
            embedding.reverseAdjacencyOrientation(frame.internalVertexId);
        }

        const int first = embedding.firstIncidentHalfEdge(frame.internalVertexId);
        if (first == -1) {
            continue;
        }

        int current = first;
        int steps = 0;

        do {
            const BmHalfEdge& halfEdge = embedding.halfEdge(current);
            const int next = halfEdge.nextAroundVertex;

            if (isTreeEdgeFromParentToChild(state, frame.internalVertexId, halfEdge)) {
                const BmEmbeddedEdge& edge = embedding.embeddedEdge(halfEdge.embeddedEdgeId);
                const int childVertex =
                    embedding.originalVertexForInternalVertex(halfEdge.to);
                const bool childInverted = frame.inverted ^ (edge.sign == -1);

                clearRecoveredTreeEdgeSign(state, childVertex, halfEdge.embeddedEdgeId);
                stack.push_back({halfEdge.to, childInverted});
            }

            current = next;
            ++steps;

            if (steps > embedding.halfEdgeCount()) {
                throw std::logic_error(
                    "Recovery adjacency traversal did not return to its starting half-edge."
                );
            }
        } while (current != first);
    }
}

void BmEmbeddingRecovery::joinRemainingBicomps(BmEmbeddingState& state) {
    BmPartialEmbedding& embedding = state.partialEmbedding();

    for (int rootId = 0; rootId < state.bicompRootCount(); ++rootId) {
        BmBicompRoot& root = state.bicompRoot(rootId);

        if (!root.active) {
            continue;
        }

        const int targetInternalVertexId =
            embedding.originalInternalVertex(root.parentVertex);
        const int sourceInternalVertexId =
            root.internalRootVertexId;

        embedding.redirectAdjacencyToVertex(
            sourceInternalVertexId,
            targetInternalVertexId
        );

        // Matches the reference post-processing JoinBicomps call:
        // MergeVertex(parentCopy, 0, rootCopy).
        embedding.spliceAdjacencyLists(
            targetInternalVertexId,
            sourceInternalVertexId,
            0,
            1
        );

        state.deactivateBicompRoot(rootId);
    }
}

PlanarEmbedding BmEmbeddingRecovery::buildPublicEmbedding(
    const BmEmbeddingState& state
) {
    const Graph& graph = state.graph();
    const BmPartialEmbedding& internalEmbedding = state.partialEmbedding();

    PlanarEmbedding result;
    result.clockwiseEdgesAroundVertex.resize(
        static_cast<std::size_t>(graph.vertexCount())
    );

    for (int vertex = 0; vertex < graph.vertexCount(); ++vertex) {
        const int internalVertexId =
            internalEmbedding.originalInternalVertex(vertex);
        const int first =
            internalEmbedding.firstIncidentHalfEdge(internalVertexId);

        if (first == -1) {
            continue;
        }

        auto& rotation =
            result.clockwiseEdgesAroundVertex[static_cast<std::size_t>(vertex)];

        int current = first;
        int steps = 0;

        do {
            const BmHalfEdge& halfEdge =
                internalEmbedding.halfEdge(current);

            if (halfEdge.from != internalVertexId) {
                throw std::logic_error(
                    "Recovered half-edge is stored in the wrong original-vertex adjacency list."
                );
            }

            rotation.push_back(
                internalEmbedding.embeddedEdge(halfEdge.embeddedEdgeId).originalEdgeId
            );

            current = halfEdge.nextAroundVertex;
            ++steps;

            if (steps > internalEmbedding.halfEdgeCount()) {
                throw std::logic_error(
                    "Recovered adjacency traversal did not return to its starting half-edge."
                );
            }
        } while (current != first);
    }

    return result;
}

} // namespace bm
