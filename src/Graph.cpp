#include "bm/Graph.hpp"

namespace {

int validateVertexCount(int vertexCount) {
    if (vertexCount < 0)
        throw std::invalid_argument("Vertex count cannot be negative.");

    return vertexCount;
}

} // namespace

namespace bm {

Graph::Graph(int vertexCount)
    : vertexCount_(validateVertexCount(vertexCount)), adjacencyEdgeIds_(vertexCount_) {
    if (vertexCount < 0)
        throw std::invalid_argument("Vertex count cannot be negative.");
}

int Graph::addEdge(int u, int v) {
    validateVertex(u);
    validateVertex(v);

    if (u == v)
        throw std::invalid_argument("Self-loops are not supported.");

    const int edgeId = edges_.size();

    Edge edge;
    edge.id = edgeId;
    edge.u = u;
    edge.v = v;

    edges_.push_back(edge);

    adjacencyEdgeIds_[u].push_back(edgeId);
    adjacencyEdgeIds_[v].push_back(edgeId);

    return edgeId;
}

int Graph::vertexCount() const {
    return vertexCount_;
}

int Graph::edgeCount() const {
    return edges_.size();
}

const std::vector<Edge>& Graph::edges() const {
    return edges_;
}

const std::vector<std::vector<int>>& Graph::adjacencyEdgeIds() const {
    return adjacencyEdgeIds_;
}

const Edge& Graph::edge(int edgeId) const {
    if (edgeId < 0 || edgeId >= edges_.size())
        throw std::out_of_range("Invalid edge id.");

    return edges_[edgeId];
}

void Graph::validateVertex(int v) const {
    if (v < 0 || v >= vertexCount_)
        throw std::out_of_range("Invalid vertex id.");
}

int Graph::opposite(int edgeId, int vertex) const {
    const auto& e = edge(edgeId);

    if (e.u == vertex)
        return e.v;
    if (e.v == vertex)
        return e.u;

    throw std::invalid_argument("Vertex is not incident to edge.");
}

} // namespace bm