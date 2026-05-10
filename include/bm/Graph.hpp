#pragma once

#include <stdexcept>
#include <vector>

namespace bm {

struct Edge {
    int id = -1;
    int u = -1;
    int v = -1;
};

class Graph {
public:
    explicit Graph(int vertexCount);

    int addEdge(int u, int v);

    int vertexCount() const;
    int edgeCount() const;

    const std::vector<Edge>& edges() const;
    const std::vector<std::vector<int>>& adjacencyEdgeIds() const;

    const Edge& edge(int edgeId) const;

    int opposite(int edgeId, int vertex) const;

private:
    int vertexCount_ = 0;
    std::vector<Edge> edges_;
    std::vector<std::vector<int>> adjacencyEdgeIds_;

    void validateVertex(int v) const;
};

} // namespace bm