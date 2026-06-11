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

    require(static_cast<int>(embedding.clockwiseEdgesAroundVertex.size()) == vertexCount,
            "Rotation system must contain one adjacency order for every graph vertex.");

    std::vector<std::array<int, 2>> positionByEndpoint(
        static_cast<std::size_t>(edgeCount), std::array<int, 2>{-1, -1});

    std::vector<int> occurrenceCount(static_cast<std::size_t>(edgeCount), 0);

    for (int vertex = 0; vertex < vertexCount; ++vertex) {
        const auto& rotation = embedding.clockwiseEdgesAroundVertex[static_cast<std::size_t>(vertex)];
        const auto& adjacency = graph.adjacencyEdgeIds()[static_cast<std::size_t>(vertex)];

        require(rotation.size() == adjacency.size(),
                "Rotation-system degree does not match graph degree.");

        for (int position = 0; position < static_cast<int>(rotation.size()); ++position) {
            const int edgeId = rotation[static_cast<std::size_t>(position)];

            require(edgeId >= 0 && edgeId < edgeCount,
                    "Rotation system contains an invalid original edge id.");

            const Edge& edge = graph.edge(edgeId);
            const int side = endpointSide(edge, vertex);
            int& storedPosition = positionByEndpoint[static_cast<std::size_t>(edgeId)][side];

            require(storedPosition == -1,
                    "Rotation system contains an edge more than once around one endpoint.");

            storedPosition = position;
            ++occurrenceCount[static_cast<std::size_t>(edgeId)];
        }
    }

    for (int edgeId = 0; edgeId < edgeCount; ++edgeId) {
        require(occurrenceCount[static_cast<std::size_t>(edgeId)] == 2,
                "Every original edge must appear exactly once around each endpoint.");
        require(positionByEndpoint[static_cast<std::size_t>(edgeId)][0] != -1
                    && positionByEndpoint[static_cast<std::size_t>(edgeId)][1] != -1,
                "Every original edge must appear around both endpoints.");
    }

    std::vector<int> componentOfVertex(static_cast<std::size_t>(vertexCount), -1);
    std::vector<int> verticesPerComponent;
    std::vector<int> edgesPerComponent;

    for (int start = 0; start < vertexCount; ++start) {
        if (componentOfVertex[static_cast<std::size_t>(start)] != -1) {
            continue;
        }

        const int component = static_cast<int>(verticesPerComponent.size());
        verticesPerComponent.push_back(0);
        edgesPerComponent.push_back(0);

        std::deque<int> queue;
        queue.push_back(start);
        componentOfVertex[static_cast<std::size_t>(start)] = component;

        while (!queue.empty()) {
            const int vertex = queue.front();
            queue.pop_front();

            ++verticesPerComponent[static_cast<std::size_t>(component)];
            edgesPerComponent[static_cast<std::size_t>(component)] +=
                static_cast<int>(graph.adjacencyEdgeIds()[static_cast<std::size_t>(vertex)].size());

            for (int edgeId : graph.adjacencyEdgeIds()[static_cast<std::size_t>(vertex)]) {
                const int neighbor = graph.opposite(edgeId, vertex);

                if (componentOfVertex[static_cast<std::size_t>(neighbor)] == -1) {
                    componentOfVertex[static_cast<std::size_t>(neighbor)] = component;
                    queue.push_back(neighbor);
                }
            }
        }

        edgesPerComponent[static_cast<std::size_t>(component)] /= 2;
    }

    std::vector<bool> visitedDart(static_cast<std::size_t>(2 * edgeCount), false);
    std::vector<int> faceCyclesPerComponent(verticesPerComponent.size(), 0);

    for (int startEdgeId = 0; startEdgeId < edgeCount; ++startEdgeId) {
        for (int startSide = 0; startSide <= 1; ++startSide) {
            const int startDart = 2 * startEdgeId + startSide;

            if (visitedDart[static_cast<std::size_t>(startDart)]) {
                continue;
            }

            const int component = componentOfVertex[static_cast<std::size_t>(
                sourceVertex(graph.edge(startEdgeId), startSide))];
            ++faceCyclesPerComponent[static_cast<std::size_t>(component)];

            int edgeId = startEdgeId;
            int side = startSide;
            int steps = 0;

            do {
                const int dart = 2 * edgeId + side;
                require(!visitedDart[static_cast<std::size_t>(dart)],
                        "Face traversal entered an already visited dart before closing its cycle.");
                visitedDart[static_cast<std::size_t>(dart)] = true;

                const Edge& edge = graph.edge(edgeId);
                const int target = targetVertex(edge, side);
                const auto& targetRotation =
                    embedding.clockwiseEdgesAroundVertex[static_cast<std::size_t>(target)];
                const int targetSide = endpointSide(edge, target);
                const int position = positionByEndpoint[static_cast<std::size_t>(edgeId)][targetSide];

                require(!targetRotation.empty(),
                        "A dart cannot arrive at a vertex with an empty rotation order.");

                const int nextPosition =
                    (position + 1) % static_cast<int>(targetRotation.size());
                edgeId = targetRotation[static_cast<std::size_t>(nextPosition)];
                side = endpointSide(graph.edge(edgeId), target);

                ++steps;
                require(steps <= 2 * edgeCount + 1,
                        "Face traversal did not close within the number of darts.");
            } while (edgeId != startEdgeId || side != startSide);
        }
    }

    for (int component = 0; component < static_cast<int>(verticesPerComponent.size()); ++component) {
        const int vertices = verticesPerComponent[static_cast<std::size_t>(component)];
        const int edges = edgesPerComponent[static_cast<std::size_t>(component)];
        const int faces = edges == 0
                              ? 1
                              : faceCyclesPerComponent[static_cast<std::size_t>(component)];

        require(vertices - edges + faces == 2,
                "Rotation system does not describe a planar embedding of a connected component.");
    }
}

} // namespace bm
