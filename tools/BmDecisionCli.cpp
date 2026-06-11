#include "bm/BoyerMyrvoldPlanarity.hpp"
#include "bm/Graph.hpp"

#include <exception>
#include <iostream>

int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    int graphCount = 0;

    if (!(std::cin >> graphCount) || graphCount < 0) {
        std::cerr << "Expected a non-negative graph count.\n";
        return 2;
    }

    bm::BoyerMyrvoldPlanarity algorithm;

    for (int graphIndex = 0; graphIndex < graphCount; ++graphIndex) {
        int vertexCount = 0;
        int edgeCount = 0;

        if (!(std::cin >> vertexCount >> edgeCount)
            || vertexCount < 0
            || edgeCount < 0) {
            std::cerr << "Invalid graph header at case " << graphIndex << ".\n";
            return 2;
        }

        try {
            bm::Graph graph(vertexCount);

            for (int edgeIndex = 0; edgeIndex < edgeCount; ++edgeIndex) {
                int u = -1;
                int v = -1;

                if (!(std::cin >> u >> v)) {
                    std::cerr << "Missing edge " << edgeIndex
                              << " at case " << graphIndex << ".\n";
                    return 2;
                }

                graph.addEdge(u, v);
            }

            const bm::PlanarityResult result = algorithm.run(graph);
            std::cout << (result.planar ? '1' : '0') << '\n';
        } catch (const std::exception& ex) {
            std::cerr << "Case " << graphIndex << " failed: "
                      << ex.what() << '\n';
            return 3;
        }
    }

    return 0;
}
