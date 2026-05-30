#ifndef INAPI_AUTH
#define INAPI_AUTH

#pragma once

#include <string>
#include <utility>
#include <error.hpp>
#include <middleware.hpp>
#include <request.hpp>
#include <response.hpp>

inline Response unauthorized(std::string message = "Unauthorized", std::string challenge = "") {
    Response response = error(401, std::move(message));

    if (!challenge.empty()) {
        response.header("WWW-Authenticate", std::move(challenge));
    }

    return response;
}

inline Response forbidden(std::string message = "Forbidden") {
    return error(403, std::move(message));
}

inline AuthHook basic_auth(std::string username, std::string password) {
    return [username = std::move(username), password = std::move(password)](Request request) {
        auto auth = request.basic_auth();
        return auth && auth->username == username && auth->password == password;
    };
}

inline Middleware require_auth(AuthHook hook, std::string challenge = "Bearer") {
    return [hook = std::move(hook), challenge = std::move(challenge)](Request request, Next next) {
        if (!hook(request)) {
            return unauthorized("Unauthorized", challenge);
        }

        return next(request);
    };
}

#endif
