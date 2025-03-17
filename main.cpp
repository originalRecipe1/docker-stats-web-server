#include "httplib.h"
#include "json.hpp"

#include <cstdlib>   // std::getenv
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

// Utility: split a string by delimiter
std::vector<std::string> split(const std::string &s, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream stream(s);
    while (std::getline(stream, token, delimiter)) {
        if (!token.empty()) {
            tokens.push_back(token);
        }
    }
    return tokens;
}

// Returns Docker endpoint from DOCKER_ENDPOINT env var, or defaults to "unix:///var/run/docker.sock"
std::string get_docker_endpoint() {
    if (const char* ep = std::getenv("DOCKER_ENDPOINT")) {
        return {ep};
    }
    return "unix:///var/run/docker.sock";
}

// Build a single, static Docker client once. We'll reuse it for all requests.
httplib::Client& get_docker_client() {
    // This static function local variable is constructed on first call
    // and never reconstructed again.
    static httplib::Client docker_client([](){
        // Figure out the endpoint
        std::string ep = get_docker_endpoint();

        httplib::Client client{""};

        // If the endpoint starts with "unix://", strip that part and set AF_UNIX
        const std::string prefix = "unix://";
        if (ep.rfind(prefix, 0) == 0) {
            ep = ep.substr(prefix.size()); // remove "unix://"
            client = httplib::Client(ep.c_str());
            client.set_address_family(AF_UNIX);
        } else {
            // e.g., "tcp://", "http://", etc.
            client = httplib::Client(ep.c_str());
        }

        // Set a short read timeout (5 seconds)
        client.set_read_timeout(5, 0);

        // Override the "Host" header so Docker doesn't complain about
        // "400 Bad Request: malformed Host header".
        client.set_default_headers({{"Host", "localhost"}});

        return client;
    }());

    return docker_client;
}

// Retrieve list of Docker containers with extra error details
std::string get_docker_containers() {
    auto &docker_client = get_docker_client();
    auto res = docker_client.Get("/containers/json");

    if (!res) {
        return R"({"error":"No response from Docker daemon"})";
    }
    if (res->status == 200) {
        return res->body; // Success
    }

    nlohmann::json j;
    j["error"] = "Failed to retrieve containers";
    j["status_code"] = res->status;
    j["docker_error"] = res->body;
    return j.dump();
}

// Retrieve stats for a list of container IDs with extra error details
std::string get_container_stats(const std::vector<std::string>& ids) {
    auto &docker_client = get_docker_client();

    nlohmann::json result;
    result["containers"] = nlohmann::json::array();

    for (const auto &id : ids) {
        std::string stats_endpoint = "/containers/" + id + "/stats?stream=false";
        auto stats_res = docker_client.Get(stats_endpoint.c_str());

        nlohmann::json container_stats;
        container_stats["Id"] = id;

        if (!stats_res) {
            container_stats["error"] = "No response from Docker daemon";
        } else if (stats_res->status == 200) {
            try {
                container_stats["stats"] = nlohmann::json::parse(stats_res->body);
            } catch (const std::exception &e) {
                container_stats["error"] = std::string("JSON parsing error: ") + e.what();
            }
        } else {
            container_stats["error"] = "Failed to retrieve stats";
            container_stats["status_code"] = stats_res->status;
            container_stats["docker_error"] = stats_res->body;
        }

        result["containers"].push_back(container_stats);
    }
    return result.dump();
}

int main() {
    // Print the Docker endpoint once at startup
    const std::string endpoint = get_docker_endpoint();
    std::cerr << "Using Docker endpoint: " << endpoint << std::endl;

    httplib::Server svr;

    // Endpoint: List all Docker containers
    svr.Get("/containers", [](const httplib::Request&, httplib::Response &res) {
        const std::string containers = get_docker_containers();
        res.set_content(containers, "application/json");
    });

    // Endpoint: Retrieve stats for specific containers (comma-separated ID list)
    svr.Get("/container_stats", [](const httplib::Request &req, httplib::Response &res) {
        if (!req.has_param("ids")) {
            res.status = 400;
            res.set_content(R"({"error":"Query param 'ids' is required"})", "application/json");
            return;
        }
        const std::string ids_param = req.get_param_value("ids");
        const std::vector<std::string> ids = split(ids_param, ',');
        if (ids.empty()) {
            res.status = 400;
            res.set_content(R"({"error":"No container IDs provided"})", "application/json");
            return;
        }
        const std::string stats = get_container_stats(ids);
        res.set_content(stats, "application/json");
    });

    svr.set_error_handler([](const httplib::Request &req, httplib::Response &res) {
        nlohmann::json error_response;
        error_response["error"] = "Invalid endpoint";
        error_response["message"] = "This endpoint does not exist. Please see API docs.";
        error_response["requested_path"] = req.path;
        res.status = 400;
        res.set_content(error_response.dump(), "application/json");
    });

    std::cout << "Server listening on 0.0.0.0:8080\n";
    svr.listen("0.0.0.0", 8080);
    return 0;
}
