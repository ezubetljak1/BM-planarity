#pragma once

#include <optional>
#include <vector>

namespace bm {

enum class KuratowskiType { Unknown, K5, K33 };

struct PlanarEmbedding {
    // Final public embedding:
    // clockwise/cyclic edge order around each original vertex
    // For now this is only a placeholder and will be filled in recovery phase
    std::vector<std::vector<int>> clockwiseEdgesAroundVertex;
};

struct KuratowskiCertificate {
    KuratowskiType type = KuratowskiType::Unknown;

    // Original graph edge IDs that form the certificate
    std::vector<int> edgeIds;

    // Optional branch vertices for later verifier/extractor
    std::vector<int> branchVertices;
};

struct PlanarityResult {
    bool planar = false;

    std::optional<PlanarEmbedding> embedding;
    std::optional<KuratowskiCertificate> certificate;
};

} // namespace bm