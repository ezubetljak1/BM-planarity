#pragma once

#include "bm/BmEmbeddingState.hpp"
#include "bm/PlanarityResult.hpp"

#include <vector>

namespace bm {

class BmEmbeddingRecovery {
public:
    static PlanarEmbedding recover(BmEmbeddingState& state);

private:
    static void orientRemainingBicomps(BmEmbeddingState& state);
    static void orientBicomp(BmEmbeddingState& state, int rootId, std::vector<bool>& visited);
    static void joinRemainingBicomps(BmEmbeddingState& state);
    static PlanarEmbedding buildPublicEmbedding(const BmEmbeddingState& state);
};

} // namespace bm
