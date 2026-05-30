#ifndef APP
#define APP

#pragma once

#include <httplib.h>
#include <chrono>
#include <cctype>
#include <ctime>
#include <exception>
#include <filesystem>
#include <functional>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <error.hpp>
#include <middleware.hpp>
#include <request.hpp>
#include <response.hpp>
#include <json.hpp>

class App {
    public:
        using Handler = std::function<Response(Request)>;
        using SimpleHandler = std::function<Response()>;
        using ErrorHandler = std::function<Response(Request)>;
        using ExceptionHandler = std::function<Response(const std::exception&)>;

    private:
        std::unordered_map<int, std::function<Response(Request)>> error_handlers;
        std::vector<Middleware> middlewares;
        CorsOptions cors_options;
        bool cors_enabled = false;
        bool logger_enabled = true;
        httplib::Server server;

        struct Route {
            std::string path;
            std::vector<std::string> params;
        };

        static bool regex_special(char symbol) {
            static const std::string special = R"(\.^$|()[]{}*+?)";
            return special.find(symbol) != std::string::npos;
        }

        static std::string param_pattern(const std::string& type) {
            if (type == "int") {
                return R"((-?[0-9]+))";
            }

            if (type == "path") {
                return R"((.+))";
            }

            return R"(([^/]+))";
        }

        static Route route_path(const std::string& path) {
            Route route;
            route.path.reserve(path.size() + 8);

            for (size_t i = 0; i < path.size(); ++i) {
                if (path[i] != '{') {
                    if (regex_special(path[i])) {
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
                route.path += param_pattern(type);
                i = close;
            }

            return route;
        }

        static std::unordered_map<std::string, std::string> route_params(const Route& route, const httplib::Request& req) {
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

        static void send(Response response, httplib::Response& res) {
            res.status = response.status;

            for (const auto& header : response.headers) {
                res.set_header(header.first, header.second);
            }

            for (const auto& cookie : response.cookie_headers) {
                res.headers.emplace("Set-Cookie", cookie);
            }

            res.set_content(response.body, response.content_type);
        }

        static std::string normalize_mount(std::string mount) {
            if (mount.empty() || mount[0] != '/') {
                mount.insert(mount.begin(), '/');
            }

            while (mount.size() > 1 && mount.back() == '/') {
                mount.pop_back();
            }

            return mount;
        }

        static std::string static_route_pattern(const std::string& mount) {
            if (mount == "/") {
                return R"((/.*)?)";
            }

            std::string pattern;
            pattern.reserve(mount.size() + 8);

            for (char symbol : mount) {
                if (regex_special(symbol)) {
                    pattern += '\\';
                }

                pattern += symbol;
            }

            return pattern + R"((/.*)?)";
        }

        static std::string static_relative_path(const std::string& mount, const std::string& request_path) {
            if (mount == "/") {
                return request_path.empty() ? "/" : request_path;
            }

            if (request_path.size() < mount.size() || request_path.compare(0, mount.size(), mount) != 0) {
                return "";
            }

            if (request_path.size() > mount.size() && request_path[mount.size()] != '/') {
                return "";
            }

            std::string relative = request_path.substr(mount.size());
            return relative.empty() ? "/" : relative;
        }

        static bool safe_static_path(const std::string& path) {
            std::string segment;

            for (char symbol : path) {
                if (symbol == '\\') {
                    return false;
                }

                if (symbol == '/') {
                    if (segment == "..") {
                        return false;
                    }

                    segment.clear();
                    continue;
                }

                segment += symbol;
            }

            return segment != "..";
        }

        static bool path_inside(const std::filesystem::path& root, const std::filesystem::path& path) {
            std::error_code error;
            std::filesystem::path relative = std::filesystem::relative(path, root, error);

            if (error) {
                return false;
            }

            if (relative.empty()) {
                return true;
            }

            auto begin = relative.begin();
            return begin == relative.end() || *begin != "..";
        }

        static std::string http_date(std::filesystem::file_time_type time) {
            auto system_time = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                time - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now()
            );
            std::time_t raw = std::chrono::system_clock::to_time_t(system_time);
            std::tm utc = *std::gmtime(&raw);
            std::ostringstream result;

            result << std::put_time(&utc, "%a, %d %b %Y %H:%M:%S GMT");
            return result.str();
        }

        static std::string static_etag(const std::filesystem::path& path, std::uintmax_t size) {
            std::error_code error;
            auto write_time = std::filesystem::last_write_time(path, error);

            if (error) {
                return "";
            }

            auto ticks = write_time.time_since_epoch().count();
            std::ostringstream result;
            result << "W/\"" << size << "-" << ticks << "\"";
            return result.str();
        }

        static bool header_has_etag(const std::string& header, const std::string& etag) {
            size_t position = 0;

            while (position < header.size()) {
                size_t comma = header.find(',', position);
                std::string value = header.substr(position, comma == std::string::npos ? std::string::npos : comma - position);

                while (!value.empty() && std::isspace(static_cast<unsigned char>(value.front()))) {
                    value.erase(value.begin());
                }

                while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back()))) {
                    value.pop_back();
                }

                if (value == "*" || value == etag) {
                    return true;
                }

                if (comma == std::string::npos) {
                    break;
                }

                position = comma + 1;
            }

