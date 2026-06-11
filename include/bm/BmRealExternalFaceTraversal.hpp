#pragma once

#include "bm/BmPartialEmbedding.hpp"

namespace bm {

struct BmRealExternalFacePosition {
    int internalVertexId = -1;

    // 0 or 1: the external-face anchor used to enter this vertex.
    int incomingLinkIndex = -1;
};

class BmRealExternalFaceTraversal {
public:
    explicit BmRealExternalFaceTraversal(const BmPartialEmbedding& embedding);

    BmRealExternalFacePosition successor(BmRealExternalFacePosition position) const;

private:
    const BmPartialEmbedding* embedding_ = nullptr;

    void validatePosition(BmRealExternalFacePosition position) const;
};

} // namespace bm
