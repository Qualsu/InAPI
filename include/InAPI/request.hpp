#ifndef INAPI_REQUEST
#define INAPI_REQUEST

#include <httplib.h>
#include <json.hpp>
#include <algorithm>
#include <cctype>
#include <string>
#include <unordered_map>
#include <utility>

class Request {
    private:
        const httplib::Request& request;
        std::unordered_map<std::string, std::string> route_params;

        static bool parse_int(const std::string& value, int& result) {
            if (value.empty()) {
                return false;
            }

            try {
                size_t position = 0;
                int parsed = std::stoi(value, &position);

                if (position != value.size()) {
                    return false;
                }

                result = parsed;
                return true;
            } catch (...) {
                return false;
            }
        }

        static bool parse_bool(const std::string& value, bool default_value) {
            std::string normalized = value;
            std::transform(normalized.begin(), normalized.end(), normalized.begin(), [](unsigned char symbol) {
                return static_cast<char>(std::tolower(symbol));
            });

            if (normalized == "1" || normalized == "true" || normalized == "yes" || normalized == "on") {
                return true;
            }

            if (normalized == "0" || normalized == "false" || normalized == "no" || normalized == "off") {
                return false;
            }

            return default_value;
        }

    public:
        explicit Request(const httplib::Request& request)
            : request(request) {}

        Request(const httplib::Request& request, std::unordered_map<std::string, std::string> route_params)
            : request(request), route_params(std::move(route_params)) {}

        std::string method() const {
            return request.method;
        }

        std::string path() const {
            return request.path;
        }

        std::string body() const {
            return request.body;
        }

        nlohmann::json json() const {
            return nlohmann::json::parse(request.body);
        }

        std::string header(const std::string& name) const {
            return request.get_header_value(name);
        }

        std::string query(const std::string& name) const {
            return request.get_param_value(name);
        }

        std::string query_or(const std::string& name, const std::string& default_value) const {
            if (!request.has_param(name)) {
                return default_value;
            }

            return request.get_param_value(name);
        }

        int query_int(const std::string& name, int default_value = 0) const {
            int result = default_value;

            if (!request.has_param(name) || !parse_int(request.get_param_value(name), result)) {
                return default_value;
            }

            return result;
        }

        bool query_bool(const std::string& name, bool default_value = false) const {
            if (!request.has_param(name)) {
                return default_value;
            }

            return parse_bool(request.get_param_value(name), default_value);
        }

        bool has_query(const std::string& name) const {
            return request.has_param(name);
        }

        std::string param(const std::string& name) const {
            auto route_found = route_params.find(name);

            if (route_found != route_params.end()) {
                return route_found->second;
            }

            auto found = request.path_params.find(name);

            if (found == request.path_params.end()) {
                return "";
            }

            return found->second;
        }

        int param_int(const std::string& name) const {
            int result = 0;
            parse_int(param(name), result);
            return result;
        }

        bool has_param(const std::string& name) const {
            return route_params.find(name) != route_params.end() || request.path_params.find(name) != request.path_params.end();
        }

        bool has_header(const std::string& name) const {
            return request.has_header(name);
        }

        std::string ip() const {
            return request.remote_addr;
        }
};

#endif
