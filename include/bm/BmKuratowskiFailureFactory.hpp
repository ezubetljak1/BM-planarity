#pragma once

#include "bm/BmEmbeddingState.hpp"
#include "bm/BmWalkdown.hpp"
#include "bm/DfsPreprocessor.hpp"

namespace bm {

class BmKuratowskiFailureFactory {
public:
    static BmWalkdownFailure fromUnembeddedBackedge(
        const BmEmbeddingState& state,
        int currentVertex,
        const DfsBackEdge& backEdge
    );

private:
    static int firstChildBelowAncestor(
        const DfsInfo& dfsInfo,
        int ancestor,
        int descendant
    );
};

} // namespace bm
