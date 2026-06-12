#pragma once

#include "bm/Graph.hpp"
#include "bm/PlanarityResult.hpp"

#include <nlohmann/json_fwd.hpp>

#include <string>
#include <vector>

namespace bm::io {

struct JsonVertex {
    std::string id;
    std::string label;
};

struct JsonEdge {
    std::string id;
    std::string source;
    std::string target;
};

struct ParsedJsonGraph {
    Graph graph;
    std::vector<JsonVertex> vertices;
    std::vector<JsonEdge> edges;
};

class GraphJsonAdapter {
    public:
    static ParsedJsonGraph fromJson(const nlohmann::json& input);

    static nlohmann::json toJson(const ParsedJsonGraph& parsedGraph, const PlanarityResult& result);
};

} // namespace bm::io