            return false;
        }

        static void static_cache_headers(Response& response, const std::string& etag, const std::string& last_modified) {
            response.header("Cache-Control", "public, max-age=3600");

            if (!etag.empty()) {
                response.header("ETag", etag);
            }

            if (!last_modified.empty()) {
                response.header("Last-Modified", last_modified);
            }
        }

        static std::string read_static_file(const std::filesystem::path& path) {
            std::ifstream stream(path, std::ios::binary);
            std::ostringstream body;
            body << stream.rdbuf();
            return body.str();
        }

        static Response static_file_response(const Request& request, const std::filesystem::path& path) {
            std::error_code error;
            std::uintmax_t size = std::filesystem::file_size(path, error);

            if (error) {
                return Response(404, "File not found", "text/plain; charset=utf-8");
            }

            std::string etag = static_etag(path, size);
            auto write_time = std::filesystem::last_write_time(path, error);
            std::string last_modified = error ? "" : http_date(write_time);
            std::string if_none_match = request.header("If-None-Match");

            if ((!etag.empty() && !if_none_match.empty() && header_has_etag(if_none_match, etag)) ||
                (if_none_match.empty() && !last_modified.empty() && request.header("If-Modified-Since") == last_modified)) {
                Response response = status(304);
                static_cache_headers(response, etag, last_modified);
                return response;
            }

            Response response(200, read_static_file(path), file_content_type(path.string()));
            static_cache_headers(response, etag, last_modified);
            return response;
        }

        static std::string join(const std::vector<std::string>& values) {
            std::ostringstream result;

            for (size_t i = 0; i < values.size(); ++i) {
                if (i != 0) {
                    result << ", ";
                }

                result << values[i];
            }

            return result.str();
        }

        static bool contains(const std::vector<std::string>& values, const std::string& value) {
            for (const auto& item : values) {
                if (item == value) {
                    return true;
                }
            }

            return false;
        }

        static std::string reason_phrase(int status) {
            switch (status) {
                case 200: return "OK";
                case 201: return "Created";
                case 202: return "Accepted";
                case 204: return "No Content";
                case 301: return "Moved Permanently";
                case 302: return "Found";
                case 304: return "Not Modified";
                case 400: return "Bad Request";
                case 401: return "Unauthorized";
                case 403: return "Forbidden";
                case 404: return "Not Found";
                case 405: return "Method Not Allowed";
                case 409: return "Conflict";
                case 422: return "Unprocessable Entity";
                case 500: return "Internal Server Error";
                case 502: return "Bad Gateway";
                case 503: return "Service Unavailable";
                default: return "";
            }
        }

        static void apply_cors_headers(Response& response, const CorsOptions& options, const Request& request) {
            std::string origin = request.header("Origin");

            if (contains(options.origins, "*")) {
                response.header("Access-Control-Allow-Origin", "*");
            } else if (!origin.empty() && contains(options.origins, origin)) {
                response.header("Access-Control-Allow-Origin", origin);
                response.header("Vary", "Origin");
            }

            response.header("Access-Control-Allow-Methods", join(options.methods));
            response.header("Access-Control-Allow-Headers", join(options.headers));
        }

        static void log_request(const Request& request, const Response& response) {
            std::cout << "INFO: " << request.ip()
                      << " - \"" << request.method() << " " << request.path() << "\" "
                      << response.status;

            std::string reason = reason_phrase(response.status);

            if (!reason.empty()) {
                std::cout << " " << reason;
            }

            std::cout << "\n";
        }

        Response handle(Request request, Handler handler) const {
            auto next = std::make_shared<Next>();
            size_t index = 0;

            *next = [this, handler = std::move(handler), next, index](Request current) mutable -> Response {
                if (index >= middlewares.size()) {
                    return handler(current);
                }

                Middleware current_middleware = middlewares[index++];
                return current_middleware(current, *next);
            };

            Response response = (*next)(request);

            if (logger_enabled) {
                log_request(request, response);
            }

            return response;
        }

    public:
        App() {
            server.set_error_handler([this](const httplib::Request& req, httplib::Response& res) {
                if (!res.body.empty()) {
                    return httplib::Server::HandlerResponse::Unhandled;
                }

                auto found = error_handlers.find(res.status);

                if (found != error_handlers.end()) {
                    Request request(req);
                    Response response = found->second(request);

                    if (logger_enabled) {
                        log_request(request, response);
                    }

                    send(response, res);
                    return httplib::Server::HandlerResponse::Handled;
                }

                Request request(req);
                Response response = error(res.status);

                if (logger_enabled) {
                    log_request(request, response);
                }

                send(response, res);
                return httplib::Server::HandlerResponse::Handled;
            });
        }

        void error_handler(int status, ErrorHandler handler) {
            error_handlers[status] = std::move(handler);
        }

        void middleware(Middleware handler) {
            middlewares.push_back(std::move(handler));
        }

        void Cors(CorsOptions options = {}) {
            cors_options = std::move(options);

            if (!cors_enabled) {
                cors_enabled = true;

                server.set_pre_routing_handler([this](const httplib::Request& req, httplib::Response& res) {
                    if (req.method != "OPTIONS") {
                        return httplib::Server::HandlerResponse::Unhandled;
                    }

                    Response response = status(204);
                    Request request(req);
                    apply_cors_headers(response, cors_options, request);

                    if (logger_enabled) {
                        log_request(request, response);
                    }

                    send(response, res);

                    return httplib::Server::HandlerResponse::Handled;
                });

                middleware([this](Request request, Next next) {
                    Response response = next(request);
                    apply_cors_headers(response, cors_options, request);
                    return response;
                });
            }
        }

        void BearerAuth(const std::string& token) {
            BearerAuth([token](Request request) {
                return request.bearer_token() == token;
            });
        }

        void BearerAuth(AuthHook hook) {
            middleware([hook = std::move(hook)](Request request, Next next) {
                if (!hook(request)) {
                    Response response = error(401);
                    response.header("WWW-Authenticate", "Bearer");
                    return response;
                }

                return next(request);
            });
        }

        void exception_handler(ExceptionHandler handler) {
            server.set_exception_handler([handler = std::move(handler)](const httplib::Request&, httplib::Response& res, std::exception_ptr ep) {
                if (!ep) {
                    send(error(500), res);
                    return;
                }

                try {
                    std::rethrow_exception(ep);
                } catch (const std::exception& exception) {
                    try {
                        send(handler(exception), res);
                    } catch (...) {
                        send(error(500), res);
                    }

                    return;
                } catch (...) {
                    send(error(500), res);
                }
            });
        }

        void mount(const std::string& mount, const std::string& directory) {
            std::string normalized_mount = normalize_mount(mount);
            std::string pattern = static_route_pattern(normalized_mount);
            std::filesystem::path root = std::filesystem::path(directory);

            server.Get(pattern, [this, normalized_mount, root](const httplib::Request& req, httplib::Response& res) {
                Request request(req);

                Response response = handle(request, [normalized_mount, root](Request current) {
                    std::error_code error;
                    std::filesystem::path base = std::filesystem::weakly_canonical(root, error);

                    if (error) {
                        base = std::filesystem::absolute(root, error);
                    }

                    if (error || !std::filesystem::is_directory(base, error)) {
                        return Response(404, "Static directory not found", "text/plain; charset=utf-8");
                    }

                    std::string relative = static_relative_path(normalized_mount, current.path());

                    if (relative.empty() || !safe_static_path(relative)) {
                        return Response(403, "Forbidden", "text/plain; charset=utf-8");
                    }

                    while (!relative.empty() && relative.front() == '/') {
                        relative.erase(relative.begin());
                    }

                    std::filesystem::path requested = relative.empty() ? base : base / std::filesystem::path(relative);
                    std::filesystem::path target = requested;

                    if (std::filesystem::is_directory(target, error)) {
                        target /= "index.html";
                    }

                    if (!std::filesystem::is_regular_file(target, error)) {
                        target = base / "index.html";
                    }

                    std::filesystem::path real_target = std::filesystem::weakly_canonical(target, error);

                    if (error || !path_inside(base, real_target)) {
                        return Response(403, "Forbidden", "text/plain; charset=utf-8");
                    }

                    if (!std::filesystem::is_regular_file(real_target, error)) {
                        return Response(404, "File not found", "text/plain; charset=utf-8");
                    }

                    return static_file_response(current, real_target);
                });

                send(response, res);
            });
        }

        void get(const std::string& path, Handler handler) {
            Route route = route_path(path);
            server.Get(route.path, [this, handler, route](const httplib::Request& req, httplib::Response& res) {
                send(handle(Request(req, route_params(route, req)), handler), res);
            });
        }

        void get(const std::string& path, SimpleHandler handler) {
            server.Get(route_path(path).path, [this, handler](const httplib::Request& req, httplib::Response& res) {
                send(handle(Request(req), [handler](Request) {
                    return handler();
                }), res);
            });
        }

        void post(const std::string& path, Handler handler) {
            Route route = route_path(path);
            server.Post(route.path, [this, handler, route](const httplib::Request& req, httplib::Response& res) {
                send(handle(Request(req, route_params(route, req)), handler), res);
            });
        }

        void post(const std::string& path, SimpleHandler handler) {
            server.Post(route_path(path).path, [this, handler](const httplib::Request& req, httplib::Response& res) {
                send(handle(Request(req), [handler](Request) {
                    return handler();
                }), res);
            });
        }

        void put(const std::string& path, Handler handler) {
            Route route = route_path(path);
            server.Put(route.path, [this, handler, route](const httplib::Request& req, httplib::Response& res) {
                send(handle(Request(req, route_params(route, req)), handler), res);
            });
        }

        void put(const std::string& path, SimpleHandler handler) {
            server.Put(route_path(path).path, [this, handler](const httplib::Request& req, httplib::Response& res) {
                send(handle(Request(req), [handler](Request) {
                    return handler();
                }), res);
            });
        }

        void patch(const std::string& path, Handler handler) {
            Route route = route_path(path);
            server.Patch(route.path, [this, handler, route](const httplib::Request& req, httplib::Response& res) {
                send(handle(Request(req, route_params(route, req)), handler), res);
            });
        }

        void patch(const std::string& path, SimpleHandler handler) {
            server.Patch(route_path(path).path, [this, handler](const httplib::Request& req, httplib::Response& res) {
                send(handle(Request(req), [handler](Request) {
                    return handler();
                }), res);
            });
        }

        void del(const std::string& path, Handler handler) {
            Route route = route_path(path);
            server.Delete(route.path, [this, handler, route](const httplib::Request& req, httplib::Response& res) {
                send(handle(Request(req, route_params(route, req)), handler), res);
            });
        }

        void del(const std::string& path, SimpleHandler handler) {
            server.Delete(route_path(path).path, [this, handler](const httplib::Request& req, httplib::Response& res) {
                send(handle(Request(req), [handler](Request) {
                    return handler();
                }), res);
            });
        }

        void options(const std::string& path, Handler handler) {
            Route route = route_path(path);
            server.Options(route.path, [this, handler, route](const httplib::Request& req, httplib::Response& res) {
                send(handle(Request(req, route_params(route, req)), handler), res);
            });
        }

        void options(const std::string& path, SimpleHandler handler) {
            server.Options(route_path(path).path, [this, handler](const httplib::Request& req, httplib::Response& res) {
                send(handle(Request(req), [handler](Request) {
                    return handler();
                }), res);
            });
        }

        void run(int port, bool logger) {
            run(port, "0.0.0.0", logger);
        }

        void run(int port, const std::string& host = "0.0.0.0", bool logger = true) {
            logger_enabled = logger;
            std::string url_host = host == "0.0.0.0" ? "127.0.0.1" : host;

            std::cout << "INFO: InAPI running on http://" << url_host << ":" << port;

            if (url_host != host) {
                std::cout << " (bound to " << host << ")";
            }

            std::cout << "\n";
            std::cout << "INFO: Press CTRL+C to quit\n";

            server.listen(host, port);
        }
};

#endif
