#ifndef RESPONSE
#define RESPONSE

#include <algorithm>
#include <cctype>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#include <encoding.hpp>
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
    std::vector<std::string> cookie_headers;

    Response& header(const std::string& name, std::string value) {
        headers[name] = std::move(value);
        return *this;
    }

    Response& set_cookie(const std::string& name,
                         std::string value,
                         const std::string& path = "/",
                         int max_age = -1,
                         bool http_only = true,
                         bool secure = false,
                         const std::string& same_site = "Lax") {
        std::ostringstream cookie;
        cookie << safe_cookie_part(name) << "=" << safe_cookie_value(std::move(value));

        if (!path.empty()) {
            cookie << "; Path=" << safe_cookie_value(path);
        }

        if (max_age >= 0) {
            cookie << "; Max-Age=" << max_age;
        }

        if (http_only) {
            cookie << "; HttpOnly";
        }

        if (secure) {
            cookie << "; Secure";
        }

        if (!same_site.empty()) {
            cookie << "; SameSite=" << safe_cookie_value(same_site);
        }

        cookie_headers.push_back(cookie.str());
        return *this;
    }

    Response& delete_cookie(const std::string& name, const std::string& path = "/") {
        std::ostringstream cookie;
        cookie << safe_cookie_part(name) << "=; Path=" << safe_cookie_value(path)
               << "; Max-Age=0; Expires=Thu, 01 Jan 1970 00:00:00 GMT";

        cookie_headers.push_back(cookie.str());
        return *this;
    }

    private:
        static std::string safe_cookie_part(std::string value) {
            value.erase(std::remove_if(value.begin(), value.end(), [](unsigned char symbol) {
                return symbol <= 32 || symbol == 127 || symbol == ';' || symbol == ',' || symbol == '=';
            }), value.end());

            return value;
        }

        static std::string safe_cookie_value(std::string value) {
            value.erase(std::remove_if(value.begin(), value.end(), [](unsigned char symbol) {
                return symbol == '\r' || symbol == '\n' || symbol == ';';
            }), value.end());

            return value;
        }
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
    if (extension == "mjs") return "application/javascript";
    if (extension == "json") return "application/json";
    if (extension == "xml") return "application/xml";
    if (extension == "png") return "image/png";
    if (extension == "jpg" || extension == "jpeg") return "image/jpeg";
    if (extension == "gif") return "image/gif";
    if (extension == "svg") return "image/svg+xml";
    if (extension == "webp") return "image/webp";
    if (extension == "avif") return "image/avif";
    if (extension == "ico") return "image/x-icon";
    if (extension == "pdf") return "application/pdf";
    if (extension == "wasm") return "application/wasm";
    if (extension == "woff") return "font/woff";
    if (extension == "woff2") return "font/woff2";
    if (extension == "ttf") return "font/ttf";
    if (extension == "otf") return "font/otf";
    if (extension == "mp4") return "video/mp4";
    if (extension == "webm") return "video/webm";
    if (extension == "mp3") return "audio/mpeg";
    if (extension == "wav") return "audio/wav";

    return "application/octet-stream";
}

inline Response file(std::string path, int status = 200) {
    std::ifstream stream(InAPIEncoding::path_from_utf8(path), std::ios::binary);

    if (!stream) {
        return Response(404, "File not found", "text/plain; charset=utf-8");
    }

    std::ostringstream body;
    body << stream.rdbuf();

    return Response(status, body.str(), file_content_type(path));
}

#endif
