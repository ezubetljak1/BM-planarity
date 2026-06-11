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
    src/BmExternalFaceTraversal.cpp
    src/BmWalkup.cpp
    src/BmEmbeddingOperations.cpp
    src/BmWalkdown.cpp
    src/BmEmbeddingValidator.cpp
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
    tests/BmExternalFaceTraversalTest.cpp
    tests/BmWalkupTest.cpp
    tests/BmEmbeddingOperationsTest.cpp
    tests/BmWalkdownTest.cpp
    tests/BmEmbeddingValidatorTest.cpp
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
# Batch CLI used by differential regression and later benchmarking tools.
add_executable(bm_planarity_decision_cli
    tools/BmDecisionCli.cpp
)

target_link_libraries(bm_planarity_decision_cli
    PRIVATE
        bm_planarity
)

option(
    BM_ENABLE_DIFFERENTIAL_TESTS
    "Enable the optional NetworkX-based differential regression CTest"
    OFF
)

if(BM_ENABLE_DIFFERENTIAL_TESTS)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)

    add_test(
        NAME bm_differential_regression_quick
        COMMAND
            ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tools/differential_regression.py
            --cli $<TARGET_FILE:bm_planarity_decision_cli>
            --profile quick
    )

    set_tests_properties(
        bm_differential_regression_quick
        PROPERTIES
            LABELS "differential"
            TIMEOUT 120
    )
endif()
