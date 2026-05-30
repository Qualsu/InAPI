#ifndef RESPONSE
#define RESPONSE

#include <algorithm>
#include <cctype>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <utility>
#include <json.hpp>

struct Response {
    Response() = default;

    Response(int status,
             std::string body = "",
             std::string content_type = "text/plain; charset=utf-8",
             std::map<std::string, std::string> headers = {})
        : status(status),
          body(std::move(body)),
          content_type(std::move(content_type)),
          headers(std::move(headers)) {}

    Response(std::string body,
             std::string content_type = "text/plain; charset=utf-8",
             int status = 200)
        : status(status), body(std::move(body)), content_type(std::move(content_type)) {}

    Response(const char* body)
        : body(body) {}

    int status = 200;
    std::string body;
    std::string content_type = "text/plain; charset=utf-8";
    std::map<std::string, std::string> headers;
};

inline Response text(std::string body, int status = 200) {
    return Response(status, std::move(body), "text/plain; charset=utf-8");
}

inline Response html(std::string body, int status = 200) {
    return Response(status, std::move(body), "text/html; charset=utf-8");
}

inline Response json(nlohmann::json body, int status = 200) {
    return Response(status, body.dump(), "application/json");
}

inline Response redirect(std::string url, int status = 302) {
    return Response(status, "", "text/plain; charset=utf-8", {{"Location", std::move(url)}});
}

inline Response status(int code) {
    return Response(code);
}

inline std::string file_content_type(const std::string& path) {
    auto dot = path.find_last_of('.');

    if (dot == std::string::npos) {
        return "application/octet-stream";
    }

    std::string extension = path.substr(dot + 1);
    std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char value) {
        return static_cast<char>(std::tolower(value));
    });

    if (extension == "html" || extension == "htm") return "text/html; charset=utf-8";
    if (extension == "txt") return "text/plain; charset=utf-8";
    if (extension == "css") return "text/css; charset=utf-8";
    if (extension == "js") return "application/javascript";
    if (extension == "json") return "application/json";
    if (extension == "png") return "image/png";
    if (extension == "jpg" || extension == "jpeg") return "image/jpeg";
    if (extension == "gif") return "image/gif";
    if (extension == "svg") return "image/svg+xml";
    if (extension == "webp") return "image/webp";
    if (extension == "ico") return "image/x-icon";
    if (extension == "pdf") return "application/pdf";

    return "application/octet-stream";
}

inline Response file(std::string path, int status = 200) {
    std::ifstream stream(path, std::ios::binary);

    if (!stream) {
        return Response(404, "File not found", "text/plain; charset=utf-8");
    }

    std::ostringstream body;
    body << stream.rdbuf();

    return Response(status, body.str(), file_content_type(path));
}

#endif
