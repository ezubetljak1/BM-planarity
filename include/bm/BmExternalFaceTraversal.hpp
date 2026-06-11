#pragma once

#include "bm/BmPartialEmbedding.hpp"

namespace bm {

struct BmExternalFacePosition {
    int internalVertexId = -1;

    // 0 or 1; link[0] / link[1] in BM paper.
    int linkIndex = -1;
};

class BmExternalFaceTraversal {
public:
    explicit BmExternalFaceTraversal(const BmPartialEmbedding& embedding);

    BmExternalFacePosition successor(BmExternalFacePosition position) const;

private:
    const BmPartialEmbedding* embedding_ = nullptr;

    void validatePosition(BmExternalFacePosition position) const;
};

} // namespace bm
