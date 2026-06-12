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
    src/BmEmbeddingRecovery.cpp
    src/PlanarEmbeddingValidator.cpp
    src/KuratowskiCertificateVerifier.cpp
    src/BmRealExternalFaceTraversal.cpp
    src/BmKuratowskiExtractionContext.cpp
    src/BmKuratowskiFailureFactory.cpp
    src/BmKuratowskiPathMarker.cpp
    src/BmKuratowskiConnectionFinder.cpp
    src/BmKuratowskiMinorClassifier.cpp
    src/BmKuratowskiInternalPathAnalyzer.cpp
    src/BmKuratowskiIsolationPreparation.cpp
    src/BmKuratowskiExtractor.cpp
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
    tests/BmEmbeddingRecoveryTest.cpp
    tests/PlanarEmbeddingValidatorTest.cpp
    tests/KuratowskiCertificateVerifierTest.cpp
    tests/BmKuratowskiExtractionContextTest.cpp
    tests/BmKuratowskiFoundationTest.cpp
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

# Batch CLI used to validate recovered rotation systems with NetworkX.
add_executable(bm_planarity_embedding_cli
    tools/BmEmbeddingCli.cpp
)

target_link_libraries(bm_planarity_embedding_cli
    PRIVATE
        bm_planarity
)


# Batch CLI used to validate Kuratowski certificates.
add_executable(bm_kuratowski_verifier_cli
    tools/BmKuratowskiVerifierCli.cpp
)

target_link_libraries(bm_kuratowski_verifier_cli
    PRIVATE
        bm_planarity
)



# Batch CLI used to validate automatically extracted Kuratowski certificates.
add_executable(bm_kuratowski_certificate_cli
    tools/BmCertificateCli.cpp
)

target_link_libraries(bm_kuratowski_certificate_cli
    PRIVATE
        bm_planarity
)

# Batch CLI used to smoke-test Kuratowski minor classification.
add_executable(bm_kuratowski_classifier_cli
    tools/BmKuratowskiClassifierCli.cpp
)

target_link_libraries(bm_kuratowski_classifier_cli
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

    add_test(
        NAME bm_embedding_regression_quick
        COMMAND
            ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tools/embedding_regression.py
            --cli $<TARGET_FILE:bm_planarity_embedding_cli>
            --profile quick
    )

    set_tests_properties(
        bm_embedding_regression_quick
        PROPERTIES
            LABELS "embedding;differential"
            TIMEOUT 120
    )

    add_test(
        NAME bm_kuratowski_verifier_regression_quick
        COMMAND
            ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tools/kuratowski_verifier_regression.py
            --cli $<TARGET_FILE:bm_kuratowski_verifier_cli>
            --profile quick
    )

    set_tests_properties(
        bm_kuratowski_verifier_regression_quick
        PROPERTIES
            LABELS "kuratowski;differential"
            TIMEOUT 120
    )

    add_test(
        NAME bm_kuratowski_classifier_regression_quick
        COMMAND
            ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tools/kuratowski_classifier_regression.py
            --cli $<TARGET_FILE:bm_kuratowski_classifier_cli>
            --profile quick
    )

    set_tests_properties(
        bm_kuratowski_classifier_regression_quick
        PROPERTIES
            LABELS "kuratowski;differential"
            TIMEOUT 120
    )


    add_test(
        NAME bm_kuratowski_extractor_regression_quick
        COMMAND
            ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tools/kuratowski_extractor_regression.py
            --cli $<TARGET_FILE:bm_kuratowski_certificate_cli>
            --profile quick
    )

    set_tests_properties(
        bm_kuratowski_extractor_regression_quick
        PROPERTIES
            LABELS "kuratowski;differential"
            TIMEOUT 180
    )
endif()

# Optional JSON tools used by the UI/API layer.
option(
    BM_ENABLE_JSON_TOOLS
    "Build JSON input/output tools for UI and API integration"
    OFF
)

if(BM_ENABLE_JSON_TOOLS)
    find_package(nlohmann_json CONFIG QUIET)

    if(NOT nlohmann_json_FOUND)
        include(FetchContent)

        set(JSON_BuildTests OFF CACHE INTERNAL "")

        FetchContent_Declare(
            nlohmann_json
            URL
                https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz
            URL_HASH
                SHA256=42f6e95cad6ec532fd372391373363b62a14af6d771056dbfc86160e6dfff7aa
        )

        FetchContent_MakeAvailable(
            nlohmann_json
        )
    endif()

    add_library(bm_planarity_json
        src/io/GraphJsonAdapter.cpp
    )

    target_include_directories(
        bm_planarity_json
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/include
    )

    target_link_libraries(
        bm_planarity_json
        PUBLIC
            bm_planarity
            nlohmann_json::nlohmann_json
    )

    add_executable(
        bm_planarity_json_cli
        tools/BmJsonCli.cpp
    )

    target_link_libraries(
        bm_planarity_json_cli
        PRIVATE
            bm_planarity_json
    )
endif()

# Optional local REST API used by the web frontend.
option(
    BM_ENABLE_API_SERVER
    "Build local HTTP API server for UI integration"
    OFF
)

if(BM_ENABLE_API_SERVER)
    if(NOT BM_ENABLE_JSON_TOOLS)
        message(
            FATAL_ERROR
            "BM_ENABLE_API_SERVER requires BM_ENABLE_JSON_TOOLS=ON"
        )
    endif()

    include(FetchContent)

    set(
        HTTPLIB_INSTALL
        OFF
        CACHE BOOL
        ""
        FORCE
    )

    set(
        HTTPLIB_TEST
        OFF
        CACHE BOOL
        ""
        FORCE
    )

    # Za sada koristimo obični lokalni HTTP.
    # TLS može kasnije rješavati reverse proxy pri deploymentu.
    set(
        HTTPLIB_USE_OPENSSL_IF_AVAILABLE
        OFF
        CACHE BOOL
        ""
        FORCE
    )

    set(
        HTTPLIB_USE_ZLIB_IF_AVAILABLE
        OFF
        CACHE BOOL
        ""
        FORCE
    )

    set(
        HTTPLIB_USE_BROTLI_IF_AVAILABLE
        OFF
        CACHE BOOL
        ""
        FORCE
    )

    set(
        HTTPLIB_USE_ZSTD_IF_AVAILABLE
        OFF
        CACHE BOOL
        ""
        FORCE
    )

    FetchContent_Declare(
        httplib
        GIT_REPOSITORY
            https://github.com/yhirose/cpp-httplib.git
        GIT_TAG
            v0.47.0
        GIT_SHALLOW
            TRUE
    )

    FetchContent_MakeAvailable(
        httplib
    )

    add_executable(
        bm_planarity_api
        api/BmApiServer.cpp
    )

    target_link_libraries(
        bm_planarity_api
        PRIVATE
            bm_planarity_json
            httplib::httplib
    )
endif()