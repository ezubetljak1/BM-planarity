#include "bm/KuratowskiCertificateVerifier.hpp"

#include "bm/SimpleGraphValidator.hpp"

#include <algorithm>
#include <array>
#include <deque>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace bm {

namespace {

void require(bool condition, const std::string& message) {
    if (!condition) {
        throw std::logic_error(message);
    }
}

struct CertificateAnalysis {
    KuratowskiType type = KuratowskiType::Unknown;
    std::vector<int> branchVertices;
};

CertificateAnalysis analyzeEdgeSet(
    const Graph& graph,
    const std::vector<int>& edgeIds
) {
    SimpleGraphValidator::validate(graph);

    require(!edgeIds.empty(), "Kuratowski certificate must contain at least one edge.");

    const int vertexCount = graph.vertexCount();
    const int edgeCount = graph.edgeCount();

    std::vector<bool> selectedEdge(edgeCount, false);
    std::vector<std::vector<std::pair<int, int>>> adjacency(
        vertexCount
    );

    for (int edgeId : edgeIds) {
        require(edgeId >= 0 && edgeId < edgeCount,
                "Kuratowski certificate contains an invalid original edge id.");
        require(!selectedEdge[edgeId],
                "Kuratowski certificate contains a duplicate original edge id.");

        selectedEdge[edgeId] = true;

        const Edge& edge = graph.edge(edgeId);
        adjacency[edge.u].push_back({edge.v, edgeId});
        adjacency[edge.v].push_back({edge.u, edgeId});
    }

    std::vector<int> nonIsolatedVertices;
    std::vector<int> branchVertices;

    for (int vertex = 0; vertex < vertexCount; ++vertex) {
        const int degree = adjacency[vertex].size();

        if (degree == 0) {
            continue;
        }

        nonIsolatedVertices.push_back(vertex);

        require(degree == 2 || degree == 3 || degree == 4,
                "Certificate vertex degree must be 2, 3 or 4 before suppressing subdivision vertices.");

        if (degree != 2) {
            branchVertices.push_back(vertex);
        }
    }

    require(!nonIsolatedVertices.empty(),
            "Kuratowski certificate does not contain a non-isolated vertex.");

    KuratowskiType type = KuratowskiType::Unknown;

    if (branchVertices.size() == 5) {
        for (int vertex : branchVertices) {
            require(adjacency[vertex].size() == 4,
                    "A K5 subdivision must have five degree-4 branch vertices.");
        }
        type = KuratowskiType::K5;
    } else if (branchVertices.size() == 6) {
        for (int vertex : branchVertices) {
            require(adjacency[vertex].size() == 3,
                    "A K3,3 subdivision must have six degree-3 branch vertices.");
        }
        type = KuratowskiType::K33;
    } else {
        throw std::logic_error(
            "After suppressing degree-2 subdivision vertices, the certificate must have five or six branch vertices."
        );
    }

    std::vector<int> branchIndex(vertexCount, -1);

    const int branchVertexCount = branchVertices.size();
    for (int index = 0; index < branchVertexCount; ++index) {
        branchIndex[branchVertices[index]] = index;
    }

    std::vector<bool> visitedCertificateEdge(edgeCount, false);
    std::set<std::pair<int, int>> kernelEdges;

    for (int startBranch : branchVertices) {
        for (const auto& [neighbor, firstEdgeId] : adjacency[startBranch]) {
            if (visitedCertificateEdge[firstEdgeId]) {
                continue;
            }

            int previous = startBranch;
            int current = neighbor;
            int currentEdgeId = firstEdgeId;

            while (true) {
                require(!visitedCertificateEdge[currentEdgeId],
                        "Two suppressed certificate paths reuse an original edge.");
                visitedCertificateEdge[currentEdgeId] = true;

                if (branchIndex[current] != -1) {
                    require(current != startBranch,
                            "A suppressed certificate path must connect two distinct branch vertices.");

                    const auto kernelEdge = std::minmax(startBranch, current);
                    require(kernelEdges.insert(kernelEdge).second,
                            "Multiple internally disjoint paths connect the same branch-vertex pair.");
                    break;
                }

                const auto& currentAdjacency = adjacency[current];
                require(currentAdjacency.size() == 2,
                        "Every internal subdivision vertex must have degree 2.");

                const auto& first = currentAdjacency[0];
                const auto& second = currentAdjacency[1];
                const auto& next = first.first == previous ? second : first;

                require(next.first != previous || second.first != previous,
                        "Subdivision path cannot immediately terminate without reaching a branch vertex.");

                previous = current;
                current = next.first;
                currentEdgeId = next.second;
            }
        }
    }

    for (int edgeId : edgeIds) {
        require(visitedCertificateEdge[edgeId],
                "Certificate contains an edge outside the suppressed Kuratowski kernel paths.");
    }

    if (type == KuratowskiType::K5) {
        require(kernelEdges.size() == 10,
                "A K5 subdivision must suppress to exactly ten kernel edges.");

        for (int first = 0; first < branchVertexCount; ++first) {
            for (int second = first + 1; second < branchVertexCount; ++second) {
                const auto edge = std::minmax(
                    branchVertices[first],
                    branchVertices[second]
                );
                require(kernelEdges.contains(edge),
                        "Suppressed K5 certificate is missing a branch-vertex pair.");
            }
        }
    } else {
        require(kernelEdges.size() == 9,
                "A K3,3 subdivision must suppress to exactly nine kernel edges.");

        std::map<int, std::vector<int>> kernelAdjacency;
        for (int vertex : branchVertices) {
            kernelAdjacency[vertex] = {};
        }

        for (const auto& [first, second] : kernelEdges) {
            kernelAdjacency[first].push_back(second);
            kernelAdjacency[second].push_back(first);
        }

        std::map<int, int> partition;
        std::deque<int> queue;

        partition[branchVertices.front()] = 0;
        queue.push_back(branchVertices.front());

        while (!queue.empty()) {
            const int vertex = queue.front();
            queue.pop_front();

            for (int neighbor : kernelAdjacency[vertex]) {
                const auto found = partition.find(neighbor);

                if (found == partition.end()) {
                    partition[neighbor] = 1 - partition[vertex];
                    queue.push_back(neighbor);
                } else {
                    require(found->second != partition[vertex],
                            "Suppressed K3,3 certificate kernel must be bipartite.");
                }
            }
        }

        require(partition.size() == branchVertices.size(),
                "Suppressed K3,3 certificate kernel must be connected.");

        int firstPartitionSize = 0;
        int secondPartitionSize = 0;

        for (int vertex : branchVertices) {
            require(kernelAdjacency[vertex].size() == 3,
                    "Every K3,3 kernel branch vertex must have degree 3.");

            if (partition[vertex] == 0) {
                ++firstPartitionSize;
            } else {
                ++secondPartitionSize;
            }
        }

        require(firstPartitionSize == 3 && secondPartitionSize == 3,
                "K3,3 kernel partitions must each contain exactly three vertices.");
    }

    std::sort(branchVertices.begin(), branchVertices.end());

    CertificateAnalysis result;
    result.type = type;
    result.branchVertices = std::move(branchVertices);
    return result;
}

} // namespace

