#include "bm/BoyerMyrvoldPlanarity.hpp"
#include "bm/Graph.hpp"

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

volatile std::uint64_t benchmarkSink = 0;

struct Options {
    std::string profile = "quick";
    std::string outputPath;
    std::uint64_t seed = 19676;
    int repetitions = -1;
    int warmups = -1;
    std::vector<int> sizes;
    std::vector<std::string> families;
};

struct FamilyDefinition {
    std::string name;
    std::vector<std::pair<int, int>> baseEdges;
    int baseVertexCount = 0;
};

struct Scenario {
    const FamilyDefinition* family = nullptr;
    int requestedSize = 0;
    std::uint64_t seed = 0;
};

std::vector<std::string> splitCommaSeparated(const std::string& text) {
    std::vector<std::string> values;
    std::stringstream stream(text);
    std::string value;

    while (std::getline(stream, value, ',')) {
        if (value.empty()) {
            throw std::invalid_argument("Comma-separated lists cannot contain empty values.");
        }
        values.push_back(value);
    }

    if (values.empty()) {
        throw std::invalid_argument("Expected at least one comma-separated value.");
    }

    return values;
}

std::vector<int> parseSizes(const std::string& text) {
    std::vector<int> sizes;
    for (const std::string& value : splitCommaSeparated(text)) {
        const int size = std::stoi(value);
        if (size < 0) {
            throw std::invalid_argument("Graph sizes must be non-negative.");
        }
        sizes.push_back(size);
    }
    return sizes;
}

std::string requireArgument(int argc, char** argv, int& index, const char* option) {
    if (index + 1 >= argc) {
        throw std::invalid_argument(std::string("Missing value for ") + option + ".");
    }
    ++index;
    return argv[index];
}

Options parseOptions(int argc, char** argv) {
    Options options;

    for (int index = 1; index < argc; ++index) {
        const std::string argument = argv[index];

        if (argument == "--profile") {
            options.profile = requireArgument(argc, argv, index, "--profile");
        } else if (argument == "--output") {
            options.outputPath = requireArgument(argc, argv, index, "--output");
        } else if (argument == "--seed") {
            options.seed = std::stoull(requireArgument(argc, argv, index, "--seed"));
        } else if (argument == "--repetitions") {
            options.repetitions = std::stoi(requireArgument(argc, argv, index, "--repetitions"));
        } else if (argument == "--warmups") {
            options.warmups = std::stoi(requireArgument(argc, argv, index, "--warmups"));
        } else if (argument == "--families") {
            options.families = splitCommaSeparated(requireArgument(argc, argv, index, "--families"));
        } else if (argument == "--sizes") {
            options.sizes = parseSizes(requireArgument(argc, argv, index, "--sizes"));
        } else if (argument == "--help" || argument == "-h") {
            std::cout
                << "Usage: bm_planarity_phase_benchmark [options]\n"
                << "  --profile quick|full\n"
                << "  --output FILE.csv\n"
                << "  --seed INTEGER\n"
                << "  --repetitions INTEGER\n"
                << "  --warmups INTEGER\n"
                << "  --families subdivided_k33,subdivided_k5\n"
                << "  --sizes n1,n2,...\n";
            std::exit(0);
        } else {
            throw std::invalid_argument("Unknown option: " + argument);
        }
    }

    if (options.profile == "quick") {
        if (options.sizes.empty()) {
            options.sizes = {100, 1000, 10000};
        }
        if (options.repetitions < 0) {
            options.repetitions = 5;
        }
        if (options.warmups < 0) {
            options.warmups = 1;
        }
    } else if (options.profile == "full") {
        if (options.sizes.empty()) {
            options.sizes = {1000, 3000, 10000, 30000, 50000, 75000, 100000, 150000};
        }
        if (options.repetitions < 0) {
            options.repetitions = 15;
        }
        if (options.warmups < 0) {
            options.warmups = 3;
        }
    } else {
        throw std::invalid_argument("Unknown phase-benchmark profile: " + options.profile);
    }

    if (options.repetitions <= 0) {
        throw std::invalid_argument("Repetition count must be positive.");
    }
    if (options.warmups < 0) {
        throw std::invalid_argument("Warmup count cannot be negative.");
    }

    return options;
}

