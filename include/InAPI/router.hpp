#ifndef INAPI_ROUTER
#define INAPI_ROUTER

#pragma once

#include <functional>
#include <string>
#include <utility>
#include <vector>
#include <error.hpp>
#include <middleware.hpp>
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
        };

    private:
        std::vector<Route> router_routes;
        std::vector<Middleware> router_middlewares;

        void add(RouteMethod method, std::string path, Handler handler) {
            router_routes.push_back({method, std::move(path), std::move(handler)});
        }

        void add(RouteMethod method, std::string path, SimpleHandler handler) {
            add(method, std::move(path), [handler = std::move(handler)](Request) {
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

        void get(const std::string& path, Handler handler) {
            add(RouteMethod::Get, path, std::move(handler));
        }

        void get(const std::string& path, SimpleHandler handler) {
            add(RouteMethod::Get, path, std::move(handler));
        }

        void post(const std::string& path, Handler handler) {
            add(RouteMethod::Post, path, std::move(handler));
        }

        void post(const std::string& path, SimpleHandler handler) {
            add(RouteMethod::Post, path, std::move(handler));
        }

        void put(const std::string& path, Handler handler) {
            add(RouteMethod::Put, path, std::move(handler));
        }

        void put(const std::string& path, SimpleHandler handler) {
            add(RouteMethod::Put, path, std::move(handler));
        }

        void patch(const std::string& path, Handler handler) {
            add(RouteMethod::Patch, path, std::move(handler));
        }

        void patch(const std::string& path, SimpleHandler handler) {
            add(RouteMethod::Patch, path, std::move(handler));
        }

        void del(const std::string& path, Handler handler) {
            add(RouteMethod::Delete, path, std::move(handler));
        }

        void del(const std::string& path, SimpleHandler handler) {
            add(RouteMethod::Delete, path, std::move(handler));
        }

        void options(const std::string& path, Handler handler) {
            add(RouteMethod::Options, path, std::move(handler));
        }

        void options(const std::string& path, SimpleHandler handler) {
            add(RouteMethod::Options, path, std::move(handler));
        }
};

#endif
