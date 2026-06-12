#include "bm/BoyerMyrvoldPlanarity.hpp"
#include "bm/Graph.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <limits>
#include <numeric>
#include <optional>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

namespace {

using Clock = std::chrono::steady_clock;

volatile std::uint64_t benchmarkSink = 0;

enum class ExpectedPlanarity {
    Planar,
    NonPlanar,
    Unknown
};

struct Options {
    std::string profile = "quick";
    std::string outputPath;
    std::uint64_t seed = 19676;
    int repetitions = -1;
    int warmups = -1;
    int randomInstances = -1;
    std::vector<int> sparseSizes;
    std::vector<int> denseSizes;
    std::vector<std::string> families;
};

struct FamilyDefinition {
    std::string name;
    bool usesDenseSizes = false;
    bool randomized = false;
    ExpectedPlanarity expected = ExpectedPlanarity::Unknown;
    std::function<bm::Graph(int, std::uint64_t)> generate;
};

struct Scenario {
    const FamilyDefinition* family = nullptr;
    int requestedSize = 0;
    int instance = 0;
    std::uint64_t seed = 0;
};

std::uint64_t stableHash(std::string_view text) {
    std::uint64_t result = 1469598103934665603ULL;
    for (unsigned char character : text) {
        result ^= character;
        result *= 1099511628211ULL;
    }
    return result;
}

std::uint64_t scenarioSeed(
    std::uint64_t baseSeed,
    std::string_view family,
    int requestedSize,
    int instance
) {
    std::uint64_t value = baseSeed;
    value ^= stableHash(family) + 0x9e3779b97f4a7c15ULL + (value << 6U) + (value >> 2U);
    value ^= static_cast<std::uint64_t>(requestedSize) + 0x9e3779b97f4a7c15ULL + (value << 6U) + (value >> 2U);
    value ^= static_cast<std::uint64_t>(instance) + 0x9e3779b97f4a7c15ULL + (value << 6U) + (value >> 2U);
    return value;
}

std::string expectedToString(ExpectedPlanarity expected) {
    switch (expected) {
    case ExpectedPlanarity::Planar:
        return "PLANAR";
    case ExpectedPlanarity::NonPlanar:
        return "NONPLANAR";
    case ExpectedPlanarity::Unknown:
        return "UNKNOWN";
    }

    throw std::logic_error("Unknown expected-planarity enum value.");
}

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
        } else if (argument == "--random-instances") {
            options.randomInstances = std::stoi(requireArgument(argc, argv, index, "--random-instances"));
        } else if (argument == "--families") {
            options.families = splitCommaSeparated(requireArgument(argc, argv, index, "--families"));
        } else if (argument == "--sparse-sizes") {
            options.sparseSizes = parseSizes(requireArgument(argc, argv, index, "--sparse-sizes"));
        } else if (argument == "--dense-sizes") {
            options.denseSizes = parseSizes(requireArgument(argc, argv, index, "--dense-sizes"));
        } else if (argument == "--help" || argument == "-h") {
            std::cout
                << "Usage: bm_planarity_benchmark [options]\n"
                << "  --profile quick|full\n"
                << "  --output FILE.csv\n"
                << "  --seed INTEGER\n"
                << "  --repetitions INTEGER\n"
                << "  --warmups INTEGER\n"
                << "  --random-instances INTEGER\n"
                << "  --families name1,name2,...\n"
                << "  --sparse-sizes n1,n2,...\n"
                << "  --dense-sizes n1,n2,...\n";
            std::exit(0);
        } else {
            throw std::invalid_argument("Unknown option: " + argument);
        }
    }

    if (options.profile == "quick") {
        if (options.sparseSizes.empty()) {
            options.sparseSizes = {10, 100, 1000, 5000};
        }
        if (options.denseSizes.empty()) {
            options.denseSizes = {10, 30, 60, 100};
        }
        if (options.repetitions < 0) {
            options.repetitions = 5;
        }
        if (options.warmups < 0) {
            options.warmups = 1;
        }
        if (options.randomInstances < 0) {
            options.randomInstances = 2;
        }
    } else if (options.profile == "full") {
        if (options.sparseSizes.empty()) {
            options.sparseSizes = {100, 300, 1000, 3000, 10000, 30000, 100000};
        }
        if (options.denseSizes.empty()) {
            options.denseSizes = {20, 50, 100, 200, 400, 800};
        }
        if (options.repetitions < 0) {
            options.repetitions = 11;
        }
        if (options.warmups < 0) {
            options.warmups = 2;
        }
        if (options.randomInstances < 0) {
            options.randomInstances = 5;
        }
    } else {
        throw std::invalid_argument("Unknown benchmark profile: " + options.profile);
    }

    if (options.repetitions <= 0) {
        throw std::invalid_argument("Repetition count must be positive.");
    }
    if (options.warmups < 0) {
        throw std::invalid_argument("Warmup count cannot be negative.");
    }
    if (options.randomInstances <= 0) {
        throw std::invalid_argument("Random-instance count must be positive.");
    }

    return options;
}

