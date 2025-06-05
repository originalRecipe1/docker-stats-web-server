#include "httplib.h"
#include "json.hpp"
#include "split.hpp"

#include <cstdlib>
#include <iostream>
#include <csignal>
#include <sstream>
#include <string>
#include <vector>

static httplib::Server* g_server_ptr = nullptr;

void handle_signal(const int signal) {
    std::cerr << "\nReceived signal " << signal << ", stopping server...\n";
    if (g_server_ptr) {
        g_server_ptr->stop();
    }
}

void setup_signal_handlers(httplib::Server &server) {
    g_server_ptr = &server;
    std::signal(SIGTERM, handle_signal);
    std::signal(SIGINT, handle_signal);
}


std::string get_docker_endpoint() {
    if (const char* ep = std::getenv("DOCKER_ENDPOINT")) {
        return {ep};
    }
    return "unix:///var/run/docker.sock";
}

httplib::Client get_docker_client() {
    std::string ep = get_docker_endpoint();

    httplib::Client client{""};

    const std::string prefix = "unix://";
    if (ep.rfind(prefix, 0) == 0) {
        ep = ep.substr(prefix.size()); // remove "unix://"
        client = httplib::Client(ep.c_str());
        client.set_address_family(AF_UNIX);
    } else {
        client = httplib::Client(ep.c_str());
    }

    client.set_read_timeout(5, 0);

    client.set_default_headers({{"Host", "localhost"}});

    return client;
}

std::string get_docker_containers() {
    auto docker_client = get_docker_client();
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

std::string get_container_stats(const std::vector<std::string>& ids) {
    auto docker_client = get_docker_client();

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
    const std::string endpoint = get_docker_endpoint();
    std::cerr << "Using Docker endpoint: " << endpoint << std::endl;

    httplib::Server svr;

    svr.Get("/containers", [](const httplib::Request&, httplib::Response &res) {
        const std::string containers = get_docker_containers();
        res.set_content(containers, "application/json");
    });

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

    setup_signal_handlers(svr);

    std::cout << "Server listening on 0.0.0.0:8080\n";
    svr.listen("0.0.0.0", 8080);

    return 0;
}
