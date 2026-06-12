#include "bm/BmKuratowskiExtractor.hpp"

#include "bm/BmKuratowskiConnectionFinder.hpp"
#include "bm/BmKuratowskiIsolationPreparation.hpp"
#include "bm/BmKuratowskiPathMarker.hpp"
#include "bm/KuratowskiCertificateVerifier.hpp"

#include <algorithm>
#include <chrono>
#include <initializer_list>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

namespace bm {

namespace {

using Mark = BmKuratowskiObstructionMark;
using Clock = std::chrono::steady_clock;

std::int64_t elapsedNanoseconds(Clock::time_point started) {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
        Clock::now() - started
    ).count();
}

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

bool hasLowerDfi(
    const DfsInfo& dfsInfo,
    int first,
    int second
) {
    return dfsInfo.dfsIndex[static_cast<std::size_t>(first)]
        < dfsInfo.dfsIndex[static_cast<std::size_t>(second)];
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
    const BmKuratowskiPathMarker& marker,
    BmKuratowskiExtractionTimings* timings
) {
    const auto started = Clock::now();
    KuratowskiCertificate certificate = KuratowskiCertificateVerifier::analyze(
        state.graph(),
        marker.markedOriginalEdgeIds()
    );

    if (timings != nullptr) {
        timings->certificateVerificationNs += elapsedNanoseconds(started);
    }

    return certificate;
}

Mark obstructionMark(
    const BmEmbeddingState& state,
    const BmKuratowskiExtractionContext& context,
    int originalVertexId
) {
    return context.obstructionMarksByInternalVertex[
        static_cast<std::size_t>(internalVertex(state, originalVertexId))
    ];
}

} // namespace

KuratowskiCertificate BmKuratowskiExtractor::extract(
    const BmEmbeddingState& state,
    const BmWalkdownFailure& failure
) {
    return extractWithTimings(state, failure, nullptr);
}

BmProfiledKuratowskiExtraction BmKuratowskiExtractor::extractProfiled(
    const BmEmbeddingState& state,
    const BmWalkdownFailure& failure
) {
    BmKuratowskiExtractionTimings timings;
    KuratowskiCertificate certificate = extractWithTimings(state, failure, &timings);

    return {
        std::move(certificate),
        timings
    };
}

KuratowskiCertificate BmKuratowskiExtractor::extractWithTimings(
    const BmEmbeddingState& state,
    const BmWalkdownFailure& failure,
    BmKuratowskiExtractionTimings* timings
) {
    const auto preparationStarted = Clock::now();
    BmPreparedKuratowskiIsolation prepared =
        BmKuratowskiIsolationPreparation::prepare(state, failure, timings);

    if (timings != nullptr) {
        timings->preparationNs += elapsedNanoseconds(preparationStarted);
    }

    const std::int64_t verificationBefore = timings != nullptr
        ? timings->certificateVerificationNs
        : 0;
    const auto isolationStarted = Clock::now();

    KuratowskiCertificate certificate;

    switch (prepared.minorType) {
    case BmKuratowskiMinorType::A:
        certificate = isolateMinorA(prepared.orientedState, prepared.context, timings);
        break;
    case BmKuratowskiMinorType::B:
        certificate = isolateMinorB(prepared.orientedState, prepared.context, timings);
        break;
    case BmKuratowskiMinorType::C:
        certificate = isolateMinorC(prepared.orientedState, prepared.context, timings);
        break;
    case BmKuratowskiMinorType::D:
        certificate = isolateMinorD(prepared.orientedState, prepared.context, timings);
        break;
    case BmKuratowskiMinorType::E:
        certificate = isolateMinorE(prepared.orientedState, prepared.context, timings);
        break;
    default:
        throw std::logic_error("Kuratowski obstruction could not be classified as A-E.");
    }

    if (timings != nullptr) {
        const std::int64_t verificationElapsed =
            timings->certificateVerificationNs - verificationBefore;
        timings->isolationNs += elapsedNanoseconds(isolationStarted) - verificationElapsed;
    }

    return certificate;
}

KuratowskiCertificate BmKuratowskiExtractor::extractInitialMinor(
    const BmEmbeddingState& state,
    const BmWalkdownFailure& failure
) {
    const BmKuratowskiExtractionContext context =
        BmKuratowskiExtractionContextBuilder::initialize(state, failure);

    switch (BmKuratowskiMinorClassifier::classifyInitial(state, context)) {
    case BmKuratowskiMinorType::A:
        return isolateMinorA(state, context, nullptr);
    case BmKuratowskiMinorType::B:
        return isolateMinorB(state, context, nullptr);
    default:
        throw std::logic_error(
            "Kuratowski extraction requires the C-D-E internal X-Y path stage."
        );
    }
}

KuratowskiCertificate BmKuratowskiExtractor::isolateMinorA(
    const BmEmbeddingState& state,
    const BmKuratowskiExtractionContext& context,
    BmKuratowskiExtractionTimings* timings
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
    return analyzeMarked(state, marker, timings);
}

KuratowskiCertificate BmKuratowskiExtractor::isolateMinorB(
    const BmEmbeddingState& state,
    const BmKuratowskiExtractionContext& context,
    BmKuratowskiExtractionTimings* timings
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
    return analyzeMarked(state, marker, timings);
}

KuratowskiCertificate BmKuratowskiExtractor::isolateMinorC(
    const BmEmbeddingState& state,
    const BmKuratowskiExtractionContext& context,
    BmKuratowskiExtractionTimings* timings
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

    return analyzeMarked(state, marker, timings);
}

KuratowskiCertificate BmKuratowskiExtractor::isolateMinorD(
    const BmEmbeddingState& state,
    const BmKuratowskiExtractionContext& context,
    BmKuratowskiExtractionTimings* timings
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

    return analyzeMarked(state, marker, timings);
}

KuratowskiCertificate BmKuratowskiExtractor::isolateMinorE(
    const BmEmbeddingState& state,
    const BmKuratowskiExtractionContext& context,
    BmKuratowskiExtractionTimings* timings
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
                static_cast<std::size_t>(internalVertex(state, context.px))
            ] = Mark::HighRxw;
        } else if (zMark == Mark::LowRyw) {
            reduced.y.internalVertexId = internalVertex(state, context.z);
            reduced.obstructionMarksByInternalVertex[
                static_cast<std::size_t>(internalVertex(state, context.py))
            ] = Mark::HighRyw;
        } else {
            throw std::logic_error("Minor E1 requires Z on a low external-face segment.");
        }

        return isolateMinorC(state, reduced, timings);
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
        return analyzeMarked(state, reducedMarker, timings);
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

    return analyzeMarked(state, marker, timings);
}

} // namespace bm
