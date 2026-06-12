#include "bm/BoyerMyrvoldPlanarity.hpp"

#include "bm/BmEmbeddingRecovery.hpp"
#include "bm/BmKuratowskiExtractor.hpp"
#include "bm/BmKuratowskiFailureFactory.hpp"
#include "bm/KuratowskiCertificateVerifier.hpp"
#include "bm/BmWalkdown.hpp"
#include "bm/BmWalkup.hpp"
#include "bm/SimpleGraphValidator.hpp"

#include <stdexcept>
#include <utility>
#include <vector>

namespace bm {

PlanarityResult BoyerMyrvoldPlanarity::run(const Graph& graph) const {
    SimpleGraphValidator::validate(graph);

    // A simple planar graph with n >= 3 has at most 3n - 6 edges. For a
    // denser input, any 3n - 5-edge subgraph is already non-planar. Run the
    // certifying BM core only on that linear-size prefix, then map the
    // witness back to original stable edge IDs.
    if (graph.vertexCount() >= 3) {
        const long long sparseCertificateEdgeLimit =
            3LL * static_cast<long long>(graph.vertexCount()) - 5LL;

        if (static_cast<long long>(graph.edgeCount()) > sparseCertificateEdgeLimit) {
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

            PlanarityResult sparseResult = run(sparseGraph);

            if (sparseResult.planar || !sparseResult.certificate.has_value()) {
                throw std::logic_error(
                    "A 3n-5 edge simple subgraph must produce a Kuratowski certificate."
                );
            }

            std::vector<int> mappedEdgeIds;
            mappedEdgeIds.reserve(sparseResult.certificate->edgeIds.size());

            for (int sparseEdgeId : sparseResult.certificate->edgeIds) {
                mappedEdgeIds.push_back(
                    originalEdgeIdBySparseEdge.at(static_cast<std::size_t>(sparseEdgeId))
                );
            }

            return makeNonPlanarResult(
                KuratowskiCertificateVerifier::analyze(graph, mappedEdgeIds)
            );
        }
    }

    DfsPreprocessor dfsPreprocessor;
    const DfsInfo dfsInfo = dfsPreprocessor.run(graph);

    BmEmbeddingState state(graph, dfsInfo);

    BmWalkup walkup;
    BmWalkdown walkdown;

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
                if (!result.failure.has_value()) {
                    throw std::logic_error("Walkdown failure has no Kuratowski extraction context.");
                }

                return makeNonPlanarResult(
                    BmKuratowskiExtractor::extract(state, *result.failure)
                );
            }
        }

        for (int backEdgeIndex :
             dfsInfo.backEdgeIndicesFromAncestor[static_cast<std::size_t>(vertex)]) {
            const DfsBackEdge& backEdge =
                dfsInfo.backEdges[static_cast<std::size_t>(backEdgeIndex)];

            if (!state.isOriginalEdgeEmbedded(backEdge.edgeId)) {
                return makeNonPlanarResult(
                    BmKuratowskiExtractor::extract(
                        state,
                        BmKuratowskiFailureFactory::fromUnembeddedBackedge(
                            state,
                            vertex,
                            backEdge
                        )
                    )
                );
            }
        }
    }

    return makePlanarResult(BmEmbeddingRecovery::recover(state));
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
