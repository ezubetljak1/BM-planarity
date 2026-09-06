#include "bm/BmKuratowskiExtractor.hpp"

#include "bm/BmKuratowskiConnectionFinder.hpp"
#include "bm/BmKuratowskiIsolationPreparation.hpp"
#include "bm/BmKuratowskiPathMarker.hpp"
#include "bm/KuratowskiCertificateVerifier.hpp"

#include <algorithm>
#include <initializer_list>
#include <optional>
#include <stdexcept>
#include <vector>

namespace bm {

namespace {

using Mark = BmKuratowskiObstructionMark;

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

int internalVertex(
    const BmEmbeddingState& state,
    int originalVertexId
) {
    return state.partialEmbedding().originalInternalVertex(originalVertexId);
}

int rootParent(
    const BmEmbeddingState& state,
    const BmKuratowskiExtractionContext& context
) {
    return state.bicompRoot(context.centralRootId).parentVertex;
}

int vertexWithLowestDfi(
    const DfsInfo& dfsInfo,
    std::initializer_list<int> vertices
) {
    return *std::min_element(
        vertices.begin(),
        vertices.end(),
        [&](int first, int second) {
            return dfsInfo.dfsIndex[first]
                < dfsInfo.dfsIndex[second];
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
            return dfsInfo.dfsIndex[first]
                < dfsInfo.dfsIndex[second];
        }
    );
}

bool hasLowerDfi(
    const DfsInfo& dfsInfo,
    int first,
    int second
) {
    return dfsInfo.dfsIndex[first]
        < dfsInfo.dfsIndex[second];
}

void markConnection(
    BmKuratowskiPathMarker& marker,
    const BmOriginalBackEdgeConnection& connection
) {
    marker.markOriginalEdge(connection.edgeId);
}

void markOriginalEdges(
    BmKuratowskiPathMarker& marker,
    const std::vector<int>& edgeIds
) {
    for (int edgeId : edgeIds) {
        marker.markOriginalEdge(edgeId);
    }
}

void markExternalFacePath(
    const BmEmbeddingState& state,
    const BmKuratowskiExtractionContext& context,
    BmKuratowskiPathMarker& marker,
    int startOriginalVertex,
    int endOriginalVertex
) {
    marker.markRealExternalFacePath(
        {internalVertex(state, startOriginalVertex), 1},
        internalVertex(state, endOriginalVertex)
    );
}

void markExternalFacePathFromRoot(
    const BmEmbeddingState& state,
    const BmKuratowskiExtractionContext& context,
    BmKuratowskiPathMarker& marker,
    int endOriginalVertex
) {
    marker.markRealExternalFacePath(
        {context.centralRootInternalVertexId, 1},
        internalVertex(state, endOriginalVertex)
    );
}

void markExternalFacePathToRoot(
    const BmEmbeddingState& state,
    const BmKuratowskiExtractionContext& context,
    BmKuratowskiPathMarker& marker,
    int startOriginalVertex
) {
    marker.markRealExternalFacePath(
        {internalVertex(state, startOriginalVertex), 1},
        context.centralRootInternalVertexId
    );
}

void markWholeCentralExternalFace(
    const BmKuratowskiExtractionContext& context,
    BmKuratowskiPathMarker& marker
) {
    marker.markRealExternalFacePath(
        {context.centralRootInternalVertexId, 1},
        context.centralRootInternalVertexId
    );
}

struct CommonConnections {
    BmOriginalBackEdgeConnection uxDx;
    BmOriginalBackEdgeConnection uyDy;
    BmOriginalBackEdgeConnection vDw;
};

CommonConnections findCommonConnections(
    const BmEmbeddingState& state,
    const BmKuratowskiExtractionContext& context
) {
    const int x = originalVertex(state, context.x);
    const int y = originalVertex(state, context.y);
    const int w = context.pertinentVertex;

    return {
        requireConnection(
            BmKuratowskiConnectionFinder::findToAncestor(state, context.currentVertex, x),
            "Kuratowski isolation requires an X-to-ancestor connection."
        ),
        requireConnection(
            BmKuratowskiConnectionFinder::findToAncestor(state, context.currentVertex, y),
            "Kuratowski isolation requires a Y-to-ancestor connection."
        ),
        requireConnection(
            BmKuratowskiConnectionFinder::findToCurrentVertex(state, context.currentVertex, w),
            "Kuratowski isolation requires a current-vertex-to-W-subtree connection."
        )
    };
}

void markCommonDescendantPaths(
    const BmEmbeddingState& state,
    const BmKuratowskiExtractionContext& context,
    const CommonConnections& connections,
    BmKuratowskiPathMarker& marker
) {
    marker.markDfsPath(originalVertex(state, context.x), connections.uxDx.descendantVertex);
    marker.markDfsPath(originalVertex(state, context.y), connections.uyDy.descendantVertex);
    marker.markDfsPath(context.pertinentVertex, connections.vDw.descendantVertex);
}

void markCommonConnections(
    const CommonConnections& connections,
    BmKuratowskiPathMarker& marker
) {
    markConnection(marker, connections.uxDx);
    markConnection(marker, connections.uyDy);
    markConnection(marker, connections.vDw);
}

KuratowskiCertificate analyzeMarked(
    const BmEmbeddingState& state,
    const BmKuratowskiPathMarker& marker
) {
    return KuratowskiCertificateVerifier::analyze(
        state.graph(),
        marker.markedOriginalEdgeIds()
    );
}

Mark obstructionMark(
    const BmEmbeddingState& state,
    const BmKuratowskiExtractionContext& context,
    int originalVertexId
) {
    return context.obstructionMarksByInternalVertex[
        internalVertex(state, originalVertexId)
    ];
}

} // namespace

KuratowskiCertificate BmKuratowskiExtractor::extract(
    const BmEmbeddingState& state,
    const BmWalkdownFailure& failure
) {
    BmPreparedKuratowskiIsolation prepared =
        BmKuratowskiIsolationPreparation::prepare(state, failure);

    switch (prepared.minorType) {
    case BmKuratowskiMinorType::A:
        return isolateMinorA(prepared.orientedState, prepared.context);
    case BmKuratowskiMinorType::B:
        return isolateMinorB(prepared.orientedState, prepared.context);
    case BmKuratowskiMinorType::C:
        return isolateMinorC(prepared.orientedState, prepared.context);
    case BmKuratowskiMinorType::D:
        return isolateMinorD(prepared.orientedState, prepared.context);
    case BmKuratowskiMinorType::E:
        return isolateMinorE(prepared.orientedState, prepared.context);
    default:
        throw std::logic_error("Kuratowski obstruction could not be classified as A-E.");
    }
}

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
    const int r = rootParent(state, context);