void KuratowskiCertificateVerifier::validate(
    const Graph& graph,
    const KuratowskiCertificate& certificate
) {
    const CertificateAnalysis analysis = analyzeEdgeSet(graph, certificate.edgeIds);

    require(certificate.type == KuratowskiType::Unknown || certificate.type == analysis.type,
            "Declared Kuratowski type does not match the selected original edge set.");

    if (!certificate.branchVertices.empty()) {
        std::vector<int> expectedBranchVertices = certificate.branchVertices;
        std::sort(expectedBranchVertices.begin(), expectedBranchVertices.end());
        expectedBranchVertices.erase(
            std::unique(expectedBranchVertices.begin(), expectedBranchVertices.end()),
            expectedBranchVertices.end()
        );

        require(expectedBranchVertices == analysis.branchVertices,
                "Declared branch vertices do not match the selected original edge set.");
    }
}

KuratowskiCertificate KuratowskiCertificateVerifier::analyze(
    const Graph& graph,
    const std::vector<int>& edgeIds
) {
    const CertificateAnalysis analysis = analyzeEdgeSet(graph, edgeIds);

    KuratowskiCertificate certificate;
    certificate.type = analysis.type;
    certificate.edgeIds = edgeIds;
    certificate.branchVertices = analysis.branchVertices;
    return certificate;
}

} // namespace bm
