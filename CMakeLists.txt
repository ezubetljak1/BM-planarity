cmake_minimum_required(VERSION 3.20)

project(bm_planarity LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Core library
add_library(bm_planarity
    src/Graph.cpp
    src/DfsPreprocessor.cpp
    src/BmEmbeddingState.cpp
    src/SeparatedDfsChildLists.cpp
    src/SimpleGraphValidator.cpp
    src/BoyerMyrvoldPlanarity.cpp
    src/BmPartialEmbedding.cpp
)

target_include_directories(bm_planarity
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Tests executable
add_executable(bm_planarity_tests
    tests/TestMain.cpp
    tests/GraphTest.cpp
    tests/DfsPreprocessorTest.cpp
    tests/BmEmbeddingStateTest.cpp
    tests/SeparatedDfsChildListsTest.cpp
    tests/SimpleGraphValidatorTest.cpp
    tests/BoyerMyrvoldPlanarityTest.cpp
    tests/BmPartialEmbeddingTest.cpp
)

target_include_directories(bm_planarity_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/tests
)

target_link_libraries(bm_planarity_tests
    PRIVATE
        bm_planarity
)

enable_testing()

add_test(
    NAME bm_planarity_tests
    COMMAND bm_planarity_tests
)