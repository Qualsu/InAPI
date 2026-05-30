#ifndef INAPI_REQUEST
#define INAPI_REQUEST

#include <httplib.h>
#include <json.hpp>
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <system_error>
#include <unordered_map>
#include <utility>
#include <vector>

class UploadedFile {
    public:
        std::string name;
        std::string filename;
        std::string content_type;
        std::string content;
        httplib::Headers headers;

        UploadedFile() = default;

        explicit UploadedFile(httplib::FormData file)
            : name(std::move(file.name)),
              filename(std::move(file.filename)),
              content_type(std::move(file.content_type)),
              content(std::move(file.content)),
              headers(std::move(file.headers)) {}

        size_t size() const {
            return content.size();
        }

        bool empty() const {
            return name.empty() && filename.empty() && content.empty();
        }

        bool save(const std::string& path) const {
            std::filesystem::path target(path);
            std::filesystem::path parent = target.parent_path();

            if (!parent.empty()) {
                std::error_code error;
                std::filesystem::create_directories(parent, error);

                if (error) {
                    return false;
                }
            }

            std::ofstream stream(target, std::ios::binary);

            if (!stream) {
                return false;
            }

            stream.write(content.data(), static_cast<std::streamsize>(content.size()));
            return stream.good();
        }
};

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

        static std::string trim(std::string value) {
            while (!value.empty() && std::isspace(static_cast<unsigned char>(value.front()))) {
                value.erase(value.begin());
            }

            while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back()))) {
                value.pop_back();
            }

            return value;
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

        std::string bearer_token() const {
            std::string authorization = header("Authorization");

            if (authorization.size() < 7) {
                return "";
            }

            std::string scheme = authorization.substr(0, 6);
            std::transform(scheme.begin(), scheme.end(), scheme.begin(), [](unsigned char symbol) {
                return static_cast<char>(std::tolower(symbol));
            });

            if (scheme != "bearer" || !std::isspace(static_cast<unsigned char>(authorization[6]))) {
                return "";
            }

            return trim(authorization.substr(7));
        }

        std::string content_type() const {
            return header("Content-Type");
        }

        std::string user_agent() const {
            return header("User-Agent");
        }

        std::string cookie(const std::string& name) const {
            std::string cookies = header("Cookie");
            size_t position = 0;

            while (position < cookies.size()) {
                size_t semicolon = cookies.find(';', position);
                std::string item = cookies.substr(position, semicolon == std::string::npos ? std::string::npos : semicolon - position);
                size_t separator = item.find('=');

                if (separator != std::string::npos && trim(item.substr(0, separator)) == name) {
                    return trim(item.substr(separator + 1));
                }

                if (semicolon == std::string::npos) {
                    break;
                }

                position = semicolon + 1;
            }

            return "";
        }

        bool has_cookie(const std::string& name) const {
            std::string cookies = header("Cookie");
            size_t position = 0;

            while (position < cookies.size()) {
                size_t semicolon = cookies.find(';', position);
                std::string item = cookies.substr(position, semicolon == std::string::npos ? std::string::npos : semicolon - position);
                size_t separator = item.find('=');

                if (separator != std::string::npos && trim(item.substr(0, separator)) == name) {
                    return true;
                }

                if (semicolon == std::string::npos) {
                    break;
                }

                position = semicolon + 1;
            }

            return false;
        }

        std::string query(const std::string& name) const {
            return request.get_param_value(name);
        }

        std::string form(const std::string& name) const {
            if (request.form.has_field(name)) {
                return request.form.get_field(name);
            }

            return request.get_param_value(name);
        }

        std::string form_or(const std::string& name, const std::string& default_value) const {
            if (request.form.has_field(name)) {
                return request.form.get_field(name);
            }

            if (!request.has_param(name)) {
                return default_value;
            }

            return request.get_param_value(name);
        }

        bool has_form(const std::string& name) const {
            return request.form.has_field(name) || request.has_param(name);
        }

        bool has_file(const std::string& name) const {
            return request.form.has_file(name);
        }

        UploadedFile file(const std::string& name) const {
            return UploadedFile(request.form.get_file(name));
        }

        std::vector<UploadedFile> files() const {
            std::vector<UploadedFile> result;
            result.reserve(request.form.files.size());

            for (const auto& item : request.form.files) {
                result.emplace_back(item.second);
            }

            return result;
        }

        std::vector<UploadedFile> files(const std::string& name) const {
            std::vector<UploadedFile> result;
            auto range = request.form.files.equal_range(name);
            result.reserve(static_cast<size_t>(std::distance(range.first, range.second)));

            for (auto item = range.first; item != range.second; ++item) {
                result.emplace_back(item->second);
            }

            return result;
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
