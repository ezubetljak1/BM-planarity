#include "bm/BoyerMyrvoldPlanarity.hpp"

#include "bm/BmEmbeddingRecovery.hpp"
#include "bm/BmKuratowskiExtractor.hpp"
#include "bm/BmKuratowskiFailureFactory.hpp"
#include "bm/KuratowskiCertificateVerifier.hpp"
#include "bm/BmWalkdown.hpp"
#include "bm/BmWalkup.hpp"
#include "bm/SimpleGraphValidator.hpp"

#include <chrono>
#include <stdexcept>
#include <utility>
#include <vector>

namespace bm {

namespace {

using Clock = std::chrono::steady_clock;

std::int64_t elapsedNanoseconds(Clock::time_point started) {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
        Clock::now() - started
    ).count();
}

void addElapsed(
    BmPlanarityPhaseTimings* timings,
    std::int64_t BmPlanarityPhaseTimings::* field,
    Clock::time_point started
) {
    if (timings != nullptr) {
        timings->*field += elapsedNanoseconds(started);
    }
}

KuratowskiCertificate extractCertificate(
    const BmEmbeddingState& state,
    const BmWalkdownFailure& failure,
    BmPlanarityPhaseTimings* timings
) {
    if (timings == nullptr) {
        return BmKuratowskiExtractor::extract(state, failure);
    }

    BmProfiledKuratowskiExtraction profiled =
        BmKuratowskiExtractor::extractProfiled(state, failure);

    timings->kuratowskiPreparationNs += profiled.timings.preparationNs;
    timings->kuratowskiOrientedStateCopyNs += profiled.timings.orientedStateCopyNs;
    timings->kuratowskiOrientationNs += profiled.timings.orientationNs;
    timings->kuratowskiContextInitializationNs += profiled.timings.contextInitializationNs;
    timings->kuratowskiMinorClassificationNs += profiled.timings.minorClassificationNs;
    timings->kuratowskiClassifyInitialNs += profiled.timings.classifyInitialNs;
    timings->kuratowskiClassifyExternalFaceVerticesNs += profiled.timings.classifyExternalFaceVerticesNs;
    timings->kuratowskiFindHighestXyPathNs += profiled.timings.findHighestXyPathNs;
    timings->kuratowskiFindZToRootPathNs += profiled.timings.findZToRootPathNs;
    timings->kuratowskiFindFuturePertinentBelowXyPathNs += profiled.timings.findFuturePertinentBelowXyPathNs;
    timings->kuratowskiIsolationNs += profiled.timings.isolationNs;
    timings->certificateVerificationNs += profiled.timings.certificateVerificationNs;

    return std::move(profiled.certificate);
}

} // namespace

PlanarityResult BoyerMyrvoldPlanarity::run(const Graph& graph) const {
    return runInternal(graph, nullptr);
}

BmProfiledPlanarityResult BoyerMyrvoldPlanarity::runProfiled(const Graph& graph) const {
    BmPlanarityPhaseTimings timings;
    const auto totalStarted = Clock::now();

    PlanarityResult result = runInternal(graph, &timings);
    timings.totalNs = elapsedNanoseconds(totalStarted);

    return {
        std::move(result),
        timings
    };
}

