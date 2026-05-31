#ifndef INAPI_ROUTER
#define INAPI_ROUTER

#pragma once

#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <error.hpp>
#include <middleware.hpp>
#include <openapi.hpp>
#include <request.hpp>
#include <response.hpp>
#include <route.hpp>

class Router {
    public:
        using Handler = std::function<Response(Request)>;
        using SimpleHandler = std::function<Response()>;

        struct Route {
            RouteMethod method;
            std::string path;
            Handler handler;
            std::shared_ptr<OpenApiOperation> openapi;
        };

    private:
        std::vector<Route> router_routes;
        std::vector<Middleware> router_middlewares;
        bool bearer_auth_enabled = false;

        RouteDoc add(RouteMethod method, std::string path, Handler handler) {
            auto operation = std::make_shared<OpenApiOperation>();
            operation->bearer_auth = bearer_auth_enabled;
            router_routes.push_back({method, std::move(path), std::move(handler), operation});
            return RouteDoc(operation);
        }

        RouteDoc add(RouteMethod method, std::string path, SimpleHandler handler) {
            return add(method, std::move(path), [handler = std::move(handler)](Request) {
                return handler();
            });
        }

    public:
        const std::vector<Route>& routes() const {
            return router_routes;
        }

        const std::vector<Middleware>& middlewares() const {
            return router_middlewares;
        }

        void middleware(Middleware handler) {
            router_middlewares.push_back(std::move(handler));
        }

        void BearerAuth(const std::string& token) {
            BearerAuth([token](Request request) {
                auto bearer = request.bearer_token();
                return bearer && *bearer == token;
            });
        }

        void BearerAuth(AuthHook hook) {
            bearer_auth_enabled = true;

            for (auto& route : router_routes) {
                if (route.openapi) {
                    route.openapi->bearer_auth = true;
                }
            }

            middleware([hook = std::move(hook)](Request request, Next next) {
                if (!hook(request)) {
                    Response response = error(401);
                    response.header("WWW-Authenticate", "Bearer");
                    return response;
                }

                return next(request);
            });
        }

        RouteDoc get(const std::string& path, Handler handler) {
            return add(RouteMethod::Get, path, std::move(handler));
        }

        RouteDoc get(const std::string& path, SimpleHandler handler) {
            return add(RouteMethod::Get, path, std::move(handler));
        }

        RouteDoc post(const std::string& path, Handler handler) {
            return add(RouteMethod::Post, path, std::move(handler));
        }

        RouteDoc post(const std::string& path, SimpleHandler handler) {
            return add(RouteMethod::Post, path, std::move(handler));
        }

        RouteDoc put(const std::string& path, Handler handler) {
            return add(RouteMethod::Put, path, std::move(handler));
        }

        RouteDoc put(const std::string& path, SimpleHandler handler) {
            return add(RouteMethod::Put, path, std::move(handler));
        }

        RouteDoc patch(const std::string& path, Handler handler) {
            return add(RouteMethod::Patch, path, std::move(handler));
        }

        RouteDoc patch(const std::string& path, SimpleHandler handler) {
            return add(RouteMethod::Patch, path, std::move(handler));
        }

        RouteDoc del(const std::string& path, Handler handler) {
            return add(RouteMethod::Delete, path, std::move(handler));
        }

        RouteDoc del(const std::string& path, SimpleHandler handler) {
            return add(RouteMethod::Delete, path, std::move(handler));
        }

        RouteDoc options(const std::string& path, Handler handler) {
            return add(RouteMethod::Options, path, std::move(handler));
        }

        RouteDoc options(const std::string& path, SimpleHandler handler) {
            return add(RouteMethod::Options, path, std::move(handler));
        }
};

#endif