std::vector<FamilyDefinition> familyDefinitions() {
    return {
        {
            "subdivided_k33",
            {
                {0, 3}, {0, 4}, {0, 5},
                {1, 3}, {1, 4}, {1, 5},
                {2, 3}, {2, 4}, {2, 5}
            },
            6
        },
        {
            "subdivided_k5",
            {
                {0, 1}, {0, 2}, {0, 3}, {0, 4},
                {1, 2}, {1, 3}, {1, 4},
                {2, 3}, {2, 4},
                {3, 4}
            },
            5
        }
    };
}

std::vector<FamilyDefinition> selectFamilies(const Options& options) {
    const std::vector<FamilyDefinition> available = familyDefinitions();
    if (options.families.empty()) {
        return available;
    }

    std::vector<FamilyDefinition> selected;
    for (const std::string& requestedName : options.families) {
        const auto iterator = std::find_if(
            available.begin(),
            available.end(),
            [&](const FamilyDefinition& definition) {
                return definition.name == requestedName;
            }
        );

        if (iterator == available.end()) {
            throw std::invalid_argument("Unknown phase-benchmark family: " + requestedName);
        }
        selected.push_back(*iterator);
    }
    return selected;
}

bm::Graph makeSubdividedGraph(int vertexCount, const FamilyDefinition& family) {
    const int actualVertexCount = std::max(vertexCount, family.baseVertexCount);
    bm::Graph graph(actualVertexCount);

    const int extraVertices = actualVertexCount - family.baseVertexCount;
    const int quotient = extraVertices / static_cast<int>(family.baseEdges.size());
    const int remainder = extraVertices % static_cast<int>(family.baseEdges.size());

    int nextVertex = family.baseVertexCount;

    for (std::size_t index = 0; index < family.baseEdges.size(); ++index) {
        const auto [source, target] = family.baseEdges[index];
        const int subdivisions = quotient + (static_cast<int>(index) < remainder ? 1 : 0);

        int previous = source;
        for (int subdivision = 0; subdivision < subdivisions; ++subdivision) {
            graph.addEdge(previous, nextVertex);
            previous = nextVertex;
            ++nextVertex;
        }
        graph.addEdge(previous, target);
    }

    return graph;
}

std::vector<Scenario> createScenarios(
    const Options& options,
    const std::vector<FamilyDefinition>& families
) {
    std::vector<Scenario> scenarios;
    for (const FamilyDefinition& family : families) {
        for (int size : options.sizes) {
            scenarios.push_back({&family, size, options.seed});
        }
    }

    std::mt19937_64 rng(options.seed);
    std::shuffle(scenarios.begin(), scenarios.end(), rng);
    return scenarios;
}

void consumeResult(const bm::BmProfiledPlanarityResult& profiled) {
    std::uint64_t value = profiled.result.planar ? 1ULL : 2ULL;
    if (profiled.result.certificate.has_value()) {
        value += static_cast<std::uint64_t>(profiled.result.certificate->edgeIds.size());
    }
    value += static_cast<std::uint64_t>(profiled.timings.totalNs & 1LL);
    benchmarkSink = benchmarkSink ^ value;
}

void writeHeader(std::ostream& output) {
    output
        << "scenario_index,family,requested_n,n,m,work_size,repetition,seed,actual_planarity,"
        << "total_ns,validation_ns,dense_shortcut_overhead_ns,dfs_preprocessing_ns,"
        << "state_initialization_ns,decision_core_ns,failure_factory_ns,"
        << "kuratowski_preparation_ns,kuratowski_oriented_state_copy_ns,kuratowski_orientation_ns,"
        << "kuratowski_context_initialization_ns,kuratowski_minor_classification_ns,"
        << "kuratowski_classify_initial_ns,kuratowski_classify_external_face_vertices_ns,"
        << "kuratowski_find_highest_xy_path_ns,kuratowski_find_z_to_root_path_ns,"
        << "kuratowski_find_future_pertinent_below_xy_path_ns,"
        << "kuratowski_isolation_ns,certificate_verification_ns,"
        << "embedding_recovery_ns,accounted_ns,unaccounted_ns,ns_per_work_item\n";
}

