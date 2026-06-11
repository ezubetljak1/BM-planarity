#include "bm/BmKuratowskiIsolationPreparation.hpp"

#include "bm/BmEmbeddingRecovery.hpp"

#include <utility>

namespace bm {

BmPreparedKuratowskiIsolation BmKuratowskiIsolationPreparation::prepare(
    const BmEmbeddingState& state,
    const BmWalkdownFailure& failure
) {
    BmEmbeddingState orientedState = state;
    BmEmbeddingRecovery::orientForIsolation(orientedState);

    BmKuratowskiExtractionContext context =
        BmKuratowskiExtractionContextBuilder::initialize(orientedState, failure);

    const BmKuratowskiMinorType minorType =
        BmKuratowskiMinorClassifier::classifyComplete(orientedState, context);

    return {
        std::move(orientedState),
        std::move(context),
        minorType
    };
}

} // namespace bm
