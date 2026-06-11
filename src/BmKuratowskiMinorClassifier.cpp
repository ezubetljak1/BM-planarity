#include "bm/BmKuratowskiMinorClassifier.hpp"

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

} // namespace bm
