#ifndef INAPI_VALIDATION
#define INAPI_VALIDATION

#include <json.hpp>
#include <algorithm>
#include <cctype>
#include <exception>
#include <functional>
#include <initializer_list>
#include <regex>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

struct ValidationErrorDetail {
    std::string field;
    std::string code;
    std::string message;
};

class ValidationException : public std::exception {
    private:
        std::vector<ValidationErrorDetail> error_details;

    public:
        explicit ValidationException(std::vector<ValidationErrorDetail> details)
            : error_details(std::move(details)) {}

        const char* what() const noexcept override {
            return "Validation failed";
        }

        const std::vector<ValidationErrorDetail>& details() const {
            return error_details;
        }
};

enum class ValidationType {
    Any,
    String,
    Integer,
    Number,
    Boolean,
    Array,
    Object
};

class Field {
    public:
        using CustomCallback = std::function<bool(const nlohmann::json&)>;

        std::string name;
        ValidationType value_type = ValidationType::Any;
        bool is_required = false;
        bool is_nullable = false;
        bool has_default = false;
        nlohmann::json default_json;
        bool has_min_value = false;
        bool has_max_value = false;
        double min_value = 0;
        double max_value = 0;
        bool has_min_length = false;
        bool has_max_length = false;
        size_t min_length = 0;
        size_t max_length = 0;
        std::vector<nlohmann::json> allowed_values;
        bool has_regex_rule = false;
        std::regex regex_rule;
        bool email_rule = false;
        bool url_rule = false;
        bool uuid_rule = false;
        std::vector<Field> children;
        bool has_array_item = false;
        Field* array_item = nullptr;
        std::vector<std::pair<std::string, CustomCallback>> custom_rules;

        Field() = default;

        explicit Field(std::string name)
            : name(std::move(name)) {}

        Field(const Field& other)
            : name(other.name),
              value_type(other.value_type),
              is_required(other.is_required),
              is_nullable(other.is_nullable),
              has_default(other.has_default),
              default_json(other.default_json),
              has_min_value(other.has_min_value),
              has_max_value(other.has_max_value),
              min_value(other.min_value),
              max_value(other.max_value),
              has_min_length(other.has_min_length),
              has_max_length(other.has_max_length),
              min_length(other.min_length),
              max_length(other.max_length),
              allowed_values(other.allowed_values),
              has_regex_rule(other.has_regex_rule),
              regex_rule(other.regex_rule),
              email_rule(other.email_rule),
              url_rule(other.url_rule),
              uuid_rule(other.uuid_rule),
              children(other.children),
              has_array_item(other.has_array_item),
              custom_rules(other.custom_rules) {
            if (other.array_item) {
                array_item = new Field(*other.array_item);
            }
        }

        Field(Field&& other) noexcept
            : name(std::move(other.name)),
              value_type(other.value_type),
              is_required(other.is_required),
              is_nullable(other.is_nullable),
              has_default(other.has_default),
              default_json(std::move(other.default_json)),
              has_min_value(other.has_min_value),
              has_max_value(other.has_max_value),
              min_value(other.min_value),
              max_value(other.max_value),
              has_min_length(other.has_min_length),
              has_max_length(other.has_max_length),
              min_length(other.min_length),
              max_length(other.max_length),
              allowed_values(std::move(other.allowed_values)),
              has_regex_rule(other.has_regex_rule),
              regex_rule(std::move(other.regex_rule)),
              email_rule(other.email_rule),
              url_rule(other.url_rule),
              uuid_rule(other.uuid_rule),
              children(std::move(other.children)),
              has_array_item(other.has_array_item),
              array_item(other.array_item),
              custom_rules(std::move(other.custom_rules)) {
            other.array_item = nullptr;
            other.has_array_item = false;
        }

        ~Field() {
            delete array_item;
        }

        Field& operator=(const Field& other) {
            if (this == &other) {
                return *this;
            }

            Field copy(other);
            *this = std::move(copy);
            return *this;
        }