    const CommonConnections connections = findCommonConnections(state, context);

    BmKuratowskiPathMarker marker(state);
    markWholeCentralExternalFace(context, marker);

    marker.markDfsPath(
        vertexWithLowestDfi(
            state.dfsInfo(),
            {connections.uxDx.ancestorVertex, connections.uyDy.ancestorVertex}
        ),
        r
    );
    marker.markDfsPath(x, connections.uxDx.descendantVertex);
    marker.markDfsPath(y, connections.uyDy.descendantVertex);
    marker.markDfsPath(w, connections.vDw.descendantVertex);

    markCommonConnections(connections, marker);
    return analyzeMarked(state, marker);
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
    const int uz = state.dfsInfo().lowpointVertex[subtreeRoot];

    const BmOriginalBackEdgeConnection uxDx = requireConnection(
        BmKuratowskiConnectionFinder::findToAncestor(state, context.currentVertex, x),
        "Minor B requires an X-to-ancestor connection."
    );
    const BmOriginalBackEdgeConnection uyDy = requireConnection(
        BmKuratowskiConnectionFinder::findToAncestor(state, context.currentVertex, y),
        "Minor B requires a Y-to-ancestor connection."
    );
    // Reference Minor B deliberately uses the last future-pertinent child
    // bicomp at W. A direct W edge would select the wrong subdivision path.
    const BmOriginalBackEdgeConnection vDw = requireConnection(
        BmKuratowskiConnectionFinder::findToSubtree(state, context.currentVertex, subtreeRoot),
        "Minor B requires a current-vertex-to-W-subtree connection."
    );
    const BmOriginalBackEdgeConnection uzDz = requireConnection(
        BmKuratowskiConnectionFinder::findToSubtree(state, uz, subtreeRoot),
        "Minor B requires a lowpoint-ancestor-to-W-subtree connection."
    );

