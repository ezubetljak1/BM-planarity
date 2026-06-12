#pragma once

#include "bm/BmEmbeddingState.hpp"
#include "bm/PlanarityResult.hpp"

#include <vector>

namespace bm {

class BmEmbeddingRecovery {
public:
    static PlanarEmbedding recover(BmEmbeddingState& state);

    // Normalizes lazy flip signs without joining bicomps. Useful on a copy
    // of a non-planar failure state before Kuratowski isolation.
    static void orientForIsolation(BmEmbeddingState& state);

private:
    static void orientRemainingBicomps(BmEmbeddingState& state);
    static void orientBicomp(BmEmbeddingState& state, int rootId, std::vector<bool>& visited);
    static void joinRemainingBicomps(BmEmbeddingState& state);
    static PlanarEmbedding buildPublicEmbedding(const BmEmbeddingState& state);
};

} // namespace bm