        Field& operator=(Field&& other) noexcept {
            if (this == &other) {
                return *this;
            }

            delete array_item;
            name = std::move(other.name);
            value_type = other.value_type;
            is_required = other.is_required;
            is_nullable = other.is_nullable;
            has_default = other.has_default;
            default_json = std::move(other.default_json);
            has_min_value = other.has_min_value;
            has_max_value = other.has_max_value;
            min_value = other.min_value;
            max_value = other.max_value;
            has_min_length = other.has_min_length;
            has_max_length = other.has_max_length;
            min_length = other.min_length;
            max_length = other.max_length;
            allowed_values = std::move(other.allowed_values);
            has_regex_rule = other.has_regex_rule;
            regex_rule = std::move(other.regex_rule);
            email_rule = other.email_rule;
            url_rule = other.url_rule;
            uuid_rule = other.uuid_rule;
            children = std::move(other.children);
            has_array_item = other.has_array_item;
            array_item = other.array_item;
            custom_rules = std::move(other.custom_rules);
            other.array_item = nullptr;
            other.has_array_item = false;
            return *this;
        }

        Field& string() {
            value_type = ValidationType::String;
            return *this;
        }

        Field& integer() {
            value_type = ValidationType::Integer;
            return *this;
        }

        Field& number() {
            value_type = ValidationType::Number;
            return *this;
        }

        Field& boolean() {
            value_type = ValidationType::Boolean;
            return *this;
        }

        Field& array() {
            value_type = ValidationType::Array;
            return *this;
        }

        Field& array(Field item) {
            value_type = ValidationType::Array;
            delete array_item;
            array_item = new Field(std::move(item));
            has_array_item = true;
            return *this;
        }

        Field& object(std::initializer_list<Field> schema) {
            value_type = ValidationType::Object;
            children.assign(schema.begin(), schema.end());
            return *this;
        }

        Field& required() {
            is_required = true;
            return *this;
        }

        Field& optional() {
            is_required = false;
            return *this;
        }

        Field& nullable() {
            is_nullable = true;
            return *this;
        }

        template <typename T>
        Field& default_value(T value) {
            has_default = true;
            default_json = value;
            return *this;
        }

        Field& min(double value) {
            has_min_value = true;
            min_value = value;
            return *this;
        }

        Field& max(double value) {
            has_max_value = true;
            max_value = value;
            return *this;
        }

        Field& min_len(size_t value) {
            has_min_length = true;
            min_length = value;
            return *this;
        }

        Field& max_len(size_t value) {
            has_max_length = true;
            max_length = value;
            return *this;
        }

        Field& one_of(std::initializer_list<nlohmann::json> values) {
            allowed_values.assign(values.begin(), values.end());
            return *this;
        }

        Field& regex(const std::string& pattern) {
            has_regex_rule = true;
            regex_rule = std::regex(pattern);
            return *this;
        }

        Field& email() {
            email_rule = true;
            return *this;
        }

        Field& url() {
            url_rule = true;
            return *this;
        }

        Field& uuid() {
            uuid_rule = true;
            return *this;
        }

        Field& custom(std::string message, CustomCallback callback) {
            custom_rules.emplace_back(std::move(message), std::move(callback));
            return *this;
        }
};

class ValidationSchema {
    public:
        std::vector<Field> fields;

        ValidationSchema() = default;

        ValidationSchema(std::initializer_list<Field> fields)
            : fields(fields.begin(), fields.end()) {}
};

inline Field field(std::string name) {
    return Field(std::move(name));
}

inline nlohmann::json validation_error_json(const std::vector<ValidationErrorDetail>& details) {
    nlohmann::json body = {
        {"error", "Validation failed"},
        {"details", nlohmann::json::array()}
    };

    for (const auto& detail : details) {
        body["details"].push_back({
            {"field", detail.field},
            {"code", detail.code},
            {"message", detail.message}
        });
    }

    return body;
}

inline std::string validation_join_path(const std::string& base, const std::string& name) {
    if (base.empty()) {
        return name;
    }

    if (name.empty()) {
        return base;
    }

    return base + "." + name;
}

inline void validation_add_error(std::vector<ValidationErrorDetail>& errors,
                                 const std::string& field,
                                 const std::string& code,
                                 const std::string& message) {
    errors.push_back({field, code, message});
}

inline bool validation_parse_integer(const std::string& raw, long long& result) {
    if (raw.empty()) {
        return false;
    }

    try {
        size_t position = 0;
        long long value = std::stoll(raw, &position);

        if (position != raw.size()) {
            return false;
        }

        result = value;
        return true;
    } catch (...) {
        return false;
    }
}

inline bool validation_parse_number(const std::string& raw, double& result) {
    if (raw.empty()) {
        return false;
    }

    try {
        size_t position = 0;
        double value = std::stod(raw, &position);

        if (position != raw.size()) {
            return false;
        }

        result = value;
        return true;
    } catch (...) {
        return false;
    }
}