void writeMeasurement(
    std::ostream& output,
    int scenarioIndex,
    const Scenario& scenario,
    const bm::Graph& graph,
    int repetition,
    const bm::BmProfiledPlanarityResult& profiled
) {
    const bm::BmPlanarityPhaseTimings& timings = profiled.timings;
    const long long workSize = static_cast<long long>(graph.vertexCount()) + graph.edgeCount();
    const std::int64_t accounted = timings.accountedNs();
    const std::int64_t unaccounted = timings.totalNs - accounted;
    const double normalized = workSize > 0
        ? static_cast<double>(timings.totalNs) / static_cast<double>(workSize)
        : 0.0;

    output
        << scenarioIndex << ','
        << scenario.family->name << ','
        << scenario.requestedSize << ','
        << graph.vertexCount() << ','
        << graph.edgeCount() << ','
        << workSize << ','
        << repetition << ','
        << scenario.seed << ','
        << (profiled.result.planar ? "PLANAR" : "NONPLANAR") << ','
        << timings.totalNs << ','
        << timings.validationNs << ','
        << timings.denseShortcutOverheadNs << ','
        << timings.dfsPreprocessingNs << ','
        << timings.stateInitializationNs << ','
        << timings.decisionCoreNs << ','
        << timings.failureFactoryNs << ','
        << timings.kuratowskiPreparationNs << ','
        << timings.kuratowskiOrientedStateCopyNs << ','
        << timings.kuratowskiOrientationNs << ','
        << timings.kuratowskiContextInitializationNs << ','
        << timings.kuratowskiMinorClassificationNs << ','
        << timings.kuratowskiClassifyInitialNs << ','
        << timings.kuratowskiClassifyExternalFaceVerticesNs << ','
        << timings.kuratowskiFindHighestXyPathNs << ','
        << timings.kuratowskiFindZToRootPathNs << ','
        << timings.kuratowskiFindFuturePertinentBelowXyPathNs << ','
        << timings.kuratowskiIsolationNs << ','
        << timings.certificateVerificationNs << ','
        << timings.embeddingRecoveryNs << ','
        << accounted << ','
        << unaccounted << ','
        << std::setprecision(17) << normalized
        << '\n';
}

void runBenchmark(const Options& options, std::ostream& output) {
    const std::vector<FamilyDefinition> families = selectFamilies(options);
    const std::vector<Scenario> scenarios = createScenarios(options, families);

    bm::BoyerMyrvoldPlanarity algorithm;
    writeHeader(output);

    for (std::size_t scenarioIndex = 0; scenarioIndex < scenarios.size(); ++scenarioIndex) {
        const Scenario& scenario = scenarios[scenarioIndex];
        const bm::Graph graph = makeSubdividedGraph(scenario.requestedSize, *scenario.family);

        for (int warmup = 0; warmup < options.warmups; ++warmup) {
            const bm::BmProfiledPlanarityResult result = algorithm.runProfiled(graph);
            consumeResult(result);
            if (result.result.planar) {
                throw std::logic_error("Subdivision obstruction unexpectedly classified as planar during warmup.");
            }
        }

        for (int repetition = 0; repetition < options.repetitions; ++repetition) {
            const bm::BmProfiledPlanarityResult result = algorithm.runProfiled(graph);
            consumeResult(result);

            if (result.result.planar) {
                throw std::logic_error("Subdivision obstruction unexpectedly classified as planar.");
            }

            writeMeasurement(
                output,
                static_cast<int>(scenarioIndex),
                scenario,
                graph,
                repetition,
                result
            );
        }

        std::cerr
            << "Completed " << (scenarioIndex + 1) << '/' << scenarios.size()
            << ": " << scenario.family->name
            << " n=" << graph.vertexCount()
            << " m=" << graph.edgeCount()
            << '\n';
    }

    std::cerr << "Benchmark sink: " << benchmarkSink << '\n';
}

} // namespace

int main(int argc, char** argv) {
    try {
        const Options options = parseOptions(argc, argv);

        if (options.outputPath.empty()) {
            runBenchmark(options, std::cout);
            return 0;
        }

        std::ofstream output(options.outputPath);
        if (!output) {
            throw std::runtime_error("Cannot open phase-benchmark output file: " + options.outputPath);
        }

        runBenchmark(options, output);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Phase benchmark failed: " << error.what() << '\n';
        return 1;
    }
}