bm::Graph makePath(int vertexCount, std::uint64_t) {
    bm::Graph graph(vertexCount);
    for (int vertex = 1; vertex < vertexCount; ++vertex) {
        graph.addEdge(vertex - 1, vertex);
    }
    return graph;
}

bm::Graph makeCycle(int vertexCount, std::uint64_t) {
    bm::Graph graph(vertexCount);
    if (vertexCount <= 1) {
        return graph;
    }
    if (vertexCount == 2) {
        graph.addEdge(0, 1);
        return graph;
    }

    for (int vertex = 0; vertex < vertexCount; ++vertex) {
        graph.addEdge(vertex, (vertex + 1) % vertexCount);
    }
    return graph;
}

bm::Graph makeWheel(int vertexCount, std::uint64_t) {
    if (vertexCount < 4) {
        return makeCycle(vertexCount, 0);
    }

    bm::Graph graph(vertexCount);
    for (int rim = 1; rim < vertexCount; ++rim) {
        const int nextRim = rim + 1 == vertexCount ? 1 : rim + 1;
        graph.addEdge(rim, nextRim);
        graph.addEdge(0, rim);
    }
    return graph;
}

bm::Graph makeGrid(int vertexCount, std::uint64_t) {
    bm::Graph graph(vertexCount);
    if (vertexCount <= 1) {
        return graph;
    }

    const int columns = std::max(1, static_cast<int>(std::sqrt(static_cast<double>(vertexCount))));

    for (int vertex = 0; vertex < vertexCount; ++vertex) {
        const int row = vertex / columns;
        const int column = vertex % columns;

        if (column > 0) {
            graph.addEdge(vertex - 1, vertex);
        }

        if (row > 0) {
            const int above = vertex - columns;
            if (above >= 0) {
                graph.addEdge(above, vertex);
            }
        }
    }

    return graph;
}

struct Face {
    int a = -1;
    int b = -1;
    int c = -1;
};

bm::Graph makeStackedTriangulation(int vertexCount, std::uint64_t seed) {
    if (vertexCount < 3) {
        return makePath(vertexCount, seed);
    }

    bm::Graph graph(vertexCount);
    graph.addEdge(0, 1);
    graph.addEdge(1, 2);
    graph.addEdge(2, 0);

    std::vector<Face> faces = {{0, 1, 2}};
    std::mt19937_64 rng(seed);

    for (int vertex = 3; vertex < vertexCount; ++vertex) {
        std::uniform_int_distribution<std::size_t> distribution(0, faces.size() - 1);
        const std::size_t index = distribution(rng);
        const Face face = faces[index];
        faces[index] = faces.back();
        faces.pop_back();

        graph.addEdge(vertex, face.a);
        graph.addEdge(vertex, face.b);
        graph.addEdge(vertex, face.c);

        faces.push_back({face.a, face.b, vertex});
        faces.push_back({face.b, face.c, vertex});
        faces.push_back({face.c, face.a, vertex});
    }

    return graph;
}

bm::Graph makeRandomTree(int vertexCount, std::uint64_t seed) {
    bm::Graph graph(vertexCount);
    std::mt19937_64 rng(seed);

    for (int vertex = 1; vertex < vertexCount; ++vertex) {
        std::uniform_int_distribution<int> distribution(0, vertex - 1);
        graph.addEdge(vertex, distribution(rng));
    }

    return graph;
}

std::uint64_t edgeKey(int u, int v) {
    if (u > v) {
        std::swap(u, v);
    }

    return (static_cast<std::uint64_t>(static_cast<std::uint32_t>(u)) << 32U)
         | static_cast<std::uint32_t>(v);
}

