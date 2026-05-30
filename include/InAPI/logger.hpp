#ifndef INAPI_LOGGER
#define INAPI_LOGGER

#include <iostream>
#include <sstream>
#include <string>
#include <encoding.hpp>
#include <request.hpp>
#include <response.hpp>

class InAPILogger {
    private:
        static const char* color_reset() {
            return "\033[0m";
        }

        static const char* color_info() {
            return "\033[32m";
        }

        static const char* color_request() {
            return "\033[36m";
        }

        static const char* color_status(int status) {
            if (status >= 500) {
                return "\033[31m";
            }

            if (status >= 400) {
                return "\033[33m";
            }

            if (status >= 300) {
                return "\033[32m";
            }

            if (status >= 200) {
                return "\033[32m";
            }

            return "\033[0m";
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
                case 413: return "Payload Too Large";
                case 422: return "Unprocessable Entity";
                case 500: return "Internal Server Error";
                case 502: return "Bad Gateway";
                case 503: return "Service Unavailable";
                default: return "";
            }
        }

    public:
        static void info(const std::string& message) {
            InAPIEncoding::setup_utf8_console();
            std::ostringstream log;
            log << color_info() << "INFO" << color_reset()
                << ":     " << message << "\n";
            InAPIEncoding::write_stdout(log.str());
        }

        static void startup(const std::string& host, int port, const std::string& bound_host = "") {
            std::ostringstream message;
            message << "InAPI running on http://" << host << ":" << port;

            if (!bound_host.empty() && bound_host != host) {
                message << " (bound to " << bound_host << ")";
            }

            message << " (Press CTRL+C to quit)";
            info(message.str());
        }

        static void request(const Request& request, const Response& response) {
            InAPIEncoding::setup_utf8_console();
            std::string version = request.http_version().empty() ? "HTTP/1.1" : request.http_version();
            std::ostringstream log;

            log << color_info() << "INFO" << color_reset()
                << ":     " << request.ip();

            if (request.port() >= 0) {
                log << ":" << request.port();
            }

            log << " - \"" << color_request()
                << request.method() << " " << request.path() << " " << version
                << color_reset() << "\" "
                << color_status(response.status) << response.status;

            std::string reason = reason_phrase(response.status);

            if (!reason.empty()) {
                log << " " << reason;
            }

            log << color_reset() << "\n";
            InAPIEncoding::write_stdout(log.str());
        }
};

#endif
