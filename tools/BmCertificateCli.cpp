#include "bm/BoyerMyrvoldPlanarity.hpp"
#include "bm/Graph.hpp"
#include "bm/KuratowskiCertificateVerifier.hpp"

#include <exception>
#include <iostream>
#include <stdexcept>

using namespace bm;

int main() {
    try {
        int graphCount = 0;
        if (!(std::cin >> graphCount) || graphCount < 0) {
            throw std::invalid_argument("Expected a non-negative graph count.");
        }

        BoyerMyrvoldPlanarity algorithm;

        for (int graphIndex = 0; graphIndex < graphCount; ++graphIndex) {
            int vertexCount = 0;
            int edgeCount = 0;
            if (!(std::cin >> vertexCount >> edgeCount)) {
                throw std::invalid_argument("Expected graph vertex and edge counts.");
            }

            Graph graph(vertexCount);
            for (int edgeIndex = 0; edgeIndex < edgeCount; ++edgeIndex) {
                int first = -1;
                int second = -1;
                if (!(std::cin >> first >> second)) {
                    throw std::invalid_argument("Expected an edge endpoint pair.");
                }
                graph.addEdge(first, second);
            }

            const PlanarityResult result = algorithm.run(graph);
            if (result.planar) {
                std::cout << "PLANAR\n";
                continue;
            }

            if (!result.certificate.has_value()) {
                throw std::logic_error("Non-planar result has no Kuratowski certificate.");
            }

            KuratowskiCertificateVerifier::validate(graph, *result.certificate);

            std::cout << "NONPLANAR "
                      << (result.certificate->type == KuratowskiType::K5 ? "K5" : "K33")
                      << ' ' << result.certificate->edgeIds.size();

            for (int edgeId : result.certificate->edgeIds) {
                std::cout << ' ' << edgeId;
            }

            std::cout << '\n';
        }
    } catch (const std::exception& error) {
        std::cerr << "ERROR: " << error.what() << '\n';
        return 1;
    }

    return 0;
}
