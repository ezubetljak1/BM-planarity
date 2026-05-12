#pragma once

#include <vector>

#include "bm/BmPartialEmbedding.hpp"

namespace bm {

struct BmExternalFacePosition {
    int internalVertexId = -1;

    // 0 or 1; link[0] / link[1] in BM paper
    int linkIndex = -1;
};

struct BmExternalFaceStep {
    int fromInternalVertexId = -1;
    int toInternalVertexId = -1;

    int usedHalfEdgeId = -1;

    BmExternalFacePosition successor;
};

class BmExternalFaceTraversal {
public:
    explicit BmExternalFaceTraversal(const BmPartialEmbedding& embedding);

    BmExternalFacePosition successor(BmExternalFacePosition position) const;

    BmExternalFaceStep step(BmExternalFacePosition position) const;

    std::vector<BmExternalFaceStep> collectCycle(BmExternalFacePosition start, int maxSteps) const;

private:
    const BmPartialEmbedding* embedding_ = nullptr;

    void validatePosition(BmExternalFacePosition position) const;
};

} // namespace bm