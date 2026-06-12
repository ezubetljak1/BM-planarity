#include "bm/BmKuratowskiIsolationPreparation.hpp"

#include "bm/BmEmbeddingRecovery.hpp"

#include <chrono>
#include <utility>

namespace bm {

namespace {

using Clock = std::chrono::steady_clock;

std::int64_t elapsedNanoseconds(Clock::time_point started) {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
        Clock::now() - started
    ).count();
}

} // namespace

BmPreparedKuratowskiIsolation BmKuratowskiIsolationPreparation::prepare(
    const BmEmbeddingState& state,
    const BmWalkdownFailure& failure,
    BmKuratowskiExtractionTimings* timings
) {
    const auto copyStarted = Clock::now();
    BmEmbeddingState orientedState = state;
    if (timings != nullptr) {
        timings->orientedStateCopyNs += elapsedNanoseconds(copyStarted);
    }

    const auto orientationStarted = Clock::now();
    BmEmbeddingRecovery::orientForIsolation(orientedState);
    if (timings != nullptr) {
        timings->orientationNs += elapsedNanoseconds(orientationStarted);
    }

    const auto contextStarted = Clock::now();
    BmKuratowskiExtractionContext context =
        BmKuratowskiExtractionContextBuilder::initialize(orientedState, failure);
    if (timings != nullptr) {
        timings->contextInitializationNs += elapsedNanoseconds(contextStarted);
    }

    const auto classificationStarted = Clock::now();
    const BmKuratowskiMinorType minorType =
        BmKuratowskiMinorClassifier::classifyComplete(orientedState, context, timings);
    if (timings != nullptr) {
        timings->minorClassificationNs += elapsedNanoseconds(classificationStarted);
    }

    return {
        std::move(orientedState),
        std::move(context),
        minorType
    };
}

} // namespace bm
