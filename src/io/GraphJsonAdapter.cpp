#include "bm/io/GraphJsonAdapter.hpp"

#include <nlohmann/json.hpp>

#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace bm::io {

namespace {

    using Json = nlohmann::json;

    const Json& requireArray(const Json& object, const char *fieldName) {
        if (!object.contains(fieldName) || !object.at(fieldName).is_array()) 
            throw std::invalid_argument(std::string("Expected array field `") + fieldName + "`.'");
        
        return object.at(fieldName);
    }

    std::string requireString(const Json& object, const char* fieldName) {
        if (!object.contains(fieldName) || !object.at(fieldName).is_string())
            throw std::invalid_argument(std::string("Expected string field `") + fieldName + "`.");

        const std::string value = object.at(fieldName).get<std::string>();

        if (value.empty())
            throw std::invalid_argument(std::string("Field `") + fieldName + "` cannot be empty.");

        return value;
    }

    std::string kuratowskiTypeToString(KuratowskiType type) {
        switch (type) {
            case KuratowskiType::K5:
                return "K5";

            case KuratowskiType::K33:
                return "K3_3";

            case KuratowskiType::Unknown:
                break;
        }

        return "UNKNOWN";
    }

    Json verticesToJson(const ParsedJsonGraph& parsedGraph) {
        Json vertices = Json::array();

        for (const JsonVertex &vertex : parsedGraph.vertices) {
            vertices.push_back({
                {"id", vertex.id},
                {"label", vertex.label}
            });
        }

        return vertices;
    }

    Json edgesToJson(const ParsedJsonGraph &parsedGraph) {
        Json edges = Json::array();

        for (const JsonEdge &edge : parsedGraph.edges) {
            edges.push_back({
                {"id", edge.id},
                {"source", edge.source},
                {"target", edge.target}
            });
        }

        return edges;
    }

} // namespace

ParsedJsonGraph GraphJsonAdapter::fromJson(const nlohmann::json& input) {
    if (!input.is_object())
        throw std::invalid_argument("Expected a JSON object.");
    
    const Json& vertexArray = requireArray(input, "vertices");
    const Json& edgeArray = requireArray(input, "edges");

    std::vector<JsonVertex> vertices;
    vertices.reserve(vertexArray.size());

    std::unordered_map<std::string, int> vertexIndexById;

    vertexIndexById.reserve(vertexArray.size());

    for (const Json& vertexJson : vertexArray){
        if (!vertexJson.is_object())
            throw std::invalid_argument("Each vertex must be a JSON object.");

        JsonVertex vertex;
        vertex.id = requireString(vertexJson, "id");
        vertex.label = vertexJson.contains("label") ? requireString(vertexJson, "label") : vertex.id;
        const int vertexIndex = vertices.size();

        const auto [iterator, inserted] = vertexIndexById.emplace(vertex.id, vertexIndex);
        (void) iterator;

        if (!inserted)
            throw std::invalid_argument("Duplicate vertex id: " + vertex.id);

        vertices.push_back(std::move(vertex));
    }

    Graph graph(vertices.size());
    std::vector<JsonEdge> edges;
    edges.reserve(edgeArray.size());

    std::unordered_map<std::string, int> edgeIndexById;
    edgeIndexById.reserve(edgeArray.size());

    for (const Json& edgeJson : edgeArray) {
        if (!edgeJson.is_object())
            throw std::invalid_argument("Each edge must be a JSON object.");

        JsonEdge edge;
        edge.id = requireString(edgeJson, "id");
        edge.source = requireString(edgeJson, "source");
        edge.target = requireString(edgeJson, "target");

        if (!edgeIndexById.emplace(edge.id, edges.size()).second)
            throw std::invalid_argument("Duplicate edge id: " + edge.id);

        const auto sourceIterator = vertexIndexById.find(edge.source);

        const auto targetIterator = vertexIndexById.find(edge.target);

        if (sourceIterator == vertexIndexById.end())
            throw std::invalid_argument("Unknown source vertex id: " + edge.source);

        if (targetIterator == vertexIndexById.end())
            throw std::invalid_argument("Unknown target vertex id: " + edge.target);

        const int graphEdgeId = graph.addEdge(sourceIterator -> second, targetIterator -> second);

        if (graphEdgeId != edges.size())
            throw std::logic_error("Graph changed stable edge ordering.");

        edges.push_back(std::move(edge));
    }

    return ParsedJsonGraph{std::move(graph), std::move(vertices), std::move(edges)};
}

nlohmann::json GraphJsonAdapter::toJson(const ParsedJsonGraph& parsedGraph, const PlanarityResult& result) {
    Json output = {
        {"schemaVersion", 1},
        {"ok", true},
        {"planar", result.planar},
        {"vertices", verticesToJson(parsedGraph)},
        {"edges", edgesToJson(parsedGraph)}
    };

    if (result.planar){
        if (!result.embedding.has_value())
            throw std::logic_error("Planar result has no recovered embedding.");

        const auto & rotationSystem = result.embedding -> clockwiseEdgesAroundVertex;

        if (rotationSystem.size() != parsedGraph.vertices.size())
            throw std::logic_error("Recovered embedding has invalid vertex count.");

        Json clockwiseEdgesAroundVertex = Json::object();

        for(int vertexIndex = 0; vertexIndex < rotationSystem.size(); vertexIndex++) {
            Json edgeIds = Json::array();

            for (int graphEdgeId : rotationSystem[vertexIndex]) {
                if (graphEdgeId < 0 || graphEdgeId >= parsedGraph.edges.size())
                    throw std::logic_error("Recovered embedding contains an invalid edge id.");

                edgeIds.push_back(parsedGraph.edges[graphEdgeId].id);
            }

            clockwiseEdgesAroundVertex[parsedGraph.vertices[vertexIndex].id] = std::move(edgeIds);
        }

        output["embedding"] = {
            {
                "clockwiseEdgesAroundVertex",
                std::move(clockwiseEdgesAroundVertex)
            }
        };

        return output;
    }

    if (!result.certificate.has_value()) 
        throw std::logic_error("Non-planar result has no Kuratowski certificate.");

    Json certificateEdgeIds = Json::array();

    for (int graphEdgeId : result.certificate->edgeIds) {
        if (graphEdgeId < 0 || graphEdgeId >= parsedGraph.edges.size())
            throw std::logic_error("Kuratowski certificate contains an invalid edge id.");

        certificateEdgeIds.push_back(parsedGraph.edges[graphEdgeId].id);
    }

    Json branchVertexIds = Json::array();

    for (int graphVertexId : result.certificate -> branchVertices) {
        if (graphVertexId < 0 || graphVertexId >= parsedGraph.vertices.size())
            throw std::logic_error("Kuratowski certificate contains an invalid branch vertex id.");

        branchVertexIds.push_back(parsedGraph.vertices[graphVertexId].id);
    }
    
    output["certificate"] = {
        {
            "type", 
            kuratowskiTypeToString(result.certificate -> type)
        },
        {
            "edgeIds",
            std::move(certificateEdgeIds)
        },
        {
            "branchVertexIds",
            std::move(branchVertexIds)
        }
    };

    return output;
}

} // namespace bm::io