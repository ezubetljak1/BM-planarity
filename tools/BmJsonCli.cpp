#include "bm/BoyerMyrvoldPlanarity.hpp"
#include "bm/io/GraphJsonAdapter.hpp"

#include <nlohmann/json.hpp>

#include <exception>
#include <iostream>

int main() {
    try {
        const nlohmann::json input = nlohmann::json::parse(std::cin);

        bm::io::ParsedJsonGraph parsedGraph =
            bm::io::GraphJsonAdapter::fromJson(
                input
            );

        bm::BoyerMyrvoldPlanarity algorithm;

        const bm::PlanarityResult result = algorithm.run(parsedGraph.graph);

        std::cout << bm::io::GraphJsonAdapter::toJson(parsedGraph, result).dump(2) << '\n';

        return 0;
    } catch (const std::exception& error) {
        const nlohmann::json response = {
            {"schemaVersion", 1},
            {"ok", false},
            {
                "error",
                {
                    {"message", error.what()}
                }
            }
        };

        std::cout << response.dump(2) << '\n';

        return 1;
    }
}