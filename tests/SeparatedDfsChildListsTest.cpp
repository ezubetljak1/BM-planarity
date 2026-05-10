#include "TestSupport.hpp"

#include "bm/DfsPreprocessor.hpp"
#include "bm/Graph.hpp"
#include "bm/SeparatedDfsChildLists.hpp"

using namespace bm;

BM_TEST(SeparatedDfsChildListsBuildsListsFromDfsInfo) {
    Graph graph(4);
    graph.addEdge(0, 1);
    graph.addEdge(0, 2);
    graph.addEdge(0, 3);

    DfsPreprocessor preprocessor;
    const DfsInfo dfsInfo = preprocessor.run(graph);

    SeparatedDfsChildLists lists(dfsInfo);

    const auto children = lists.toVector(0);

    BM_ASSERT(children.size() == 3);
    BM_ASSERT(lists.containsChild(1));
    BM_ASSERT(lists.containsChild(2));
    BM_ASSERT(lists.containsChild(3));
}

BM_TEST(SeparatedDfsChildListsRemovesMiddleChild) {
    Graph graph(4);
    graph.addEdge(0, 1);
    graph.addEdge(0, 2);
    graph.addEdge(0, 3);

    DfsPreprocessor preprocessor;
    const DfsInfo dfsInfo = preprocessor.run(graph);

    SeparatedDfsChildLists lists(dfsInfo);

    lists.removeChild(0, 2);

    const auto children = lists.toVector(0);

    BM_ASSERT(children.size() == 2);
    BM_ASSERT(lists.containsChild(1));
    BM_ASSERT(!lists.containsChild(2));
    BM_ASSERT(lists.containsChild(3));
}

BM_TEST(SeparatedDfsChildListsRemovesFrontChild) {
    Graph graph(3);
    graph.addEdge(0, 1);
    graph.addEdge(0, 2);

    DfsPreprocessor preprocessor;
    const DfsInfo dfsInfo = preprocessor.run(graph);

    SeparatedDfsChildLists lists(dfsInfo);

    const int firstChild = lists.frontChild(0);
    lists.removeChild(0, firstChild);

    BM_ASSERT(!lists.containsChild(firstChild));
    BM_ASSERT(lists.frontChild(0) != firstChild);
}

BM_TEST(SeparatedDfsChildListsRemovesOnlyChild) {
    Graph graph(2);
    graph.addEdge(0, 1);

    DfsPreprocessor preprocessor;
    const DfsInfo dfsInfo = preprocessor.run(graph);

    SeparatedDfsChildLists lists(dfsInfo);

    BM_ASSERT(!lists.empty(0));
    BM_ASSERT(lists.frontChild(0) == 1);

    lists.removeChild(0, 1);

    BM_ASSERT(lists.empty(0));
    BM_ASSERT(lists.frontChild(0) == -1);
    BM_ASSERT(!lists.containsChild(1));
}