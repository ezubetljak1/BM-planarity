#include "layout/OgdfPlanarLayoutAdapter.hpp"

#include <ogdf/basic/Graph.h>
#include <ogdf/basic/GraphAttributes.h>
#include <ogdf/basic/List.h>
#include <ogdf/planarlayout/PlanarStraightLayout.h>

#include <algorithm>
#include <cmath>
#include <queue>
#include <stdexcept>
#include <utility>
#include <vector>

namespace bm::layout {

namespace {

struct RawComponentLayout {
    std::vector<std::pair<int, VertexPosition>> positions;

    double width = 0.0;
    double height = 0.0;
};

std::vector<std::vector<int>> findConnectedComponents(const Graph& graph) {
    const int vertexCount = graph.vertexCount();

    std::vector<bool> visited(vertexCount, false);

    std::vector<std::vector<int>> components;

    const auto& adjacency = graph.adjacencyEdgeIds();

    for (int start = 0; start < vertexCount; ++start) {
        if (visited[start])
            continue;

        std::vector<int> component;

        std::queue<int> pending;

        pending.push(start);

        visited[start] = true;

        while (!pending.empty()) {
            const int vertex = pending.front();

            pending.pop();

            component.push_back(vertex);

            for (int edgeId : adjacency[vertex]) {
                const int neighbor = graph.opposite(edgeId, vertex);

                if (visited[neighbor])
                    continue;

                visited[neighbor] = true;
                pending.push(neighbor);
            }
        }

        components.push_back(std::move(component));
    }

    return components;
}

RawComponentLayout layoutTrivialComponent(const std::vector<int>& vertices) {
    RawComponentLayout result;

    if (vertices.empty())
        return result;

    result.positions.push_back({vertices[0], {0.0, 0.0}});

    if (vertices.size() == 1)
        return result;

    result.positions.push_back({vertices[1], {120.0, 0.0}});

    result.width = 120.0;

    return result;
}

RawComponentLayout layoutNonTrivialComponent(const Graph& graph, const PlanarEmbedding& embedding,
                                             const std::vector<int>& vertices) {
    ogdf::Graph ogdfGraph;
    std::vector<ogdf::node> nodeByOriginalVertex(graph.vertexCount(), nullptr);
    std::vector<ogdf::edge> edgeByOriginalEdge(graph.edgeCount(), nullptr);
    std::vector<bool> belongsToComponent(graph.vertexCount(), false);

    for (int vertex : vertices) {
        belongsToComponent[vertex] = true;

        nodeByOriginalVertex[vertex] = ogdfGraph.newNode();
    }

    for (const bm::Edge& edge : graph.edges()) {
        const bool sourceBelongs = belongsToComponent[edge.u];

        const bool targetBelongs = belongsToComponent[edge.v];

        if (!sourceBelongs || !targetBelongs)
            continue;

        edgeByOriginalEdge[edge.id] =
            ogdfGraph.newEdge(nodeByOriginalVertex[edge.u], nodeByOriginalVertex[edge.v]);
    }

    for (int vertex : vertices) {
        const ogdf::node ogdfNode = nodeByOriginalVertex[vertex];

        ogdf::List<ogdf::adjEntry> adjacencyOrder;

        for (int originalEdgeId : embedding.clockwiseEdgesAroundVertex[vertex]) {
            const ogdf::edge ogdfEdge = edgeByOriginalEdge[originalEdgeId];

            if (ogdfEdge == nullptr)
                throw std::logic_error(
                    "Recovered embedding references an edge outside its connected component.");

            adjacencyOrder.pushBack(ogdfEdge->getAdj(ogdfNode));
        }

        if (adjacencyOrder.size() >= 2)
            ogdfGraph.sort(ogdfNode, adjacencyOrder);
    }

    ogdf::GraphAttributes attributes(ogdfGraph, ogdf::GraphAttributes::nodeGraphics |
                                                    ogdf::GraphAttributes::edgeGraphics);

    for (ogdf::node vertex : ogdfGraph.nodes) {
        attributes.width(vertex) = 36.0;

        attributes.height(vertex) = 36.0;
    }

    ogdf::PlanarStraightLayout layoutAlgorithm;

    layoutAlgorithm.separation(72.0);

    layoutAlgorithm.callFixEmbed(attributes);

    double minimumX = 0.0;
    double maximumX = 0.0;
    double minimumY = 0.0;
    double maximumY = 0.0;

    bool first = true;

    for (int originalVertex : vertices) {
        const ogdf::node ogdfNode = nodeByOriginalVertex[originalVertex];

        const double x = attributes.x(ogdfNode);

        const double y = attributes.y(ogdfNode);

        if (!std::isfinite(x) || !std::isfinite(y))
            throw std::logic_error("OGDF returned invalid layout coordinates.");

        if (first) {
            minimumX = maximumX = x;
            minimumY = maximumY = y;

            first = false;

            continue;
        }

        minimumX = std::min(minimumX, x);

        maximumX = std::max(maximumX, x);

        minimumY = std::min(minimumY, y);

        maximumY = std::max(maximumY, y);
    }

    RawComponentLayout result;

    result.width = maximumX - minimumX;

    result.height = maximumY - minimumY;

    for (int originalVertex : vertices) {
        const ogdf::node ogdfNode = nodeByOriginalVertex[originalVertex];

        result.positions.push_back({originalVertex,
                                    {attributes.x(ogdfNode) - minimumX,

                                     maximumY - attributes.y(ogdfNode)}});
    }

    return result;
}

RawComponentLayout layoutComponent(const Graph& graph, const PlanarEmbedding& embedding,
                                   const std::vector<int>& vertices) {
    if (vertices.size() <= 2)
        return layoutTrivialComponent(vertices);

    return layoutNonTrivialComponent(graph, embedding, vertices);
}

} // namespace

PlanarLayout OgdfPlanarLayoutAdapter::compute(const Graph& graph,
                                              const PlanarEmbedding& embedding) {
    if (embedding.clockwiseEdgesAroundVertex.size() != graph.vertexCount())
        throw std::invalid_argument("Embedding vertex count does not match graph.");

    PlanarLayout result;

    result.positionsByVertex.resize(graph.vertexCount());

    const auto components = findConnectedComponents(graph);

    constexpr double componentGap = 120.0;

    constexpr double minimumWidth = 100.0;

    constexpr double minimumHeight = 100.0;

    constexpr double maximumRowWidth = 1400.0;

    double cursorX = 0.0;
    double cursorY = 0.0;
    double rowHeight = 0.0;

    for (const auto& component : components) {
        const RawComponentLayout raw = layoutComponent(graph, embedding, component);

        const double packedWidth = std::max(raw.width, minimumWidth);

        const double packedHeight = std::max(raw.height, minimumHeight);

        if (cursorX > 0.0 && cursorX + packedWidth > maximumRowWidth) {
            cursorX = 0.0;

            cursorY += rowHeight + componentGap;

            rowHeight = 0.0;
        }

        for (const auto& [vertex, position] : raw.positions) {
            result.positionsByVertex[vertex] = {cursorX + position.x, cursorY + position.y};
        }

        cursorX += packedWidth + componentGap;

        rowHeight = std::max(rowHeight, packedHeight);
    }

    return result;
}

} // namespace bm::layout