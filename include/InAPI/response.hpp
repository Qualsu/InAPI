#ifndef RESPONSE
#define RESPONSE
#include <string>
#include <utility>

struct Response {
    Response() = default;

    Response(std::string body,
             std::string content_type = "text/plain; charset=utf-8",
             int status = 200)
        : body(std::move(body)), content_type(std::move(content_type)), status(status) {}

    Response(const char* body)
        : body(body) {}

    std::string body;
    std::string content_type = "text/plain; charset=utf-8";
    int status = 200;
};

#endif
