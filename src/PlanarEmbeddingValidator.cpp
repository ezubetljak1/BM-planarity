#include "bm/PlanarEmbeddingValidator.hpp"

#include <array>
#include <deque>
#include <stdexcept>
#include <string>
#include <vector>

namespace bm {

namespace {

void require(bool condition, const std::string& message) {
    if (!condition) {
        throw std::logic_error(message);
    }
}

int endpointSide(const Edge& edge, int vertex) {
    if (edge.u == vertex) {
        return 0;
    }

    if (edge.v == vertex) {
        return 1;
    }

    throw std::logic_error("Rotation system contains an edge that is not incident to its vertex.");
}

int sourceVertex(const Edge& edge, int side) {
    return side == 0 ? edge.u : edge.v;
}

int targetVertex(const Edge& edge, int side) {
    return side == 0 ? edge.v : edge.u;
}

} // namespace

void PlanarEmbeddingValidator::validate(const Graph& graph, const PlanarEmbedding& embedding) {
    const int vertexCount = graph.vertexCount();
    const int edgeCount = graph.edgeCount();

    const int embeddingVertexCount = embedding.clockwiseEdgesAroundVertex.size();
    require(embeddingVertexCount == vertexCount,
            "Rotation system must contain one adjacency order for every graph vertex.");

    std::vector<std::array<int, 2>> positionByEndpoint(
        edgeCount, std::array<int, 2>{-1, -1});

    std::vector<int> occurrenceCount(edgeCount, 0);

    for (int vertex = 0; vertex < vertexCount; ++vertex) {
        const auto& rotation = embedding.clockwiseEdgesAroundVertex[vertex];
        const auto& adjacency = graph.adjacencyEdgeIds()[vertex];

        require(rotation.size() == adjacency.size(),
                "Rotation-system degree does not match graph degree.");

        const int rotationSize = rotation.size();
        for (int position = 0; position < rotationSize; ++position) {
            const int edgeId = rotation[position];

            require(edgeId >= 0 && edgeId < edgeCount,
                    "Rotation system contains an invalid original edge id.");

            const Edge& edge = graph.edge(edgeId);
            const int side = endpointSide(edge, vertex);
            int& storedPosition = positionByEndpoint[edgeId][side];

            require(storedPosition == -1,
                    "Rotation system contains an edge more than once around one endpoint.");

            storedPosition = position;
            ++occurrenceCount[edgeId];
        }
    }

    for (int edgeId = 0; edgeId < edgeCount; ++edgeId) {
        require(occurrenceCount[edgeId] == 2,
                "Every original edge must appear exactly once around each endpoint.");
        require(positionByEndpoint[edgeId][0] != -1
                    && positionByEndpoint[edgeId][1] != -1,
                "Every original edge must appear around both endpoints.");
    }

    std::vector<int> componentOfVertex(vertexCount, -1);
    std::vector<int> verticesPerComponent;
    std::vector<int> edgesPerComponent;

    for (int start = 0; start < vertexCount; ++start) {
        if (componentOfVertex[start] != -1) {
            continue;
        }

        const int component = verticesPerComponent.size();
        verticesPerComponent.push_back(0);
        edgesPerComponent.push_back(0);

        std::deque<int> queue;
        queue.push_back(start);
        componentOfVertex[start] = component;

        while (!queue.empty()) {
            const int vertex = queue.front();
            queue.pop_front();

            ++verticesPerComponent[component];
            edgesPerComponent[component] +=
                graph.adjacencyEdgeIds()[vertex].size();

            for (int edgeId : graph.adjacencyEdgeIds()[vertex]) {
                const int neighbor = graph.opposite(edgeId, vertex);

                if (componentOfVertex[neighbor] == -1) {
                    componentOfVertex[neighbor] = component;
                    queue.push_back(neighbor);
                }
            }
        }

        edgesPerComponent[component] /= 2;
    }

    std::vector<bool> visitedDart(2 * edgeCount, false);
    std::vector<int> faceCyclesPerComponent(verticesPerComponent.size(), 0);

    for (int startEdgeId = 0; startEdgeId < edgeCount; ++startEdgeId) {
        for (int startSide = 0; startSide <= 1; ++startSide) {
            const int startDart = 2 * startEdgeId + startSide;

            if (visitedDart[startDart]) {
                continue;
            }

            const int component = componentOfVertex[
                sourceVertex(graph.edge(startEdgeId), startSide)];
            ++faceCyclesPerComponent[component];

            int edgeId = startEdgeId;
            int side = startSide;
            int steps = 0;

            do {
                const int dart = 2 * edgeId + side;
                require(!visitedDart[dart],
                        "Face traversal entered an already visited dart before closing its cycle.");
                visitedDart[dart] = true;

                const Edge& edge = graph.edge(edgeId);
                const int target = targetVertex(edge, side);
                const auto& targetRotation =
                    embedding.clockwiseEdgesAroundVertex[target];
                const int targetSide = endpointSide(edge, target);
                const int position = positionByEndpoint[edgeId][targetSide];

                require(!targetRotation.empty(),
                        "A dart cannot arrive at a vertex with an empty rotation order.");

                const int nextPosition =
                    (position + 1) % targetRotation.size();
                edgeId = targetRotation[nextPosition];
                side = endpointSide(graph.edge(edgeId), target);

                ++steps;
                require(steps <= 2 * edgeCount + 1,
                        "Face traversal did not close within the number of darts.");
            } while (edgeId != startEdgeId || side != startSide);
        }
    }

    const int componentCount = verticesPerComponent.size();
    for (int component = 0; component < componentCount; ++component) {
        const int vertices = verticesPerComponent[component];
        const int edges = edgesPerComponent[component];
        const int faces = edges == 0
                              ? 1
                              : faceCyclesPerComponent[component];

        require(vertices - edges + faces == 2,
                "Rotation system does not describe a planar embedding of a connected component.");
    }
}

} // namespace bm