    BmKuratowskiPathMarker marker(state);
    markWholeCentralExternalFace(context, marker);

    marker.markDfsPath(
        vertexWithLowestDfi(
            state.dfsInfo(),
            {uxDx.ancestorVertex, uyDy.ancestorVertex, uzDz.ancestorVertex}
        ),
        vertexWithHighestDfi(
            state.dfsInfo(),
            {uxDx.ancestorVertex, uyDy.ancestorVertex, uzDz.ancestorVertex}
        )
    );
    marker.markDfsPath(x, uxDx.descendantVertex);
    marker.markDfsPath(y, uyDy.descendantVertex);
    marker.markDfsPath(w, vDw.descendantVertex);
    marker.markDfsPath(w, uzDz.descendantVertex);

    markConnection(marker, uxDx);
    markConnection(marker, uyDy);
    markConnection(marker, vDw);
    markConnection(marker, uzDz);
    return analyzeMarked(state, marker);
}

KuratowskiCertificate BmKuratowskiExtractor::isolateMinorC(
    const BmEmbeddingState& state,
    const BmKuratowskiExtractionContext& context
) {
    const int x = originalVertex(state, context.x);
    const int y = originalVertex(state, context.y);
    const int r = rootParent(state, context);
    const CommonConnections connections = findCommonConnections(state, context);

    BmKuratowskiPathMarker marker(state);
    markOriginalEdges(marker, context.xyPathOriginalEdgeIds);

    if (obstructionMark(state, context, context.px) == Mark::HighRxw) {
        const int highY = obstructionMark(state, context, context.py) == Mark::HighRyw
            ? context.py
            : y;
        markExternalFacePathFromRoot(state, context, marker, highY);
    } else {
        markExternalFacePathToRoot(state, context, marker, x);
    }

    markCommonDescendantPaths(state, context, connections, marker);
    marker.markDfsPath(
        vertexWithLowestDfi(
            state.dfsInfo(),
            {connections.uxDx.ancestorVertex, connections.uyDy.ancestorVertex}
        ),
        r
    );
    markCommonConnections(connections, marker);

    return analyzeMarked(state, marker);
}

KuratowskiCertificate BmKuratowskiExtractor::isolateMinorD(
    const BmEmbeddingState& state,
    const BmKuratowskiExtractionContext& context
) {
    const int x = originalVertex(state, context.x);
    const int y = originalVertex(state, context.y);
    const int r = rootParent(state, context);
    const CommonConnections connections = findCommonConnections(state, context);

    BmKuratowskiPathMarker marker(state);
    markOriginalEdges(marker, context.xyPathOriginalEdgeIds);
    markOriginalEdges(marker, context.zToRootOriginalEdgeIds);
    markExternalFacePath(state, context, marker, x, y);
    marker.markDfsPath(
        vertexWithLowestDfi(
            state.dfsInfo(),
            {connections.uxDx.ancestorVertex, connections.uyDy.ancestorVertex}
        ),
        r
    );
    markCommonDescendantPaths(state, context, connections, marker);
    markCommonConnections(connections, marker);

    return analyzeMarked(state, marker);
}

