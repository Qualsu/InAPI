#ifndef INAPI_CONFIG
#define INAPI_CONFIG

#include <cctype>
#include <cstddef>
#include <optional>
#include <string>
#include <thread>
#include <utility>

inline int auto_threads() {
    unsigned int threads = std::thread::hardware_concurrency();
    return threads > 0 ? static_cast<int>(threads) : 1;
}

struct SSL {
    std::string cert_file;
    std::string key_file;

    SSL(std::string cert_file, std::string key_file)
        : cert_file(std::move(cert_file)),
          key_file(std::move(key_file)) {}
};

struct Config {
    bool logger;
    int threads;
    std::string max_body_size;
    int read_timeout_seconds;
    int write_timeout_seconds;
    int idle_timeout_seconds;
    std::optional<SSL> ssl;

    explicit Config(bool logger = true,
                    int threads = auto_threads(),
                    std::string max_body_size = "1mb",
                    int read_timeout_seconds = 5,
                    int write_timeout_seconds = 10,
                    int idle_timeout_seconds = 30,
                    std::optional<SSL> ssl = std::nullopt)
        : logger(logger),
          threads(threads > 0 ? threads : auto_threads()),
          max_body_size(std::move(max_body_size)),
          read_timeout_seconds(read_timeout_seconds > 0 ? read_timeout_seconds : 5),
          write_timeout_seconds(write_timeout_seconds > 0 ? write_timeout_seconds : 10),
          idle_timeout_seconds(idle_timeout_seconds > 0 ? idle_timeout_seconds : 30),
          ssl(std::move(ssl)) {}

    explicit Config(int threads,
                    std::string max_body_size = "1mb",
                    int read_timeout_seconds = 5,
                    int write_timeout_seconds = 10,
                    int idle_timeout_seconds = 30,
                    std::optional<SSL> ssl = std::nullopt)
        : Config(true,
                 threads,
                 std::move(max_body_size),
                 read_timeout_seconds,
                 write_timeout_seconds,
                 idle_timeout_seconds,
                 std::move(ssl)) {}
};

inline std::size_t max_body_size_bytes(std::string value) {
    for (char& symbol : value) {
        symbol = static_cast<char>(std::tolower(static_cast<unsigned char>(symbol)));
    }

    if (value == "512kb") {
        return 512ull * 1024ull;
    }

    if (value == "1mb") {
        return 1ull * 1024ull * 1024ull;
    }

    if (value == "10mb") {
        return 10ull * 1024ull * 1024ull;
    }

    if (value == "1gb") {
        return 1ull * 1024ull * 1024ull * 1024ull;
    }

    return 1ull * 1024ull * 1024ull;
}

#endif
