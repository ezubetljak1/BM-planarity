#include "bm/BmKuratowskiMinorClassifier.hpp"

#include "bm/BmKuratowskiInternalPathAnalyzer.hpp"

#include <chrono>
#include <stdexcept>

namespace bm {

namespace {

using Clock = std::chrono::steady_clock;

std::int64_t elapsedNanoseconds(Clock::time_point started) {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
        Clock::now() - started
    ).count();
}

void addElapsed(
    BmKuratowskiExtractionTimings* timings,
    std::int64_t BmKuratowskiExtractionTimings::* field,
    Clock::time_point started
) {
    if (timings != nullptr) {
        timings->*field += elapsedNanoseconds(started);
    }
}

} // namespace

BmKuratowskiMinorType BmKuratowskiMinorClassifier::classifyInitial(
    const BmEmbeddingState& state,
    const BmKuratowskiExtractionContext& context
) {
    state.validateVertex(context.currentVertex);

    if (context.centralRootId < 0) {
        throw std::invalid_argument("Kuratowski context has no central bicomp root.");
    }

    if (context.minorAConfiguration) {
        return BmKuratowskiMinorType::A;
    }

    if (context.pertinentVertex == -1) {
        throw std::logic_error("Kuratowski context has no pertinent vertex W.");
    }

    const auto& roots = state.vertexState(context.pertinentVertex).pertinentRoots;

    if (!roots.empty()) {
        const int lastRootId = roots.back();
        const int child = state.bicompRoot(lastRootId).childVertex;
        const int currentDfi = state.dfsInfo().dfsIndex[
            static_cast<std::size_t>(context.currentVertex)
        ];

        if (state.dfsInfo().lowpointDfi[static_cast<std::size_t>(child)] < currentDfi) {
            return BmKuratowskiMinorType::B;
        }
    }

    return BmKuratowskiMinorType::Unknown;
}

BmKuratowskiMinorType BmKuratowskiMinorClassifier::classifyComplete(
    const BmEmbeddingState& state,
    BmKuratowskiExtractionContext& context,
    BmKuratowskiExtractionTimings* timings
) {
    const auto initialStarted = Clock::now();
    const BmKuratowskiMinorType initial = classifyInitial(state, context);
    addElapsed(timings, &BmKuratowskiExtractionTimings::classifyInitialNs, initialStarted);

    if (initial != BmKuratowskiMinorType::Unknown) {
        return initial;
    }

    const auto externalFaceStarted = Clock::now();
    BmKuratowskiInternalPathAnalyzer::classifyExternalFaceVertices(state, context);
    addElapsed(
        timings,
        &BmKuratowskiExtractionTimings::classifyExternalFaceVerticesNs,
        externalFaceStarted
    );

    const auto highestXyStarted = Clock::now();
    BmKuratowskiInternalPathAnalyzer::findHighestXyPath(state, context);
    addElapsed(
        timings,
        &BmKuratowskiExtractionTimings::findHighestXyPathNs,
        highestXyStarted
    );

    if (context.px == -1 || context.py == -1) {
        throw std::logic_error("Non-planarity classification requires an internal X-Y path.");
    }

    const int pxInternal = state.partialEmbedding().originalInternalVertex(context.px);
    const int pyInternal = state.partialEmbedding().originalInternalVertex(context.py);

    const auto pxMark = context.obstructionMarksByInternalVertex[
        static_cast<std::size_t>(pxInternal)
    ];
    const auto pyMark = context.obstructionMarksByInternalVertex[
        static_cast<std::size_t>(pyInternal)
    ];

    if (pxMark == BmKuratowskiObstructionMark::HighRxw
        || pyMark == BmKuratowskiObstructionMark::HighRyw) {
        return BmKuratowskiMinorType::C;
    }

    const auto zToRootStarted = Clock::now();
    BmKuratowskiInternalPathAnalyzer::findZToRootPath(state, context);
    addElapsed(
        timings,
        &BmKuratowskiExtractionTimings::findZToRootPathNs,
        zToRootStarted
    );

    if (context.z != -1) {
        return BmKuratowskiMinorType::D;
    }

    const auto futurePertinentStarted = Clock::now();
    context.z = BmKuratowskiInternalPathAnalyzer::findFuturePertinentBelowXyPath(
        state,
        context
    );
    addElapsed(
        timings,
        &BmKuratowskiExtractionTimings::findFuturePertinentBelowXyPathNs,
        futurePertinentStarted
    );

    if (context.z != -1) {
        return BmKuratowskiMinorType::E;
    }

    throw std::logic_error("Could not classify the non-planarity obstruction as A-E.");
}

} // namespace bm