bm::Graph makeRandomSparse(int vertexCount, std::uint64_t seed) {
    bm::Graph graph(vertexCount);
    if (vertexCount < 2) {
        return graph;
    }

    const long long maximumEdgeCount =
        static_cast<long long>(vertexCount) * (vertexCount - 1LL) / 2LL;
    const int targetEdgeCount = static_cast<int>(
        std::min(maximumEdgeCount, std::max(0LL, 3LL * vertexCount - 6LL))
    );

    std::unordered_set<std::uint64_t> usedEdges;
    usedEdges.reserve(static_cast<std::size_t>(targetEdgeCount * 2 + 1));

    std::mt19937_64 rng(seed);
    std::uniform_int_distribution<int> vertexDistribution(0, vertexCount - 1);

    while (graph.edgeCount() < targetEdgeCount) {
        const int u = vertexDistribution(rng);
        const int v = vertexDistribution(rng);
        if (u == v) {
            continue;
        }

        const std::uint64_t key = edgeKey(u, v);
        if (!usedEdges.insert(key).second) {
            continue;
        }

        graph.addEdge(u, v);
    }

    return graph;
}

bm::Graph makeSubdividedGraph(
    int vertexCount,
    const std::vector<std::pair<int, int>>& baseEdges,
    int baseVertexCount
) {
    if (vertexCount < baseVertexCount) {
        throw std::invalid_argument("Requested subdivided graph is smaller than its base graph.");
    }

    bm::Graph graph(vertexCount);
    const int extraVertices = vertexCount - baseVertexCount;
    const int quotient = extraVertices / static_cast<int>(baseEdges.size());
    const int remainder = extraVertices % static_cast<int>(baseEdges.size());

    int nextVertex = baseVertexCount;

    for (std::size_t index = 0; index < baseEdges.size(); ++index) {
        const auto [source, target] = baseEdges[index];
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

bm::Graph makeSubdividedK33(int vertexCount, std::uint64_t) {
    static const std::vector<std::pair<int, int>> baseEdges = {
        {0, 3}, {0, 4}, {0, 5},
        {1, 3}, {1, 4}, {1, 5},
        {2, 3}, {2, 4}, {2, 5}
    };

    return makeSubdividedGraph(std::max(vertexCount, 6), baseEdges, 6);
}

bm::Graph makeSubdividedK5(int vertexCount, std::uint64_t) {
    static const std::vector<std::pair<int, int>> baseEdges = {
        {0, 1}, {0, 2}, {0, 3}, {0, 4},
        {1, 2}, {1, 3}, {1, 4},
        {2, 3}, {2, 4},
        {3, 4}
    };

    return makeSubdividedGraph(std::max(vertexCount, 5), baseEdges, 5);
}

bm::Graph makeComplete(int vertexCount, std::uint64_t) {
    bm::Graph graph(vertexCount);
    for (int u = 0; u < vertexCount; ++u) {
        for (int v = u + 1; v < vertexCount; ++v) {
            graph.addEdge(u, v);
        }
    }
    return graph;
}

std::vector<FamilyDefinition> familyDefinitions() {
    return {
        {"path", false, false, ExpectedPlanarity::Planar, makePath},
        {"cycle", false, false, ExpectedPlanarity::Planar, makeCycle},
        {"wheel", false, false, ExpectedPlanarity::Planar, makeWheel},
        {"grid", false, false, ExpectedPlanarity::Planar, makeGrid},
        {"stacked_triangulation", false, true, ExpectedPlanarity::Planar, makeStackedTriangulation},
        {"random_tree", false, true, ExpectedPlanarity::Planar, makeRandomTree},
        {"random_sparse", false, true, ExpectedPlanarity::Unknown, makeRandomSparse},
        {"subdivided_k33", false, false, ExpectedPlanarity::NonPlanar, makeSubdividedK33},
        {"subdivided_k5", false, false, ExpectedPlanarity::NonPlanar, makeSubdividedK5},
        {"complete", true, false, ExpectedPlanarity::Unknown, makeComplete}
    };
}

std::vector<FamilyDefinition> selectFamilies(const Options& options) {
    std::vector<FamilyDefinition> available = familyDefinitions();
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
            throw std::invalid_argument("Unknown benchmark family: " + requestedName);
        }
        selected.push_back(*iterator);
    }

    return selected;
}

bool expectedMatches(ExpectedPlanarity expected, bool actual) {
    return expected == ExpectedPlanarity::Unknown
        || (expected == ExpectedPlanarity::Planar && actual)
        || (expected == ExpectedPlanarity::NonPlanar && !actual);
}

