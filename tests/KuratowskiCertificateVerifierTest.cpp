#include "TestSupport.hpp"

#include "bm/Graph.hpp"
#include "bm/KuratowskiCertificateVerifier.hpp"
#include "bm/PlanarityResult.hpp"

#include <stdexcept>
#include <vector>

using namespace bm;

namespace {

bool throwsLogicError(const Graph& graph, const KuratowskiCertificate& certificate) {
    try {
        KuratowskiCertificateVerifier::validate(graph, certificate);
        return false;
    } catch (const std::logic_error&) {
        return true;
    }
}

} // namespace

BM_TEST(KuratowskiCertificateVerifierAcceptsK5) {
    Graph graph(5);
    std::vector<int> edges;

    for (int first = 0; first < 5; ++first) {
        for (int second = first + 1; second < 5; ++second) {
            edges.push_back(graph.addEdge(first, second));
        }
    }

    const KuratowskiCertificate certificate =
        KuratowskiCertificateVerifier::analyze(graph, edges);

    BM_ASSERT(certificate.type == KuratowskiType::K5);
    BM_ASSERT(certificate.branchVertices.size() == 5);
    KuratowskiCertificateVerifier::validate(graph, certificate);
}

BM_TEST(KuratowskiCertificateVerifierAcceptsSubdividedK5) {
    Graph graph(6);
    std::vector<int> edges;

    edges.push_back(graph.addEdge(0, 5));
    edges.push_back(graph.addEdge(5, 1));

    for (int first = 0; first < 5; ++first) {
        for (int second = first + 1; second < 5; ++second) {
            if (first == 0 && second == 1) {
                continue;
            }
            edges.push_back(graph.addEdge(first, second));
        }
    }

    const KuratowskiCertificate certificate =
        KuratowskiCertificateVerifier::analyze(graph, edges);

    BM_ASSERT(certificate.type == KuratowskiType::K5);
    BM_ASSERT(certificate.branchVertices.size() == 5);
    KuratowskiCertificateVerifier::validate(graph, certificate);
}

BM_TEST(KuratowskiCertificateVerifierAcceptsK33) {
    Graph graph(6);
    std::vector<int> edges;

    for (int left = 0; left < 3; ++left) {
        for (int right = 3; right < 6; ++right) {
            edges.push_back(graph.addEdge(left, right));
        }
    }

    const KuratowskiCertificate certificate =
        KuratowskiCertificateVerifier::analyze(graph, edges);

    BM_ASSERT(certificate.type == KuratowskiType::K33);
    BM_ASSERT(certificate.branchVertices.size() == 6);
    KuratowskiCertificateVerifier::validate(graph, certificate);
}

BM_TEST(KuratowskiCertificateVerifierAcceptsSubdividedK33) {
    Graph graph(7);
    std::vector<int> edges;

    edges.push_back(graph.addEdge(0, 6));
    edges.push_back(graph.addEdge(6, 3));

    for (int left = 0; left < 3; ++left) {
        for (int right = 3; right < 6; ++right) {
            if (left == 0 && right == 3) {
                continue;
            }
            edges.push_back(graph.addEdge(left, right));
        }
    }

    const KuratowskiCertificate certificate =
        KuratowskiCertificateVerifier::analyze(graph, edges);

    BM_ASSERT(certificate.type == KuratowskiType::K33);
    KuratowskiCertificateVerifier::validate(graph, certificate);
}

BM_TEST(KuratowskiCertificateVerifierRejectsMissingKernelEdge) {
    Graph graph(5);
    std::vector<int> edges;

    for (int first = 0; first < 5; ++first) {
        for (int second = first + 1; second < 5; ++second) {
            edges.push_back(graph.addEdge(first, second));
        }
    }

    edges.pop_back();

    KuratowskiCertificate certificate;
    certificate.edgeIds = edges;

    BM_ASSERT(throwsLogicError(graph, certificate));
}

BM_TEST(KuratowskiCertificateVerifierRejectsDuplicateEdgeId) {
    Graph graph(6);
    std::vector<int> edges;

    for (int left = 0; left < 3; ++left) {
        for (int right = 3; right < 6; ++right) {
            edges.push_back(graph.addEdge(left, right));
        }
    }

    edges.push_back(edges.front());

    KuratowskiCertificate certificate;
    certificate.edgeIds = edges;

    BM_ASSERT(throwsLogicError(graph, certificate));
}

BM_TEST(KuratowskiCertificateVerifierRejectsIncorrectDeclaredType) {
    Graph graph(5);
    std::vector<int> edges;

    for (int first = 0; first < 5; ++first) {
        for (int second = first + 1; second < 5; ++second) {
            edges.push_back(graph.addEdge(first, second));
        }
    }

    KuratowskiCertificate certificate;
    certificate.type = KuratowskiType::K33;
    certificate.edgeIds = edges;

    BM_ASSERT(throwsLogicError(graph, certificate));
}
