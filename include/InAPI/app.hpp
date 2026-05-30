#ifndef APP
#define APP

#pragma once

#include <httplib.h>
#include <exception>
#include <functional>
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

            res.set_content(response.body, response.content_type);
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
                response.headers["Access-Control-Allow-Origin"] = "*";
            } else if (!origin.empty() && contains(options.origins, origin)) {
                response.headers["Access-Control-Allow-Origin"] = origin;
                response.headers["Vary"] = "Origin";
            }

            response.headers["Access-Control-Allow-Methods"] = join(options.methods);
            response.headers["Access-Control-Allow-Headers"] = join(options.headers);
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
                return request.header("Authorization") == "Bearer " + token;
            });
        }

        void BearerAuth(AuthHook hook) {
            middleware([hook = std::move(hook)](Request request, Next next) {
                if (!hook(request)) {
                    Response response = error(401);
                    response.headers["WWW-Authenticate"] = "Bearer";
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