KuratowskiCertificate BmKuratowskiExtractor::isolateMinorE(
    const BmEmbeddingState& state,
    const BmKuratowskiExtractionContext& context
) {
    const DfsInfo& dfsInfo = state.dfsInfo();
    const int x = originalVertex(state, context.x);
    const int y = originalVertex(state, context.y);
    const int w = context.pertinentVertex;
    const int r = rootParent(state, context);

    CommonConnections connections = findCommonConnections(state, context);
    BmOriginalBackEdgeConnection uzDz = requireConnection(
        BmKuratowskiConnectionFinder::findToAncestor(state, context.currentVertex, context.z),
        "Minor E requires a Z-to-ancestor connection."
    );

    // E1 reduces to Minor C after replacing the lower external-face endpoint
    // with the future-pertinent vertex Z.
    if (context.z != w) {
        BmKuratowskiExtractionContext reduced = context;
        const Mark zMark = obstructionMark(state, context, context.z);

        if (zMark == Mark::LowRxw) {
            reduced.x.internalVertexId = internalVertex(state, context.z);
            reduced.obstructionMarksByInternalVertex[
                internalVertex(state, context.px)
            ] = Mark::HighRxw;
        } else if (zMark == Mark::LowRyw) {
            reduced.y.internalVertexId = internalVertex(state, context.z);
            reduced.obstructionMarksByInternalVertex[
                internalVertex(state, context.py)
            ] = Mark::HighRyw;
        } else {
            throw std::logic_error("Minor E1 requires Z on a low external-face segment.");
        }

        return isolateMinorC(state, reduced);
    }

    const int maxUxy = vertexWithHighestDfi(
        dfsInfo,
        {connections.uxDx.ancestorVertex, connections.uyDy.ancestorVertex}
    );

    // E2 reduces to Minor A with u_z acting as the current vertex and d_z
    // acting as the current-to-W descendant. Preserve the already selected
    // X/Y connections exactly as the reference isolator does.
    if (hasLowerDfi(dfsInfo, maxUxy, uzDz.ancestorVertex)) {
        BmKuratowskiPathMarker reducedMarker(state);
        markWholeCentralExternalFace(context, reducedMarker);
        reducedMarker.markDfsPath(
            vertexWithLowestDfi(
                dfsInfo,
                {connections.uxDx.ancestorVertex, connections.uyDy.ancestorVertex}
            ),
            r
        );
        reducedMarker.markDfsPath(x, connections.uxDx.descendantVertex);
        reducedMarker.markDfsPath(y, connections.uyDy.descendantVertex);
        reducedMarker.markDfsPath(w, uzDz.descendantVertex);
        markConnection(reducedMarker, connections.uxDx);
        markConnection(reducedMarker, connections.uyDy);
        markConnection(reducedMarker, uzDz);
        return analyzeMarked(state, reducedMarker);
    }

    BmKuratowskiPathMarker marker(state);
    markOriginalEdges(marker, context.xyPathOriginalEdgeIds);

    const int minUxyz = vertexWithLowestDfi(
        dfsInfo,
        {connections.uxDx.ancestorVertex, connections.uyDy.ancestorVertex, uzDz.ancestorVertex}
    );
    const int maxUxyz = vertexWithHighestDfi(
        dfsInfo,
        {connections.uxDx.ancestorVertex, connections.uyDy.ancestorVertex, uzDz.ancestorVertex}
    );

    // E3: the third ancestor lies above one of u_x/u_y and those two differ.
    if (hasLowerDfi(dfsInfo, uzDz.ancestorVertex, maxUxy)
        && connections.uxDx.ancestorVertex != connections.uyDy.ancestorVertex) {
        if (hasLowerDfi(dfsInfo, connections.uxDx.ancestorVertex, connections.uyDy.ancestorVertex)) {
            markExternalFacePathFromRoot(state, context, marker, context.px);
            markExternalFacePath(state, context, marker, w, y);
        } else {
            markExternalFacePath(state, context, marker, x, w);
            markExternalFacePathToRoot(state, context, marker, context.py);
        }

        marker.markDfsPath(minUxyz, r);
    }
    // E4: one X-Y attachment point does not coincide with its stopping vertex.
    else if (x != context.px || y != context.py) {
        if (context.px != x) {
            markExternalFacePathFromRoot(state, context, marker, w);
            markExternalFacePathToRoot(state, context, marker, context.py);
        } else {
            markExternalFacePathFromRoot(state, context, marker, context.px);
            markExternalFacePathToRoot(state, context, marker, w);
        }

        marker.markDfsPath(minUxyz, maxUxyz);
    }
    // Base E isolates a K5 subdivision.
    else {
        markWholeCentralExternalFace(context, marker);
        marker.markDfsPath(minUxyz, r);
    }

    markCommonDescendantPaths(state, context, connections, marker);
    marker.markDfsPath(w, uzDz.descendantVertex);
    markCommonConnections(connections, marker);
    markConnection(marker, uzDz);

    return analyzeMarked(state, marker);
}

} // namespace bm
