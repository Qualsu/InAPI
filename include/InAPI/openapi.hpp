#ifndef INAPI_OPENAPI
#define INAPI_OPENAPI

#pragma once

#include <cctype>
#include <memory>
#include <sstream>
#include <string>
#include <type_traits>
#include <typeinfo>
#include <utility>
#include <vector>
#include <json.hpp>
#include <route.hpp>

struct OpenApiResponse {
    int status = 200;
    std::string description = "OK";
    std::string schema_name;
    nlohmann::json schema;
};

struct OpenApiOperation {
    std::string summary_text;
    std::vector<std::string> tags;
    std::vector<OpenApiResponse> responses;
    bool bearer_auth = false;
};

struct Swagger {
    bool enabled;
    std::string path;
    std::string title;
    std::string version;
    std::string description;
    bool bearer_auth;

    explicit Swagger(bool enabled = true,
                           std::string path = "/docs",
                           std::string title = "InAPI",
                           std::string version = "1.0.0",
                           std::string description = "",
                           bool bearer_auth = false)
        : enabled(enabled),
          path(std::move(path)),
          title(std::move(title)),
          version(std::move(version)),
          description(std::move(description)),
          bearer_auth(bearer_auth) {}
};

namespace InAPIOpenApi {
    template <typename, typename = void>
    struct HasOpenApiSchema : std::false_type {};

    template <typename T>
    struct HasOpenApiSchema<T, std::void_t<decltype(T::openapi_schema())>> : std::true_type {};

    template <typename, typename = void>
    struct HasOpenApiName : std::false_type {};

    template <typename T>
    struct HasOpenApiName<T, std::void_t<decltype(T::openapi_name())>> : std::true_type {};

    inline std::string clean_type_name(std::string name) {
        const std::vector<std::string> prefixes = {"class ", "struct ", "enum "};

        for (const auto& prefix : prefixes) {
            if (name.compare(0, prefix.size(), prefix) == 0) {
                name.erase(0, prefix.size());
            }
        }

        while (!name.empty() && std::isdigit(static_cast<unsigned char>(name.front()))) {
            name.erase(name.begin());
        }

        for (char& symbol : name) {
            if (!std::isalnum(static_cast<unsigned char>(symbol)) && symbol != '_') {
                symbol = '_';
            }
        }

        return name.empty() ? "Response" : name;
    }

    template <typename T>
    std::string schema_name() {
        if constexpr (HasOpenApiName<T>::value) {
            return T::openapi_name();
        } else {
            return clean_type_name(typeid(T).name());
        }
    }

    template <typename T>
    nlohmann::json schema() {
        if constexpr (HasOpenApiSchema<T>::value) {
            return T::openapi_schema();
        } else {
            return {
                {"type", "object"}
            };
        }
    }

    inline std::string method_name(RouteMethod method) {
        switch (method) {
            case RouteMethod::Get: return "get";
            case RouteMethod::Post: return "post";
            case RouteMethod::Put: return "put";
            case RouteMethod::Patch: return "patch";
            case RouteMethod::Delete: return "delete";
            case RouteMethod::Options: return "options";
        }

        return "get";
    }

    inline std::string status_description(int status) {
        switch (status) {
            case 200: return "OK";
            case 201: return "Created";
            case 204: return "No Content";
            case 400: return "Bad Request";
            case 401: return "Unauthorized";
            case 403: return "Forbidden";
            case 404: return "Not Found";
            case 422: return "Unprocessable Entity";
            case 500: return "Internal Server Error";
        }

        return "Response";
    }

    inline std::string openapi_path(const std::string& path) {
        std::string result;
        result.reserve(path.size());

        for (size_t i = 0; i < path.size(); ++i) {
            if (path[i] != '{') {
                result += path[i];
                continue;
            }

            size_t close = path.find('}', i + 1);

            if (close == std::string::npos) {
                result += path[i];
                continue;
            }

            std::string token = path.substr(i + 1, close - i - 1);
            size_t type_start = token.find(':');
            result += "{";
            result += type_start == std::string::npos ? token : token.substr(0, type_start);
            result += "}";
            i = close;
        }

        return result;
    }

    inline std::vector<std::pair<std::string, std::string>> path_params(const std::string& path) {
        std::vector<std::pair<std::string, std::string>> params;

        for (size_t i = 0; i < path.size(); ++i) {
            if (path[i] != '{') {
                continue;
            }

            size_t close = path.find('}', i + 1);

            if (close == std::string::npos) {
                continue;
            }

            std::string token = path.substr(i + 1, close - i - 1);
            size_t type_start = token.find(':');
            std::string name = type_start == std::string::npos ? token : token.substr(0, type_start);
            std::string type = type_start == std::string::npos ? "string" : token.substr(type_start + 1);
            params.push_back({name, type});
            i = close;
        }

        return params;
    }

    inline nlohmann::json param_schema(const std::string& type) {
        if (type == "int") {
            return {{"type", "integer"}};
        }

        return {{"type", "string"}};
    }
}

