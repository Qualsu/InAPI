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
#include <stdexcept>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <config.hpp>
#include <encoding.hpp>
#include <error.hpp>
#include <middleware.hpp>
#include <request.hpp>
#include <response.hpp>
#include <route.hpp>
#include <router.hpp>
#include <validation.hpp>
#include <json.hpp>
#include <logger.hpp>

class App {
    public:
        using Handler = std::function<Response(Request)>;
        using SimpleHandler = std::function<Response()>;
        using ErrorHandler = std::function<Response(Request)>;
        using ExceptionHandler = std::function<Response(const std::exception&)>;

    private:
        struct RegisteredRoute {
            RouteMethod method;
            std::string path;
            Handler handler;
            std::vector<Middleware> middlewares;
        };

        struct RegisteredMount {
            std::string mount;
            std::string directory;
        };

        std::unordered_map<int, std::function<Response(Request)>> error_handlers;
        std::vector<RegisteredRoute> routes;
        std::vector<RegisteredMount> mounts;
        std::vector<Middleware> middlewares;
        CorsOptions cors_options;
        bool cors_enabled = false;
        bool exception_handler_enabled = false;
        bool logger_enabled = true;
        ExceptionHandler exception_handler_callback;
        std::unique_ptr<httplib::Server> server;

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
                if (route_regex_special(symbol)) {
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

        static std::string join_route_path(const std::string& prefix, const std::string& path) {
            if (path.empty() || path == "/") {
                return prefix;
            }

            if (prefix == "/") {
                return path.front() == '/' ? path : "/" + path;
            }

            return path.front() == '/' ? prefix + path : prefix + "/" + path;
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
            InAPILogger::request(request, response);
        }

        void set_error_handler() {
            server->set_error_handler([this](const httplib::Request& req, httplib::Response& res) {
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

        void set_cors_handler() {
            if (!cors_enabled) {
                return;
            }

            server->set_pre_routing_handler([this](const httplib::Request& req, httplib::Response& res) {
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
        }

        void set_exception_handler() {
            if (!exception_handler_enabled) {
                return;
            }

            server->set_exception_handler([this](const httplib::Request&, httplib::Response& res, std::exception_ptr ep) {
                if (!ep) {
                    send(error(500), res);
                    return;
                }

                try {
                    std::rethrow_exception(ep);
                } catch (const std::exception& exception) {
                    try {
                        send(exception_handler_callback(exception), res);
                    } catch (...) {
                        send(error(500), res);
                    }

                    return;
                } catch (...) {
                    send(error(500), res);
                }
            });
        }

        void set_server_handlers() {
            set_error_handler();
            set_cors_handler();
            set_exception_handler();
        }

        void add_mount_to_server(const std::string& mount, const std::string& directory) {
            std::string normalized_mount = normalize_mount(mount);
            std::string pattern = static_route_pattern(normalized_mount);
            std::filesystem::path root = InAPIEncoding::path_from_utf8(directory);

            server->Get(pattern, [this, normalized_mount, root](const httplib::Request& req, httplib::Response& res) {
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

                    std::filesystem::path requested = relative.empty() ? base : base / InAPIEncoding::path_from_utf8(relative);
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

        void create_plain_server() {
            server.reset(new httplib::Server());
            set_server_handlers();
        }

        void create_server(const Config& config) {
            if (!config.ssl) {
                create_plain_server();
                return;
            }

#ifndef CPPHTTPLIB_OPENSSL_SUPPORT
            throw std::runtime_error("SSL is configured, but InAPI was built without CPPHTTPLIB_OPENSSL_SUPPORT. Compile with -DCPPHTTPLIB_OPENSSL_SUPPORT -lssl -lcrypto.");
#else
            if (!std::filesystem::is_regular_file(config.ssl->cert_file)) {
                throw std::runtime_error("SSL certificate file not found: " + config.ssl->cert_file);
            }

            if (!std::filesystem::is_regular_file(config.ssl->key_file)) {
                throw std::runtime_error("SSL private key file not found: " + config.ssl->key_file);
            }

            server.reset(new httplib::SSLServer(config.ssl->cert_file.c_str(), config.ssl->key_file.c_str()));

            if (!server->is_valid()) {
                throw std::runtime_error("Failed to initialize SSL server with certificate: " + config.ssl->cert_file);
            }

            set_server_handlers();
#endif
        }

        void apply_config(const Config& config) {
            logger_enabled = config.logger;

            server->new_task_queue = [threads = config.threads] {
                return new httplib::ThreadPool(static_cast<std::size_t>(threads));
            };

            server->set_payload_max_length(max_body_size_bytes(config.max_body_size));
            server->set_read_timeout(static_cast<time_t>(config.read_timeout_seconds));
            server->set_write_timeout(static_cast<time_t>(config.write_timeout_seconds));
            server->set_keep_alive_timeout(static_cast<time_t>(config.idle_timeout_seconds));
        }

        Response handle(Request request, Handler handler) const {
            return handle(std::move(request), std::move(handler), {});
        }

        Response handle(Request request, Handler handler, std::vector<Middleware> route_middlewares) const {
            auto next = std::make_shared<Next>();
            size_t app_index = 0;
            size_t route_index = 0;

            *next = [this, handler = std::move(handler), route_middlewares = std::move(route_middlewares), next, app_index, route_index](Request current) mutable -> Response {
                if (app_index < middlewares.size()) {
                    Middleware current_middleware = middlewares[app_index++];
                    return current_middleware(current, *next);
                }

                if (route_index < route_middlewares.size()) {
                    Middleware current_middleware = route_middlewares[route_index++];
                    return current_middleware(current, *next);
                }

                return handler(current);
            };

            Response response;

            try {
                response = (*next)(request);
            } catch (const ValidationException& exception) {
                response = json(validation_error_json(exception.details()), 422);
            }

            if (logger_enabled) {
                log_request(request, response);
            }

            return response;
        }

        void add_route_to_server(const RegisteredRoute& registered_route) {
            CompiledRoute route = compile_route_path(registered_route.path);

            auto callback = [this, handler = registered_route.handler, route, route_middlewares = registered_route.middlewares](const httplib::Request& req, httplib::Response& res) {
                Request request(req, route_params(route, req));
                send(handle(request, handler, route_middlewares), res);
            };

            switch (registered_route.method) {
                case RouteMethod::Get:
                    server->Get(route.path, callback);
                    break;
                case RouteMethod::Post:
                    server->Post(route.path, callback);
                    break;
                case RouteMethod::Put:
                    server->Put(route.path, callback);
                    break;
                case RouteMethod::Patch:
                    server->Patch(route.path, callback);
                    break;
                case RouteMethod::Delete:
                    server->Delete(route.path, callback);
                    break;
                case RouteMethod::Options:
                    server->Options(route.path, callback);
                    break;
            }
        }

        void add_routes_to_server() {
            for (const auto& route : routes) {
                add_route_to_server(route);
            }
        }

        void add_mounts_to_server() {
            for (const auto& mount : mounts) {
                add_mount_to_server(mount.mount, mount.directory);
            }
        }

        void register_route(RouteMethod method, const std::string& path, Handler handler, std::vector<Middleware> route_middlewares = {}) {
            routes.push_back({method, path, std::move(handler), std::move(route_middlewares)});
            add_route_to_server(routes.back());
        }

    public:
        App() {
            create_plain_server();
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
                set_cors_handler();

                middleware([this](Request request, Next next) {
                    Response response = next(request);
                    apply_cors_headers(response, cors_options, request);
                    return response;
                });
            }
        }

        void BearerAuth(const std::string& token) {
            BearerAuth([token](Request request) {
                auto bearer = request.bearer_token();
                return bearer && *bearer == token;
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
            exception_handler_callback = std::move(handler);
            exception_handler_enabled = true;
            set_exception_handler();
        }

        void mount(const std::string& mount, const std::string& directory) {
            mounts.push_back({mount, directory});
            add_mount_to_server(mount, directory);
        }

        void include(const std::string& prefix, const Router& router) {
            std::string normalized_prefix = normalize_mount(prefix);
            std::vector<Middleware> router_middlewares = router.middlewares();

            for (const auto& route : router.routes()) {
                register_route(route.method, join_route_path(normalized_prefix, route.path), route.handler, router_middlewares);
            }
        }

        void get(const std::string& path, Handler handler) {
            register_route(RouteMethod::Get, path, std::move(handler));
        }

        void get(const std::string& path, SimpleHandler handler) {
            get(path, [handler = std::move(handler)](Request) {
                return handler();
            });
        }

        void post(const std::string& path, Handler handler) {
            register_route(RouteMethod::Post, path, std::move(handler));
        }

        void post(const std::string& path, SimpleHandler handler) {
            post(path, [handler = std::move(handler)](Request) {
                return handler();
            });
        }

        void put(const std::string& path, Handler handler) {
            register_route(RouteMethod::Put, path, std::move(handler));
        }

        void put(const std::string& path, SimpleHandler handler) {
            put(path, [handler = std::move(handler)](Request) {
                return handler();
            });
        }

        void patch(const std::string& path, Handler handler) {
            register_route(RouteMethod::Patch, path, std::move(handler));
        }

        void patch(const std::string& path, SimpleHandler handler) {
            patch(path, [handler = std::move(handler)](Request) {
                return handler();
            });
        }

        void del(const std::string& path, Handler handler) {
            register_route(RouteMethod::Delete, path, std::move(handler));
        }

        void del(const std::string& path, SimpleHandler handler) {
            del(path, [handler = std::move(handler)](Request) {
                return handler();
            });
        }

        void options(const std::string& path, Handler handler) {
            register_route(RouteMethod::Options, path, std::move(handler));
        }

        void options(const std::string& path, SimpleHandler handler) {
            options(path, [handler = std::move(handler)](Request) {
                return handler();
            });
        }

        void run(int port, Config config = Config()) {
            run("0.0.0.0", port, std::move(config));
        }

        void run(int port, const std::string& host, Config config = Config()) {
            run(host, port, std::move(config));
        }

        void run(const std::string& host, int port, Config config = Config()) {
            InAPIEncoding::setup_utf8_console();
            create_server(config);
            add_routes_to_server();
            add_mounts_to_server();
            apply_config(config);
            std::string url_host = host == "0.0.0.0" ? "127.0.0.1" : host;
            InAPILogger::startup(url_host, port, host);

            server->listen(host, port);
        }
};

#endif
