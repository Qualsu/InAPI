#ifndef INAPI_MIDDLEWARE
#define INAPI_MIDDLEWARE

#include <functional>
#include <string>
#include <utility>
#include <vector>
#include <request.hpp>
#include <response.hpp>

using Next = std::function<Response(Request)>;
using Middleware = std::function<Response(Request, Next)>;
using AuthHook = std::function<bool(Request)>;

struct CorsOptions {
    std::vector<std::string> origins = {"*"};
    std::vector<std::string> methods = {"GET", "POST", "PUT", "PATCH", "DELETE", "OPTIONS"};
    std::vector<std::string> headers = {"Content-Type", "Authorization"};

    CorsOptions() = default;

    CorsOptions(std::vector<std::string> origins,
                std::vector<std::string> methods,
                std::vector<std::string> headers)
        : origins(std::move(origins)),
          methods(std::move(methods)),
          headers(std::move(headers)) {}
};

#endif
