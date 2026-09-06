#include "bm/Graph.hpp"
#include "bm/KuratowskiCertificateVerifier.hpp"

#include <exception>
#include <iostream>
#include <vector>

int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    int caseCount = 0;

    if (!(std::cin >> caseCount) || caseCount < 0) {
        std::cerr << "Expected a non-negative case count.\n";
        return 2;
    }

    for (int caseIndex = 0; caseIndex < caseCount; ++caseIndex) {
        int vertexCount = 0;
        int edgeCount = 0;

        if (!(std::cin >> vertexCount >> edgeCount)
            || vertexCount < 0
            || edgeCount < 0) {
            std::cerr << "Invalid graph header at case " << caseIndex << ".\n";
            return 2;
        }

        try {
            bm::Graph graph(vertexCount);

            for (int edgeIndex = 0; edgeIndex < edgeCount; ++edgeIndex) {
                int first = -1;
                int second = -1;

                if (!(std::cin >> first >> second)) {
                    std::cerr << "Missing graph edge at case " << caseIndex << ".\n";
                    return 2;
                }

                graph.addEdge(first, second);
            }

            int certificateEdgeCount = 0;
            if (!(std::cin >> certificateEdgeCount) || certificateEdgeCount < 0) {
                std::cerr << "Invalid certificate edge count at case " << caseIndex << ".\n";
                return 2;
            }

            std::vector<int> certificateEdgeIds;
            certificateEdgeIds.reserve(certificateEdgeCount);

            for (int index = 0; index < certificateEdgeCount; ++index) {
                int edgeId = -1;
                if (!(std::cin >> edgeId)) {
                    std::cerr << "Missing certificate edge id at case " << caseIndex << ".\n";
                    return 2;
                }
                certificateEdgeIds.push_back(edgeId);
            }

            const bm::KuratowskiCertificate certificate =
                bm::KuratowskiCertificateVerifier::analyze(graph, certificateEdgeIds);

            std::cout << (certificate.type == bm::KuratowskiType::K5 ? "K5" : "K33")
                      << ' ' << certificate.branchVertices.size();

            for (int vertex : certificate.branchVertices) {
                std::cout << ' ' << vertex;
            }

            std::cout << '\n';
        } catch (const std::exception& ex) {
            std::cerr << "Case " << caseIndex << " failed: " << ex.what() << '\n';
            return 3;
        }
    }

    return 0;
}