void consumeResult(const bm::PlanarityResult& result) {
    std::uint64_t value = result.planar ? 1ULL : 2ULL;
    if (result.embedding.has_value()) {
        value += static_cast<std::uint64_t>(result.embedding->clockwiseEdgesAroundVertex.size());
    }
    if (result.certificate.has_value()) {
        value += static_cast<std::uint64_t>(result.certificate->edgeIds.size());
    }
    benchmarkSink = benchmarkSink ^ value;
}

void writeHeader(std::ostream& output) {
    output
        << "scenario_index,family,instance,requested_n,n,m,work_size,expected_planarity,actual_planarity,"
        << "repetition,elapsed_ns,ns_per_work_item,seed\n";
}

void writeMeasurement(
    std::ostream& output,
    int scenarioIndex,
    const Scenario& scenario,
    const bm::Graph& graph,
    bool actualPlanar,
    int repetition,
    std::int64_t elapsedNanoseconds
) {
    const long long workSize = static_cast<long long>(graph.vertexCount()) + graph.edgeCount();
    const double normalized = workSize > 0
        ? static_cast<double>(elapsedNanoseconds) / static_cast<double>(workSize)
        : 0.0;

    output
        << scenarioIndex << ','
        << scenario.family->name << ','
        << scenario.instance << ','
        << scenario.requestedSize << ','
        << graph.vertexCount() << ','
        << graph.edgeCount() << ','
        << workSize << ','
        << expectedToString(scenario.family->expected) << ','
        << (actualPlanar ? "PLANAR" : "NONPLANAR") << ','
        << repetition << ','
        << elapsedNanoseconds << ','
        << std::setprecision(17) << normalized << ','
        << scenario.seed << '\n';
}

std::vector<Scenario> createScenarios(
    const Options& options,
    const std::vector<FamilyDefinition>& families
) {
    std::vector<Scenario> scenarios;

    for (const FamilyDefinition& family : families) {
        const std::vector<int>& sizes = family.usesDenseSizes
            ? options.denseSizes
            : options.sparseSizes;

        const int instanceCount = family.randomized ? options.randomInstances : 1;

        for (int size : sizes) {
            for (int instance = 0; instance < instanceCount; ++instance) {
                scenarios.push_back({
                    &family,
                    size,
                    instance,
                    scenarioSeed(options.seed, family.name, size, instance)
                });
            }
        }
    }

    std::mt19937_64 rng(options.seed);
    std::shuffle(scenarios.begin(), scenarios.end(), rng);
    return scenarios;
}

void runBenchmark(const Options& options, std::ostream& output) {
    const std::vector<FamilyDefinition> families = selectFamilies(options);
    const std::vector<Scenario> scenarios = createScenarios(options, families);

    bm::BoyerMyrvoldPlanarity algorithm;
    writeHeader(output);

    for (std::size_t scenarioIndex = 0; scenarioIndex < scenarios.size(); ++scenarioIndex) {
        const Scenario& scenario = scenarios[scenarioIndex];
        const bm::Graph graph = scenario.family->generate(scenario.requestedSize, scenario.seed);

        std::optional<bool> expectedActualPlanarity;

        for (int warmup = 0; warmup < options.warmups; ++warmup) {
            const bm::PlanarityResult result = algorithm.run(graph);
            consumeResult(result);
            if (!expectedMatches(scenario.family->expected, result.planar)) {
                throw std::logic_error("Known-planarity family produced an unexpected decision during warmup: " + scenario.family->name);
            }
            expectedActualPlanarity = result.planar;
        }

        for (int repetition = 0; repetition < options.repetitions; ++repetition) {
            const auto started = Clock::now();
            const bm::PlanarityResult result = algorithm.run(graph);
            const auto finished = Clock::now();
            consumeResult(result);

            if (!expectedMatches(scenario.family->expected, result.planar)) {
                throw std::logic_error("Known-planarity family produced an unexpected decision: " + scenario.family->name);
            }
            if (expectedActualPlanarity.has_value() && result.planar != *expectedActualPlanarity) {
                throw std::logic_error("Algorithm decision changed between repetitions.");
            }
            expectedActualPlanarity = result.planar;

            const auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(finished - started).count();
            writeMeasurement(
                output,
                static_cast<int>(scenarioIndex),
                scenario,
                graph,
                result.planar,
                repetition,
                elapsed
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
            throw std::runtime_error("Cannot open benchmark output file: " + options.outputPath);
        }

        runBenchmark(options, output);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Benchmark failed: " << error.what() << '\n';
        return 1;
    }
}
