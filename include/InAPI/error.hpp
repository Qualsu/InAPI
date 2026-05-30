#ifndef INAPI_ERROR
#define INAPI_ERROR

#include <string>
#include <utility>
#include <json.hpp>
#include <response.hpp>

inline std::string error_message(int status) {
    switch (status) {
        case 400: return "Bad request";
        case 401: return "Unauthorized";
        case 403: return "Forbidden";
        case 404: return "Not found";
        case 405: return "Method not allowed";
        case 409: return "Conflict";
        case 413: return "Payload too large";
        case 422: return "Unprocessable entity";
        case 500: return "Internal server error";
        case 502: return "Bad gateway";
        case 503: return "Service unavailable";
        default: return "Error";
    }
}

inline Response error(int status, std::string message) {
    return json({
        {"error", std::move(message)}
    }, status);
}

inline Response error(int status) {
    return error(status, error_message(status));
}

#endif