class RouteDoc {
    public:
        explicit RouteDoc(std::shared_ptr<OpenApiOperation> operation = nullptr)
            : operation(std::move(operation)) {}

        RouteDoc& summary(std::string value) {
            if (operation) {
                operation->summary_text = std::move(value);
            }

            return *this;
        }

        RouteDoc& tag(std::string value) {
            if (operation) {
                operation->tags.push_back(std::move(value));
            }

            return *this;
        }

        RouteDoc& response(int status, std::string description = "") {
            if (operation) {
                if (description.empty()) {
                    description = InAPIOpenApi::status_description(status);
                }

                operation->responses.push_back({status, std::move(description), "", nlohmann::json()});
            }

            return *this;
        }

        template <typename T>
        RouteDoc& response(int status, std::string description = "") {
            if (operation) {
                if (description.empty()) {
                    description = InAPIOpenApi::status_description(status);
                }

                operation->responses.push_back({
                    status,
                    std::move(description),
                    InAPIOpenApi::schema_name<T>(),
                    InAPIOpenApi::schema<T>()
                });
            }

            return *this;
        }

        RouteDoc& bearer_auth(bool enabled = true) {
            if (operation) {
                operation->bearer_auth = enabled;
            }

            return *this;
        }

    private:
        std::shared_ptr<OpenApiOperation> operation;
};

struct OpenApiRoute {
    RouteMethod method;
    std::string path;
    std::shared_ptr<OpenApiOperation> operation;
};

inline nlohmann::json openapi_json(const std::vector<OpenApiRoute>& routes, const Swagger& config = Swagger()) {
    nlohmann::json document = {
        {"openapi", "3.0.3"},
        {"info", {
            {"title", config.title},
            {"version", config.version}
        }},
        {"paths", nlohmann::json::object()},
        {"components", {
            {"schemas", nlohmann::json::object()}
        }}
    };

    if (!config.description.empty()) {
        document["info"]["description"] = config.description;
    }

    bool has_bearer_auth = config.bearer_auth;

    for (const auto& route : routes) {
        if (route.operation && route.operation->bearer_auth) {
            has_bearer_auth = true;
            break;
        }
    }

    if (has_bearer_auth) {
        document["components"]["securitySchemes"]["BearerAuth"] = {
            {"type", "http"},
            {"scheme", "bearer"},
            {"bearerFormat", "JWT"}
        };
    }

    for (const auto& route : routes) {
        if (!route.operation) {
            continue;
        }

        nlohmann::json operation = nlohmann::json::object();

        if (!route.operation->summary_text.empty()) {
            operation["summary"] = route.operation->summary_text;
        }

        if (!route.operation->tags.empty()) {
            operation["tags"] = route.operation->tags;
        }

        if (config.bearer_auth || route.operation->bearer_auth) {
            operation["security"] = nlohmann::json::array({
                {{"BearerAuth", nlohmann::json::array()}}
            });
        }

        nlohmann::json parameters = nlohmann::json::array();

        for (const auto& param : InAPIOpenApi::path_params(route.path)) {
            parameters.push_back({
                {"name", param.first},
                {"in", "path"},
                {"required", true},
                {"schema", InAPIOpenApi::param_schema(param.second)}
            });
        }

        if (!parameters.empty()) {
            operation["parameters"] = std::move(parameters);
        }

        nlohmann::json responses = nlohmann::json::object();

        if (route.operation->responses.empty()) {
            responses["200"] = {
                {"description", "OK"}
            };
        }

        for (const auto& response : route.operation->responses) {
            nlohmann::json response_json = {
                {"description", response.description}
            };

            if (!response.schema_name.empty()) {
                document["components"]["schemas"][response.schema_name] = response.schema;
                response_json["content"] = {
                    {"application/json", {
                        {"schema", {
                            {"$ref", "#/components/schemas/" + response.schema_name}
                        }}
                    }}
                };
            }

            responses[std::to_string(response.status)] = std::move(response_json);
        }

        operation["responses"] = std::move(responses);
        document["paths"][InAPIOpenApi::openapi_path(route.path)][InAPIOpenApi::method_name(route.method)] = std::move(operation);
    }

    return document;
}

inline std::string swagger_html(const std::string& openapi_url) {
    std::ostringstream html;
    html << "<!doctype html><html><head><meta charset=\"utf-8\">";
    html << "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">";
    html << "<title>InAPI Swagger</title>";
    html << "<link rel=\"stylesheet\" href=\"https://unpkg.com/swagger-ui-dist@5/swagger-ui.css\">";
    html << "</head><body><div id=\"swagger-ui\"></div>";
    html << "<script src=\"https://unpkg.com/swagger-ui-dist@5/swagger-ui-bundle.js\"></script>";
    html << "<script>window.onload=function(){SwaggerUIBundle({url:'" << openapi_url << "',dom_id:'#swagger-ui'});};</script>";
    html << "</body></html>";
    return html.str();
}

#endif
