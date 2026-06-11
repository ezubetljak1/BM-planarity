#include "bm/BmKuratowskiInternalPathAnalyzer.hpp"

#include "bm/BmRealExternalFaceTraversal.hpp"

#include <algorithm>
#include <stdexcept>
#include <vector>

namespace bm {

namespace {

using Mark = BmKuratowskiObstructionMark;

int originalVertex(
    const BmEmbeddingState& state,
    int internalVertexId
) {
    if (state.isInternalBicompRootVertex(internalVertexId)) {
        throw std::logic_error("Expected an original internal vertex.");
    }

    return state.originalVertexForInternalVertex(internalVertexId);
}

bool isRxw(Mark mark) {
    return mark == Mark::HighRxw || mark == Mark::LowRxw;
}

bool isRyw(Mark mark) {
    return mark == Mark::HighRyw || mark == Mark::LowRyw;
}

void validateContextRoot(
    const BmEmbeddingState& state,
    const BmKuratowskiExtractionContext& context
) {
    if (context.centralRootInternalVertexId < 0
        || context.centralRootInternalVertexId >= state.partialEmbedding().internalVertexCount()) {
        throw std::invalid_argument("Kuratowski context has no valid central root.");
    }

    if (!state.isInternalBicompRootVertex(context.centralRootInternalVertexId)) {
        throw std::invalid_argument("Kuratowski context central root is not virtual.");
    }

    if (context.pertinentVertex < 0) {
        throw std::invalid_argument("Kuratowski context has no pertinent vertex W.");
    }
}

BmRealExternalFacePosition findPositionOnSide(
    const BmEmbeddingState& state,
    int rootInternalVertexId,
    int rootIncomingLink,
    int targetOriginalVertex
) {
    BmRealExternalFaceTraversal traversal(state.partialEmbedding());
    BmRealExternalFacePosition position = traversal.successor(
        {rootInternalVertexId, rootIncomingLink}
    );

    const int maxSteps = state.partialEmbedding().halfEdgeCount() + 1;

    for (int step = 0; step < maxSteps; ++step) {
        if (position.internalVertexId == rootInternalVertexId) {
            break;
        }

        if (!state.isInternalBicompRootVertex(position.internalVertexId)
            && originalVertex(state, position.internalVertexId) == targetOriginalVertex) {
            return position;
        }

        position = traversal.successor(position);
    }

    throw std::logic_error("External-face side did not reach the requested vertex.");
}

int edgeOriginalId(
    const BmPartialEmbedding& embedding,
    int halfEdgeId
) {
    return embedding.embeddedEdge(
        embedding.halfEdge(halfEdgeId).embeddedEdgeId
    ).originalEdgeId;
}

int previousAroundWithRootInternalEdgesHidden(
    const BmPartialEmbedding& embedding,
    int currentVertex,
    int entryHalfEdge,
    int rootInternalVertex
) {
    const int firstAnchor = embedding.externalFaceHalfEdge(rootInternalVertex, 0);
    const int secondAnchor = embedding.externalFaceHalfEdge(rootInternalVertex, 1);

    if (currentVertex == rootInternalVertex) {
        if (entryHalfEdge == secondAnchor) {
            return firstAnchor;
        }

        if (entryHalfEdge == firstAnchor) {
            return secondAnchor;
        }

        throw std::logic_error(
            "Highest X-Y traversal entered R through a hidden internal edge."
        );
    }

    int candidate = embedding.previousAroundVertex(entryHalfEdge);
    int steps = 0;

    while (embedding.halfEdge(candidate).to == rootInternalVertex) {
        const int twin = embedding.halfEdge(candidate).twin;

        if (twin == firstAnchor || twin == secondAnchor) {
            break;
        }

        candidate = embedding.previousAroundVertex(candidate);

        if (++steps > embedding.halfEdgeCount() + 1) {
            throw std::logic_error(
                "Could not skip hidden root-incident edges during X-Y traversal."
            );
        }
    }

    return candidate;
}

} // namespace

void BmKuratowskiInternalPathAnalyzer::classifyExternalFaceVertices(
    const BmEmbeddingState& state,
    BmKuratowskiExtractionContext& context
) {
    validateContextRoot(state, context);

    const BmPartialEmbedding& embedding = state.partialEmbedding();
    BmRealExternalFaceTraversal traversal(embedding);

    context.obstructionMarksByInternalVertex.assign(
        static_cast<std::size_t>(embedding.internalVertexCount()),
        Mark::Unmarked
    );

    // Traverse R -> ... -> X -> ... -> W.
    BmRealExternalFacePosition position = traversal.successor(
        {context.centralRootInternalVertexId, 1}
    );
    Mark mark = Mark::HighRxw;
    int steps = 0;

    while (originalVertex(state, position.internalVertexId) != context.pertinentVertex) {
        const int vertex = originalVertex(state, position.internalVertexId);

        if (vertex == originalVertex(state, context.x.internalVertexId)) {
            mark = Mark::LowRxw;
        }

        context.obstructionMarksByInternalVertex[
            static_cast<std::size_t>(position.internalVertexId)
        ] = mark;

        position = traversal.successor(position);

        if (++steps > embedding.halfEdgeCount() + 1) {
            throw std::logic_error("R-X-W external-face classification did not reach W.");
        }
    }

    // Traverse R -> ... -> Y -> ... -> W on the opposite side.
    position = traversal.successor({context.centralRootInternalVertexId, 0});
    mark = Mark::HighRyw;
    steps = 0;

    while (originalVertex(state, position.internalVertexId) != context.pertinentVertex) {
        const int vertex = originalVertex(state, position.internalVertexId);

        if (vertex == originalVertex(state, context.y.internalVertexId)) {
            mark = Mark::LowRyw;
        }

        context.obstructionMarksByInternalVertex[
            static_cast<std::size_t>(position.internalVertexId)
        ] = mark;

        position = traversal.successor(position);

        if (++steps > embedding.halfEdgeCount() + 1) {
            throw std::logic_error("R-Y-W external-face classification did not reach W.");
        }
    }
}

void BmKuratowskiInternalPathAnalyzer::findHighestXyPath(
    const BmEmbeddingState& state,
    BmKuratowskiExtractionContext& context
) {
    validateContextRoot(state, context);

    if (context.obstructionMarksByInternalVertex.empty()) {
        classifyExternalFaceVertices(state, context);
    }

    const BmPartialEmbedding& embedding = state.partialEmbedding();
    const int root = context.centralRootInternalVertexId;
    const int wInternal = embedding.originalInternalVertex(context.pertinentVertex);

    context.px = -1;
    context.py = -1;
    context.pxPosition = {};
    context.pyPosition = {};
    context.xyPathHalfEdgeIds.clear();
    context.xyPathOriginalEdgeIds.clear();

    std::vector<int> stackVertices;
    std::vector<int> stackEntryHalfEdges;
    std::vector<int> stackIndexByInternalVertex(
        static_cast<std::size_t>(embedding.internalVertexCount()),
        -1
    );

    int currentVertex = root;
    int entryHalfEdge = embedding.externalFaceHalfEdge(root, 1);
    int steps = 0;

    while (true) {
        int outgoingHalfEdge = -1;

        // Reference _MarkClosestXYPath temporarily hides internal edges at R.
        // Simulate that filtered rotation without mutating the embedding.
        outgoingHalfEdge = previousAroundWithRootInternalEdgesHidden(
            embedding,
            currentVertex,
            entryHalfEdge,
            root
        );

        const BmHalfEdge& outgoing = embedding.halfEdge(outgoingHalfEdge);
        const int nextVertex = outgoing.to;
        const int nextEntryHalfEdge = outgoing.twin;

        if (++steps > embedding.halfEdgeCount() * 2 + 2) {
            throw std::logic_error("Highest X-Y proper-face traversal exceeded its guard.");
        }

        if (nextVertex == wInternal) {
            // The proper face reached the antipodal vertex W: no X-Y path.
            context.px = -1;
            context.py = -1;
            context.xyPathHalfEdgeIds.clear();
            context.xyPathOriginalEdgeIds.clear();
            return;
        }

        const int existingIndex = stackIndexByInternalVertex[
            static_cast<std::size_t>(nextVertex)
        ];

        if (existingIndex != -1) {
            while (static_cast<int>(stackVertices.size()) - 1 > existingIndex) {
                stackIndexByInternalVertex[
                    static_cast<std::size_t>(stackVertices.back())
                ] = -1;
                stackVertices.pop_back();
                stackEntryHalfEdges.pop_back();
            }
        } else {
            const Mark vertexMark = context.obstructionMarksByInternalVertex[
                static_cast<std::size_t>(nextVertex)
            ];

            if (isRxw(vertexMark)) {
                context.px = originalVertex(state, nextVertex);
                context.pxPosition = findPositionOnSide(
                    state,
                    root,
                    1,
                    context.px
                );

                for (int vertex : stackVertices) {
                    stackIndexByInternalVertex[static_cast<std::size_t>(vertex)] = -1;
                }
                stackVertices.clear();
                stackEntryHalfEdges.clear();
            }

            stackIndexByInternalVertex[static_cast<std::size_t>(nextVertex)] =
                static_cast<int>(stackVertices.size());
            stackVertices.push_back(nextVertex);
            stackEntryHalfEdges.push_back(nextEntryHalfEdge);

            if (isRyw(vertexMark)) {
                context.py = originalVertex(state, nextVertex);
                context.pyPosition = findPositionOnSide(
                    state,
                    root,
                    0,
                    context.py
                );
                break;
            }
        }

        currentVertex = nextVertex;
        entryHalfEdge = nextEntryHalfEdge;
    }

    if (context.px == -1 || context.py == -1) {
        throw std::logic_error("Highest X-Y path did not obtain both attachment points.");
    }

    // stackEntryHalfEdges stores the half-edge used to enter each path vertex.
    // The entry edge of P_x is external-face material and is intentionally
    // omitted. Every subsequent entry edge belongs to the internal X-Y path.
    bool skippedPx = false;

    for (std::size_t index = 0; index < stackVertices.size(); ++index) {
        const int vertex = stackVertices[index];

        if (!skippedPx && originalVertex(state, vertex) == context.px) {
            skippedPx = true;
            continue;
        }

        const int directedHalfEdge = stackEntryHalfEdges[index];
        context.xyPathHalfEdgeIds.push_back(directedHalfEdge);
        context.xyPathOriginalEdgeIds.push_back(edgeOriginalId(embedding, directedHalfEdge));
    }

    if (context.xyPathHalfEdgeIds.empty()) {
        throw std::logic_error("Highest X-Y path contains no internal edge.");
    }
}

void BmKuratowskiInternalPathAnalyzer::findZToRootPath(
    const BmEmbeddingState& state,
    BmKuratowskiExtractionContext& context
) {
    validateContextRoot(state, context);

    context.z = -1;
    context.zToRootOriginalEdgeIds.clear();

    if (context.xyPathHalfEdgeIds.size() < 2) {
        return;
    }

    const BmPartialEmbedding& embedding = state.partialEmbedding();
    std::vector<bool> xyEdge(
        static_cast<std::size_t>(embedding.embeddedEdgeCount()),
        false
    );

    for (int halfEdgeId : context.xyPathHalfEdgeIds) {
        const int edgeId = embedding.halfEdge(halfEdgeId).embeddedEdgeId;
        xyEdge[static_cast<std::size_t>(edgeId)] = true;
    }

    // The stored half-edges are directed entries into successive vertices on
    // P_x -> P_y. For every internal X-Y vertex, inspect the predecessor edge
    // above the path. If it is not the next marked X-Y edge, it starts Z -> R.
    for (std::size_t index = 0; index + 1 < context.xyPathHalfEdgeIds.size(); ++index) {
        const int entryHalfEdge = context.xyPathHalfEdgeIds[index];
        const int vertex = embedding.halfEdge(entryHalfEdge).from;
        const int candidate = embedding.previousAroundVertex(entryHalfEdge);
        const int candidateEdge = embedding.halfEdge(candidate).embeddedEdgeId;

        if (xyEdge[static_cast<std::size_t>(candidateEdge)]) {
            continue;
        }

        context.z = originalVertex(state, vertex);

        int outgoingHalfEdge = candidate;
        int steps = 0;

        while (embedding.halfEdge(outgoingHalfEdge).from
               != context.centralRootInternalVertexId) {
            context.zToRootOriginalEdgeIds.push_back(
                edgeOriginalId(embedding, outgoingHalfEdge)
            );

            const int nextEntry = embedding.halfEdge(outgoingHalfEdge).twin;
            const int nextVertex = embedding.halfEdge(nextEntry).from;

            if (nextVertex == context.centralRootInternalVertexId) {
                break;
            }

            outgoingHalfEdge = embedding.previousAroundVertex(nextEntry);

            if (++steps > embedding.halfEdgeCount() + 1) {
                throw std::logic_error("Z-to-R proper-face traversal exceeded its guard.");
            }
        }

        return;
    }
}

int BmKuratowskiInternalPathAnalyzer::findFuturePertinentBelowXyPath(
    const BmEmbeddingState& state,
    const BmKuratowskiExtractionContext& context
) {
    validateContextRoot(state, context);

    if (context.px < 0 || context.py < 0) {
        throw std::invalid_argument("Future-pertinence search requires P_x and P_y.");
    }

    const BmPartialEmbedding& embedding = state.partialEmbedding();
    BmRealExternalFaceTraversal traversal(embedding);

    BmRealExternalFacePosition position = traversal.successor(context.pxPosition);
    const int pyInternal = embedding.originalInternalVertex(context.py);
    int steps = 0;

    while (position.internalVertexId != pyInternal) {
        if (state.isInternalBicompRootVertex(position.internalVertexId)) {
            throw std::logic_error("Future-pertinence search encountered a virtual root.");
        }

        const int vertex = originalVertex(state, position.internalVertexId);

        if (state.isExternallyActive(vertex, context.currentVertex)) {
            return vertex;
        }

        position = traversal.successor(position);

        if (++steps > embedding.halfEdgeCount() + 1) {
            throw std::logic_error("Future-pertinence search did not reach P_y.");
        }
    }

    return -1;
}

} // namespace bm
