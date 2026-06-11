#pragma once

#include "bm/Graph.hpp"
#include "bm/PlanarityResult.hpp"

#include <vector>

namespace bm {

class KuratowskiCertificateVerifier {
public:
    static void validate(const Graph& graph, const KuratowskiCertificate& certificate);

    static KuratowskiCertificate analyze(
        const Graph& graph,
        const std::vector<int>& edgeIds
    );
};

} // namespace bm