inline bool validation_parse_boolean(const std::string& raw, bool& result) {
    std::string value = raw;
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char symbol) {
        return static_cast<char>(std::tolower(symbol));
    });

    if (value == "1" || value == "true" || value == "yes" || value == "on") {
        result = true;
        return true;
    }

    if (value == "0" || value == "false" || value == "no" || value == "off") {
        result = false;
        return true;
    }

    return false;
}

inline nlohmann::json validation_coerce(const Field& field,
                                        const nlohmann::json& value,
                                        const std::string& path,
                                        std::vector<ValidationErrorDetail>& errors,
                                        bool& ok);

inline nlohmann::json validation_validate_object(const std::vector<Field>& fields,
                                                 const nlohmann::json& source,
                                                 const std::string& base_path,
                                                 std::vector<ValidationErrorDetail>& errors) {
    nlohmann::json result = nlohmann::json::object();

    for (const auto& field : fields) {
        std::string path = validation_join_path(base_path, field.name);
        bool has_value = source.is_object() && source.contains(field.name);
        nlohmann::json value;

        if (!has_value) {
            if (field.has_default) {
                result[field.name] = field.default_json;
            } else if (field.is_required) {
                validation_add_error(errors, path, "required", "Field is required");
            }

            continue;
        }

        value = source.at(field.name);

        if (value.is_null()) {
            if (field.is_nullable) {
                result[field.name] = nullptr;
            } else {
                validation_add_error(errors, path, "not_nullable", "Field cannot be null");
            }

            continue;
        }

        bool ok = true;
        nlohmann::json normalized = validation_coerce(field, value, path, errors, ok);

        if (ok) {
            result[field.name] = normalized;
        }
    }

    return result;
}

inline nlohmann::json validation_parse_embedded_json(const nlohmann::json& value) {
    if (!value.is_string()) {
        return value;
    }

    const std::string raw = value.get<std::string>();

    if (raw.empty() || (raw.front() != '[' && raw.front() != '{')) {
        return value;
    }

    try {
        return nlohmann::json::parse(raw);
    } catch (...) {
        return value;
    }
}

