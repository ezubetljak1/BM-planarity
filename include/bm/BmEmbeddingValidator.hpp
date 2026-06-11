#pragma once

#include "bm/BmEmbeddingState.hpp"
#include "bm/BmPartialEmbedding.hpp"

namespace bm {

class BmEmbeddingValidator {
public:
    static void validatePartialEmbedding(const BmPartialEmbedding& embedding);
    static void validateState(const BmEmbeddingState& state);
};

} // namespace bm