PlanarityResult BoyerMyrvoldPlanarity::runInternal(
    const Graph& graph,
    BmPlanarityPhaseTimings* timings
) const {
    const auto validationStarted = Clock::now();
    SimpleGraphValidator::validate(graph);
    addElapsed(timings, &BmPlanarityPhaseTimings::validationNs, validationStarted);

    // A simple planar graph with n >= 3 has at most 3n - 6 edges. For a
    // denser input, any 3n - 5-edge subgraph is already non-planar. Run the
    // certifying BM core only on that linear-size prefix, then map the
    // witness back to original stable edge IDs.
    if (graph.vertexCount() >= 3) {
        const long long sparseCertificateEdgeLimit =
            3LL * static_cast<long long>(graph.vertexCount()) - 5LL;

        if (static_cast<long long>(graph.edgeCount()) > sparseCertificateEdgeLimit) {
            const auto sparseConstructionStarted = Clock::now();
            const int sparseEdgeCount = static_cast<int>(sparseCertificateEdgeLimit);
            Graph sparseGraph(graph.vertexCount());
            std::vector<int> originalEdgeIdBySparseEdge;
            originalEdgeIdBySparseEdge.reserve(
                static_cast<std::size_t>(sparseEdgeCount)
            );

            for (int originalEdgeId = 0;
                 originalEdgeId < sparseEdgeCount;
                 ++originalEdgeId) {
                const Edge& edge = graph.edge(originalEdgeId);
                const int sparseEdgeId = sparseGraph.addEdge(edge.u, edge.v);

                if (sparseEdgeId != static_cast<int>(originalEdgeIdBySparseEdge.size())) {
                    throw std::logic_error("Sparse certificate subgraph changed stable edge ordering.");
                }

                originalEdgeIdBySparseEdge.push_back(originalEdgeId);
            }
            addElapsed(
                timings,
                &BmPlanarityPhaseTimings::denseShortcutOverheadNs,
                sparseConstructionStarted
            );

            PlanarityResult sparseResult = runInternal(sparseGraph, timings);

            if (sparseResult.planar || !sparseResult.certificate.has_value()) {
                throw std::logic_error(
                    "A 3n-5 edge simple subgraph must produce a Kuratowski certificate."
                );
            }

            const auto mappingStarted = Clock::now();
            std::vector<int> mappedEdgeIds;
            mappedEdgeIds.reserve(sparseResult.certificate->edgeIds.size());

            for (int sparseEdgeId : sparseResult.certificate->edgeIds) {
                mappedEdgeIds.push_back(
                    originalEdgeIdBySparseEdge.at(static_cast<std::size_t>(sparseEdgeId))
                );
            }
            addElapsed(
                timings,
                &BmPlanarityPhaseTimings::denseShortcutOverheadNs,
                mappingStarted
            );

            const auto verificationStarted = Clock::now();
            KuratowskiCertificate certificate =
                KuratowskiCertificateVerifier::analyze(graph, mappedEdgeIds);
            addElapsed(
                timings,
                &BmPlanarityPhaseTimings::certificateVerificationNs,
                verificationStarted
            );

            return makeNonPlanarResult(std::move(certificate));
        }
    }

    const auto preprocessingStarted = Clock::now();
    DfsPreprocessor dfsPreprocessor;
    const DfsInfo dfsInfo = dfsPreprocessor.run(graph);
    addElapsed(
        timings,
        &BmPlanarityPhaseTimings::dfsPreprocessingNs,
        preprocessingStarted
    );

    const auto stateInitializationStarted = Clock::now();
    BmEmbeddingState state(graph, dfsInfo);
    addElapsed(
        timings,
        &BmPlanarityPhaseTimings::stateInitializationNs,
        stateInitializationStarted
    );

    BmWalkup walkup;
    BmWalkdown walkdown;
    const auto decisionStarted = Clock::now();

    for (int dfi = dfsInfo.vertexCount - 1; dfi >= 0; --dfi) {
        const int vertex = dfsInfo.vertexAtDfsIndex[static_cast<std::size_t>(dfi)];

        for (int child : dfsInfo.children[static_cast<std::size_t>(vertex)]) {
            state.createTreeEdgeBicomp(vertex, child);
        }

        for (int backEdgeIndex :
             dfsInfo.backEdgeIndicesFromAncestor[static_cast<std::size_t>(vertex)]) {
            const DfsBackEdge& backEdge =
                dfsInfo.backEdges[static_cast<std::size_t>(backEdgeIndex)];

            walkup.run(state, vertex, backEdge.descendant, backEdge.edgeId);
        }

        for (int child : dfsInfo.children[static_cast<std::size_t>(vertex)]) {
            if (!state.hasPertinentRoots(child)) {
                continue;
            }

            const BmWalkdownResult result = walkdown.run(
                state,
                vertex,
                state.rootForChild(child)
            );

            if (!result.completed) {
                addElapsed(
                    timings,
                    &BmPlanarityPhaseTimings::decisionCoreNs,
                    decisionStarted
                );

                if (!result.failure.has_value()) {
                    throw std::logic_error("Walkdown failure has no Kuratowski extraction context.");
                }

                return makeNonPlanarResult(
                    extractCertificate(state, *result.failure, timings)
                );
            }
        }

        for (int backEdgeIndex :
             dfsInfo.backEdgeIndicesFromAncestor[static_cast<std::size_t>(vertex)]) {
            const DfsBackEdge& backEdge =
                dfsInfo.backEdges[static_cast<std::size_t>(backEdgeIndex)];

            if (!state.isOriginalEdgeEmbedded(backEdge.edgeId)) {
                addElapsed(
                    timings,
                    &BmPlanarityPhaseTimings::decisionCoreNs,
                    decisionStarted
                );

                const auto failureFactoryStarted = Clock::now();
                BmWalkdownFailure failure =
                    BmKuratowskiFailureFactory::fromUnembeddedBackedge(
                        state,
                        vertex,
                        backEdge
                    );
                addElapsed(
                    timings,
                    &BmPlanarityPhaseTimings::failureFactoryNs,
                    failureFactoryStarted
                );

                return makeNonPlanarResult(
                    extractCertificate(state, failure, timings)
                );
            }
        }
    }

    addElapsed(
        timings,
        &BmPlanarityPhaseTimings::decisionCoreNs,
        decisionStarted
    );

    const auto recoveryStarted = Clock::now();
    PlanarEmbedding embedding = BmEmbeddingRecovery::recover(state);
    addElapsed(
        timings,
        &BmPlanarityPhaseTimings::embeddingRecoveryNs,
        recoveryStarted
    );

    return makePlanarResult(std::move(embedding));
}

PlanarityResult BoyerMyrvoldPlanarity::makePlanarResult(PlanarEmbedding embedding) {
    PlanarityResult result;
    result.planar = true;
    result.embedding = std::move(embedding);
    result.certificate = std::nullopt;

    return result;
}

PlanarityResult BoyerMyrvoldPlanarity::makeNonPlanarResult(
    KuratowskiCertificate certificate
) {
    PlanarityResult result;
    result.planar = false;
    result.embedding = std::nullopt;
    result.certificate = std::move(certificate);

    return result;
}

} // namespace bm
