#ifndef INAPI_ROUTE
#define INAPI_ROUTE

#pragma once

#include <httplib.h>
#include <string>
#include <unordered_map>
#include <vector>

enum class RouteMethod {
    Get,
    Post,
    Put,
    Patch,
    Delete,
    Options
};

struct CompiledRoute {
    std::string path;
    std::vector<std::string> params;
};

inline bool route_regex_special(char symbol) {
    static const std::string special = R"(\.^$|()[]{}*+?)";
    return special.find(symbol) != std::string::npos;
}

inline std::string route_param_pattern(const std::string& type) {
    if (type == "int") {
        return R"((-?[0-9]+))";
    }

    if (type == "path") {
        return R"((.+))";
    }

    return R"(([^/]+))";
}

inline CompiledRoute compile_route_path(const std::string& path) {
    CompiledRoute route;
    route.path.reserve(path.size() + 8);

    for (size_t i = 0; i < path.size(); ++i) {
        if (path[i] != '{') {
            if (route_regex_special(path[i])) {
                route.path += '\\';
            }

            route.path += path[i];
            continue;
        }

        size_t close = path.find('}', i + 1);

        if (close == std::string::npos) {
            route.path += "\\{";
            continue;
        }

        std::string token = path.substr(i + 1, close - i - 1);
        size_t type_start = token.find(':');
        std::string name = type_start == std::string::npos ? token : token.substr(0, type_start);
        std::string type = type_start == std::string::npos ? "string" : token.substr(type_start + 1);

        route.params.push_back(name);
        route.path += route_param_pattern(type);
        i = close;
    }

    return route;
}

inline std::unordered_map<std::string, std::string> route_params(const CompiledRoute& route, const httplib::Request& req) {
    std::unordered_map<std::string, std::string> params;

    for (size_t i = 0; i < route.params.size(); ++i) {
        size_t match_index = i + 1;

        if (match_index >= req.matches.size()) {
            break;
        }

        params.emplace(route.params[i], req.matches[match_index].str());
    }

    return params;
}

#endif