inline nlohmann::json validation_coerce(const Field& field,
                                        const nlohmann::json& value,
                                        const std::string& path,
                                        std::vector<ValidationErrorDetail>& errors,
                                        bool& ok) {
    nlohmann::json normalized = value;

    switch (field.value_type) {
        case ValidationType::String:
            if (!value.is_string()) {
                validation_add_error(errors, path, "invalid_type", "Expected string");
                ok = false;
                return nullptr;
            }

            break;

        case ValidationType::Integer:
            if (value.is_number_integer()) {
                normalized = value.get<long long>();
            } else if (value.is_string()) {
                long long parsed = 0;

                if (!validation_parse_integer(value.get<std::string>(), parsed)) {
                    validation_add_error(errors, path, "invalid_type", "Expected integer");
                    ok = false;
                    return nullptr;
                }

                normalized = parsed;
            } else {
                validation_add_error(errors, path, "invalid_type", "Expected integer");
                ok = false;
                return nullptr;
            }

            break;

        case ValidationType::Number:
            if (value.is_number()) {
                normalized = value;
            } else if (value.is_string()) {
                double parsed = 0;

                if (!validation_parse_number(value.get<std::string>(), parsed)) {
                    validation_add_error(errors, path, "invalid_type", "Expected number");
                    ok = false;
                    return nullptr;
                }

                normalized = parsed;
            } else {
                validation_add_error(errors, path, "invalid_type", "Expected number");
                ok = false;
                return nullptr;
            }

            break;

        case ValidationType::Boolean:
            if (value.is_boolean()) {
                normalized = value;
            } else if (value.is_string()) {
                bool parsed = false;

                if (!validation_parse_boolean(value.get<std::string>(), parsed)) {
                    validation_add_error(errors, path, "invalid_type", "Expected boolean");
                    ok = false;
                    return nullptr;
                }

                normalized = parsed;
            } else {
                validation_add_error(errors, path, "invalid_type", "Expected boolean");
                ok = false;
                return nullptr;
            }

            break;

        case ValidationType::Array: {
            normalized = validation_parse_embedded_json(value);

            if (!normalized.is_array()) {
                validation_add_error(errors, path, "invalid_type", "Expected array");
                ok = false;
                return nullptr;
            }

            if (field.has_array_item && field.array_item) {
                nlohmann::json items = nlohmann::json::array();

                for (size_t i = 0; i < normalized.size(); ++i) {
                    bool item_ok = true;
                    std::ostringstream item_path;
                    item_path << path << "[" << i << "]";
                    nlohmann::json item = validation_coerce(*field.array_item, normalized[i], item_path.str(), errors, item_ok);

                    if (item_ok) {
                        items.push_back(item);
                    } else {
                        ok = false;
                    }
                }

                normalized = items;
            }

            break;
        }

        case ValidationType::Object:
            normalized = validation_parse_embedded_json(value);

            if (!normalized.is_object()) {
                validation_add_error(errors, path, "invalid_type", "Expected object");
                ok = false;
                return nullptr;
            }

            {
                size_t before = errors.size();
                normalized = validation_validate_object(field.children, normalized, path, errors);

                if (errors.size() != before) {
                    ok = false;
                }
            }
            break;

        case ValidationType::Any:
            break;
    }

    if (field.has_min_value && normalized.is_number() && normalized.get<double>() < field.min_value) {
        validation_add_error(errors, path, "min", "Value is too small");
        ok = false;
    }

    if (field.has_max_value && normalized.is_number() && normalized.get<double>() > field.max_value) {
        validation_add_error(errors, path, "max", "Value is too large");
        ok = false;
    }

    if (field.has_min_length && ((normalized.is_string() && normalized.get<std::string>().size() < field.min_length) ||
        (normalized.is_array() && normalized.size() < field.min_length))) {
        validation_add_error(errors, path, "min_len", "Value is too short");
        ok = false;
    }

    if (field.has_max_length && ((normalized.is_string() && normalized.get<std::string>().size() > field.max_length) ||
        (normalized.is_array() && normalized.size() > field.max_length))) {
        validation_add_error(errors, path, "max_len", "Value is too long");
        ok = false;
    }

    if (!field.allowed_values.empty()) {
        bool found = false;

        for (const auto& allowed : field.allowed_values) {
            if (normalized == allowed) {
                found = true;
                break;
            }
        }

        if (!found) {
            validation_add_error(errors, path, "one_of", "Value is not allowed");
            ok = false;
        }
    }

    if (field.has_regex_rule && (!normalized.is_string() || !std::regex_match(normalized.get<std::string>(), field.regex_rule))) {
        validation_add_error(errors, path, "invalid_format", "Invalid format");
        ok = false;
    }

    if (field.email_rule) {
        static const std::regex email_pattern(R"(^[^@\s]+@[^@\s]+\.[^@\s]+$)");

        if (!normalized.is_string() || !std::regex_match(normalized.get<std::string>(), email_pattern)) {
            validation_add_error(errors, path, "invalid_email", "Invalid email");
            ok = false;
        }
    }

    if (field.url_rule) {
        static const std::regex url_pattern(R"(^(https?)://[^\s/$.?#].[^\s]*$)", std::regex::icase);

        if (!normalized.is_string() || !std::regex_match(normalized.get<std::string>(), url_pattern)) {
            validation_add_error(errors, path, "invalid_url", "Invalid url");
            ok = false;
        }
    }

    if (field.uuid_rule) {
        static const std::regex uuid_pattern(R"(^[0-9a-f]{8}-[0-9a-f]{4}-[1-5][0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-f]{12}$)", std::regex::icase);

        if (!normalized.is_string() || !std::regex_match(normalized.get<std::string>(), uuid_pattern)) {
            validation_add_error(errors, path, "invalid_uuid", "Invalid uuid");
            ok = false;
        }
    }

    for (const auto& rule : field.custom_rules) {
        bool passed = false;

        try {
            passed = rule.second(normalized);
        } catch (...) {
            passed = false;
        }

        if (!passed) {
            validation_add_error(errors, path, "custom", rule.first);
            ok = false;
        }
    }

    return normalized;
}

inline nlohmann::json validate_schema(const nlohmann::json& source, const ValidationSchema& schema) {
    std::vector<ValidationErrorDetail> errors;

    if (!source.is_object()) {
        validation_add_error(errors, "", "invalid_type", "Expected object");
    }

    nlohmann::json normalized = validation_validate_object(schema.fields, source.is_object() ? source : nlohmann::json::object(), "", errors);

    if (!errors.empty()) {
        throw ValidationException(std::move(errors));
    }

    return normalized;
}

#endif
