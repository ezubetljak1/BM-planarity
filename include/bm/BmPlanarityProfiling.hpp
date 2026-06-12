#pragma once

#include "bm/PlanarityResult.hpp"

#include <cstdint>

namespace bm {

struct BmKuratowskiExtractionTimings {
    std::int64_t preparationNs = 0;
    std::int64_t orientedStateCopyNs = 0;
    std::int64_t orientationNs = 0;
    std::int64_t contextInitializationNs = 0;
    std::int64_t minorClassificationNs = 0;
    std::int64_t classifyInitialNs = 0;
    std::int64_t classifyExternalFaceVerticesNs = 0;
    std::int64_t findHighestXyPathNs = 0;
    std::int64_t findZToRootPathNs = 0;
    std::int64_t findFuturePertinentBelowXyPathNs = 0;

    std::int64_t isolationNs = 0;
    std::int64_t certificateVerificationNs = 0;
};

struct BmProfiledKuratowskiExtraction {
    KuratowskiCertificate certificate;
    BmKuratowskiExtractionTimings timings;
};

struct BmPlanarityPhaseTimings {
    std::int64_t totalNs = 0;

    std::int64_t validationNs = 0;
    std::int64_t denseShortcutOverheadNs = 0;
    std::int64_t dfsPreprocessingNs = 0;
    std::int64_t stateInitializationNs = 0;
    std::int64_t decisionCoreNs = 0;
    std::int64_t failureFactoryNs = 0;
    std::int64_t kuratowskiPreparationNs = 0;
    std::int64_t kuratowskiOrientedStateCopyNs = 0;
    std::int64_t kuratowskiOrientationNs = 0;
    std::int64_t kuratowskiContextInitializationNs = 0;
    std::int64_t kuratowskiMinorClassificationNs = 0;
    std::int64_t kuratowskiClassifyInitialNs = 0;
    std::int64_t kuratowskiClassifyExternalFaceVerticesNs = 0;
    std::int64_t kuratowskiFindHighestXyPathNs = 0;
    std::int64_t kuratowskiFindZToRootPathNs = 0;
    std::int64_t kuratowskiFindFuturePertinentBelowXyPathNs = 0;
    std::int64_t kuratowskiIsolationNs = 0;
    std::int64_t certificateVerificationNs = 0;
    std::int64_t embeddingRecoveryNs = 0;

    std::int64_t accountedNs() const {
        return validationNs
            + denseShortcutOverheadNs
            + dfsPreprocessingNs
            + stateInitializationNs
            + decisionCoreNs
            + failureFactoryNs
            + kuratowskiPreparationNs
            + kuratowskiIsolationNs
            + certificateVerificationNs
            + embeddingRecoveryNs;
    }
};

struct BmProfiledPlanarityResult {
    PlanarityResult result;
    BmPlanarityPhaseTimings timings;
};

} // namespace bm
