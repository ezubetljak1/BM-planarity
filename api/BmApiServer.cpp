#include "bm/BoyerMyrvoldPlanarity.hpp"
#include "bm/io/GraphJsonAdapter.hpp"

#include <httplib.h>
#include <nlohmann/json.hpp>

#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

using Json = nlohmann::json;

void addCorsHeaders(httplib::Response& response) {
    // za lokalni razvoj React frontend vjv ce raditi na drugom portu
    // kasnije ograniciti na tacan FE origin
    response.set_header("Access-Control-Allow-Origin", "*");

    response.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");

    response.set_header("Access-Control-Allow-Headers", "Content-Type");
}

void setJsonResponse(httplib::Response& response, int status, const Json& body) {
    addCorsHeaders(response);

    response.status = status;

    response.set_content(body.dump(2), "application/json");
}

void setErrorResponse(httplib::Response& response, int status, const std::string& message) {
    setJsonResponse(response, status, 
    {
        {"schemaVersion", 1},
        {"ok", false},
        {
            "error",
            {
                {"message", message}
            }
        }
    });
}

} // namespace

int main() {
    httplib::Server server;

    server.Get("/api/health", [](const httplib::Request&, httplib::Response& response){
        setJsonResponse(response, 200,
        {
            {"schemaVersion", 1},
            {"ok", true},
            {"service", "bm-planarity-api"}
        });
    });
    
    server.Options("/api/planarity/analyze", [](const httplib::Request&, httplib::Response& response) {
        addCorsHeaders(response);
        response.status = 204;
    });

    server.Post("/api/planarity/analyze", [](const httplib::Request& request, httplib::Response& response){
        try {
            const Json input = Json::parse(request.body);

            bm::io::ParsedJsonGraph parsedGraph = bm::io::GraphJsonAdapter::fromJson(input);

            bm::BoyerMyrvoldPlanarity algorithm;

            const bm::PlanarityResult result = algorithm.run(parsedGraph.graph);

            setJsonResponse(response, 200, bm::io::GraphJsonAdapter::toJson(parsedGraph, result));
        } catch (const nlohmann::json::parse_error& error) {
            setErrorResponse(response, 400, std::string("Invalid JSON: ") + error.what());
        } catch (const std::invalid_argument& error) {
            setErrorResponse(response, 400, error.what());
        } catch (const std::out_of_range& error) {
            setErrorResponse(response, 400, error.what());
        } catch (const std::exception& error) {
            setErrorResponse(response, 500, std::string("Internal server error: ") + error.what());
        }
    });

    std::cout << "BM planarity API listening on http://127.0.0.1:8080\n";
    
    if (!server.listen("127.0.0.1", 8080)) {
        std::cerr << "Failed to start API server.\n";

        return 1;
    }

    return 0;
}