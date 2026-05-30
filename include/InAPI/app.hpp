#ifndef APP
#define APP

#pragma once

#include <httplib.h>
#include <exception>
#include <functional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <error.hpp>
#include <request.hpp>
#include <response.hpp>
#include <json.hpp>

class App {
    private:
        std::unordered_map<int, std::function<Response(Request)>> error_handlers;
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

    public:
        using Handler = std::function<Response(Request)>;
        using SimpleHandler = std::function<Response()>;
        using ErrorHandler = std::function<Response(Request)>;
        using ExceptionHandler = std::function<Response(const std::exception&)>;

        App() {
            server.set_error_handler([this](const httplib::Request& req, httplib::Response& res) {
                if (!res.body.empty()) {
                    return httplib::Server::HandlerResponse::Unhandled;
                }

                auto found = error_handlers.find(res.status);

                if (found != error_handlers.end()) {
                    send(found->second(Request(req)), res);
                    return httplib::Server::HandlerResponse::Handled;
                }

                send(error(res.status), res);
                return httplib::Server::HandlerResponse::Handled;
            });
        }

        void error_handler(int status, ErrorHandler handler) {
            error_handlers[status] = std::move(handler);
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
            server.Get(route.path, [handler, route](const httplib::Request& req, httplib::Response& res) {
                send(handler(Request(req, route_params(route, req))), res);
            });
        }

        void get(const std::string& path, SimpleHandler handler) {
            server.Get(route_path(path).path, [handler](const httplib::Request&, httplib::Response& res) {
                send(handler(), res);
            });
        }

        void post(const std::string& path, Handler handler) {
            Route route = route_path(path);
            server.Post(route.path, [handler, route](const httplib::Request& req, httplib::Response& res) {
                send(handler(Request(req, route_params(route, req))), res);
            });
        }

        void post(const std::string& path, SimpleHandler handler) {
            server.Post(route_path(path).path, [handler](const httplib::Request&, httplib::Response& res) {
                send(handler(), res);
            });
        }

        void put(const std::string& path, Handler handler) {
            Route route = route_path(path);
            server.Put(route.path, [handler, route](const httplib::Request& req, httplib::Response& res) {
                send(handler(Request(req, route_params(route, req))), res);
            });
        }

        void put(const std::string& path, SimpleHandler handler) {
            server.Put(route_path(path).path, [handler](const httplib::Request&, httplib::Response& res) {
                send(handler(), res);
            });
        }

        void patch(const std::string& path, Handler handler) {
            Route route = route_path(path);
            server.Patch(route.path, [handler, route](const httplib::Request& req, httplib::Response& res) {
                send(handler(Request(req, route_params(route, req))), res);
            });
        }

        void patch(const std::string& path, SimpleHandler handler) {
            server.Patch(route_path(path).path, [handler](const httplib::Request&, httplib::Response& res) {
                send(handler(), res);
            });
        }

        void del(const std::string& path, Handler handler) {
            Route route = route_path(path);
            server.Delete(route.path, [handler, route](const httplib::Request& req, httplib::Response& res) {
                send(handler(Request(req, route_params(route, req))), res);
            });
        }

        void del(const std::string& path, SimpleHandler handler) {
            server.Delete(route_path(path).path, [handler](const httplib::Request&, httplib::Response& res) {
                send(handler(), res);
            });
        }

        void options(const std::string& path, Handler handler) {
            Route route = route_path(path);
            server.Options(route.path, [handler, route](const httplib::Request& req, httplib::Response& res) {
                send(handler(Request(req, route_params(route, req))), res);
            });
        }

        void options(const std::string& path, SimpleHandler handler) {
            server.Options(route_path(path).path, [handler](const httplib::Request&, httplib::Response& res) {
                send(handler(), res);
            });
        }

        void run(int port, const std::string& host = "0.0.0.0") {
            server.listen(host, port);
        }
};

#endif
