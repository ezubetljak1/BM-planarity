#include "bm/BmKuratowskiExtractor.hpp"

#include "bm/BmKuratowskiConnectionFinder.hpp"
#include "bm/BmKuratowskiPathMarker.hpp"
#include "bm/KuratowskiCertificateVerifier.hpp"

#include <algorithm>
#include <array>
#include <stdexcept>

namespace bm {

namespace {

BmOriginalBackEdgeConnection requireConnection(
    const std::optional<BmOriginalBackEdgeConnection>& connection,
    const char* message
) {
    if (!connection.has_value()) {
        throw std::logic_error(message);
    }

    return *connection;
}

int originalVertex(
    const BmEmbeddingState& state,
    BmRealExternalFacePosition position
) {
    return state.originalVertexForInternalVertex(position.internalVertexId);
}

int vertexWithLowestDfi(
    const DfsInfo& dfsInfo,
    std::initializer_list<int> vertices
) {
    return *std::min_element(
        vertices.begin(),
        vertices.end(),
        [&](int first, int second) {
            return dfsInfo.dfsIndex[static_cast<std::size_t>(first)]
                < dfsInfo.dfsIndex[static_cast<std::size_t>(second)];
        }
    );
}

int vertexWithHighestDfi(
    const DfsInfo& dfsInfo,
    std::initializer_list<int> vertices
) {
    return *std::max_element(
        vertices.begin(),
        vertices.end(),
        [&](int first, int second) {
            return dfsInfo.dfsIndex[static_cast<std::size_t>(first)]
                < dfsInfo.dfsIndex[static_cast<std::size_t>(second)];
        }
    );
}

void markWholeCentralExternalFace(
    const BmEmbeddingState& state,
    const BmKuratowskiExtractionContext& context,
    BmKuratowskiPathMarker& marker
) {
    marker.markRealExternalFacePath(
        {context.centralRootInternalVertexId, 1},
        context.centralRootInternalVertexId
    );
}

void markConnection(
    BmKuratowskiPathMarker& marker,
    const BmOriginalBackEdgeConnection& connection
) {
    marker.markOriginalEdge(connection.edgeId);
}

} // namespace

KuratowskiCertificate BmKuratowskiExtractor::extractInitialMinor(
    const BmEmbeddingState& state,
    const BmWalkdownFailure& failure
) {
    const BmKuratowskiExtractionContext context =
        BmKuratowskiExtractionContextBuilder::initialize(state, failure);

    switch (BmKuratowskiMinorClassifier::classifyInitial(state, context)) {
    case BmKuratowskiMinorType::A:
        return isolateMinorA(state, context);
    case BmKuratowskiMinorType::B:
        return isolateMinorB(state, context);
    default:
        throw std::logic_error(
            "Kuratowski extraction requires the C-D-E internal X-Y path stage."
        );
    }
}

KuratowskiCertificate BmKuratowskiExtractor::isolateMinorA(
    const BmEmbeddingState& state,
    const BmKuratowskiExtractionContext& context
) {
    const int x = originalVertex(state, context.x);
    const int y = originalVertex(state, context.y);
    const int w = context.pertinentVertex;
    const int rootParent = state.bicompRoot(context.centralRootId).parentVertex;

    const BmOriginalBackEdgeConnection uxDx = requireConnection(
        BmKuratowskiConnectionFinder::findToAncestor(state, context.currentVertex, x),
        "Minor A requires an X-to-ancestor connection."
    );
    const BmOriginalBackEdgeConnection uyDy = requireConnection(
        BmKuratowskiConnectionFinder::findToAncestor(state, context.currentVertex, y),
        "Minor A requires a Y-to-ancestor connection."
    );
    const BmOriginalBackEdgeConnection vDw = requireConnection(
        BmKuratowskiConnectionFinder::findToCurrentVertex(state, context.currentVertex, w),
        "Minor A requires a current-vertex-to-W-subtree connection."
    );

    BmKuratowskiPathMarker marker(state);
    markWholeCentralExternalFace(state, context, marker);

    const int highestAncestor = vertexWithLowestDfi(
        state.dfsInfo(),
        {uxDx.ancestorVertex, uyDy.ancestorVertex}
    );

    marker.markDfsPath(highestAncestor, rootParent);
    marker.markDfsPath(x, uxDx.descendantVertex);
    marker.markDfsPath(y, uyDy.descendantVertex);
    marker.markDfsPath(w, vDw.descendantVertex);

    markConnection(marker, uxDx);
    markConnection(marker, uyDy);
    markConnection(marker, vDw);

    return KuratowskiCertificateVerifier::analyze(
        state.graph(),
        marker.markedOriginalEdgeIds()
    );
}

KuratowskiCertificate BmKuratowskiExtractor::isolateMinorB(
    const BmEmbeddingState& state,
    const BmKuratowskiExtractionContext& context
) {
    const int x = originalVertex(state, context.x);
    const int y = originalVertex(state, context.y);
    const int w = context.pertinentVertex;

    const auto& roots = state.vertexState(w).pertinentRoots;
    if (roots.empty()) {
        throw std::logic_error("Minor B requires a pertinent child bicomp at W.");
    }

    const int subtreeRoot = state.bicompRoot(roots.back()).childVertex;
    const int uz = state.dfsInfo().lowpointVertex[static_cast<std::size_t>(subtreeRoot)];

    const BmOriginalBackEdgeConnection uxDx = requireConnection(
        BmKuratowskiConnectionFinder::findToAncestor(state, context.currentVertex, x),
        "Minor B requires an X-to-ancestor connection."
    );
    const BmOriginalBackEdgeConnection uyDy = requireConnection(
        BmKuratowskiConnectionFinder::findToAncestor(state, context.currentVertex, y),
        "Minor B requires a Y-to-ancestor connection."
    );
    const BmOriginalBackEdgeConnection vDw = requireConnection(
        BmKuratowskiConnectionFinder::findToSubtree(state, context.currentVertex, subtreeRoot),
        "Minor B requires a current-vertex-to-W-subtree connection."
    );
    const BmOriginalBackEdgeConnection uzDz = requireConnection(
        BmKuratowskiConnectionFinder::findToSubtree(state, uz, subtreeRoot),
        "Minor B requires a lowpoint-ancestor-to-W-subtree connection."
    );

    BmKuratowskiPathMarker marker(state);
    markWholeCentralExternalFace(state, context, marker);

    const int highestAncestor = vertexWithLowestDfi(
        state.dfsInfo(),
        {uxDx.ancestorVertex, uyDy.ancestorVertex, uzDz.ancestorVertex}
    );
    const int lowestAncestor = vertexWithHighestDfi(
        state.dfsInfo(),
        {uxDx.ancestorVertex, uyDy.ancestorVertex, uzDz.ancestorVertex}
    );

    marker.markDfsPath(highestAncestor, lowestAncestor);
    marker.markDfsPath(x, uxDx.descendantVertex);
    marker.markDfsPath(y, uyDy.descendantVertex);
    marker.markDfsPath(w, vDw.descendantVertex);
    marker.markDfsPath(w, uzDz.descendantVertex);

    markConnection(marker, uxDx);
    markConnection(marker, uyDy);
    markConnection(marker, vDw);
    markConnection(marker, uzDz);

    return KuratowskiCertificateVerifier::analyze(
        state.graph(),
        marker.markedOriginalEdgeIds()
    );
}

} // namespace bm
