#include "httplib.h"
#include "json.hpp"

#include <cstdlib>      // std::getenv
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

// Returns Docker endpoint from DOCKER_ENDPOINT env var, or defaults to Unix socket
std::string get_docker_endpoint() {
    if (const char* ep = std::getenv("DOCKER_ENDPOINT")) {
        return {ep};
    }
    // Default to the Unix socket if not specified
    return "unix:///var/run/docker.sock";
}

// Retrieve list of Docker containers
std::string get_docker_containers() {
    httplib::Client docker_client(get_docker_endpoint());
    docker_client.set_read_timeout(5, 0);

    if (auto res = docker_client.Get("/containers/json"); res && res->status == 200) {
        return res->body;
    }
    return R"({"error":"Failed to retrieve containers"})";
}

// Retrieve stats for a list of container IDs
std::string get_container_stats(const std::vector<std::string>& ids) {
    httplib::Client docker_client(get_docker_endpoint());
    docker_client.set_read_timeout(5, 0);

    nlohmann::json result;
    result["containers"] = nlohmann::json::array();

    for (const auto &id : ids) {
        std::string stats_endpoint = "/containers/" + id + "/stats?stream=false";
        auto stats_res = docker_client.Get(stats_endpoint);

        nlohmann::json container_stats;
        container_stats["Id"] = id;
        if (stats_res && stats_res->status == 200) {
            try {
                container_stats["stats"] = nlohmann::json::parse(stats_res->body);
            } catch (const std::exception &e) {
                container_stats["error"] = std::string("JSON parsing error: ") + e.what();
            }
        } else {
            container_stats["error"] = "Failed to retrieve stats";
        }
        result["containers"].push_back(container_stats);
    }
    return result.dump();
}

int main() {
    httplib::Server svr;

    // Endpoint to list all Docker containers
    svr.Get("/containers", [](const httplib::Request&, httplib::Response &res) {
        const std::string containers = get_docker_containers();
        res.set_content(containers, "application/json");
    });

    // Endpoint to retrieve stats for specific containers (comma-separated ID list)
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

    std::cout << "Server listening on 0.0.0.0:8080\n";
    svr.listen("0.0.0.0", 8080);
    return 0;
}
