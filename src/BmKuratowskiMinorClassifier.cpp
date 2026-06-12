#include "bm/BmKuratowskiMinorClassifier.hpp"

#include "bm/BmKuratowskiInternalPathAnalyzer.hpp"

#include <stdexcept>

namespace bm {

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
    BmKuratowskiExtractionContext& context
) {
    const BmKuratowskiMinorType initial = classifyInitial(state, context);

    if (initial != BmKuratowskiMinorType::Unknown) {
        return initial;
    }

    BmKuratowskiInternalPathAnalyzer::classifyExternalFaceVertices(state, context);
    BmKuratowskiInternalPathAnalyzer::findHighestXyPath(state, context);

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

    BmKuratowskiInternalPathAnalyzer::findZToRootPath(state, context);

    if (context.z != -1) {
        return BmKuratowskiMinorType::D;
    }

    context.z = BmKuratowskiInternalPathAnalyzer::findFuturePertinentBelowXyPath(
        state,
        context
    );

    if (context.z != -1) {
        return BmKuratowskiMinorType::E;
    }

    throw std::logic_error("Could not classify the non-planarity obstruction as A-E.");
}

} // namespace bm
