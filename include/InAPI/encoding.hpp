#ifndef INAPI_ENCODING
#define INAPI_ENCODING

#include <clocale>
#include <filesystem>
#include <iostream>
#include <limits>
#include <mutex>
#include <string>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>
#endif

class InAPIEncoding {
    public:
        static void setup_utf8_console() {
            static std::once_flag configured;

            std::call_once(configured, [] {
                std::setlocale(LC_CTYPE, "");
#ifdef _WIN32
                SetConsoleCP(CP_UTF8);
                SetConsoleOutputCP(CP_UTF8);
                enable_virtual_terminal(GetStdHandle(STD_OUTPUT_HANDLE));
                enable_virtual_terminal(GetStdHandle(STD_ERROR_HANDLE));
#endif
            });
        }

        static std::filesystem::path path_from_utf8(const std::string& value) {
#ifdef _WIN32
            std::wstring wide = utf8_to_wide(value);

            if (wide.empty() && !value.empty()) {
                return std::filesystem::path(value);
            }

            return std::filesystem::path(wide);
#else
            return std::filesystem::path(value);
#endif
        }

        static void write_stdout(const std::string& value) {
#ifdef _WIN32
            if (write_console(GetStdHandle(STD_OUTPUT_HANDLE), value)) {
                return;
            }
#endif

            std::cout << value;
        }

    private:
#ifdef _WIN32
        static std::wstring utf8_to_wide(const std::string& value) {
            if (value.empty() || value.size() > static_cast<size_t>((std::numeric_limits<int>::max)())) {
                return L"";
            }

            int length = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(), static_cast<int>(value.size()), nullptr, 0);

            if (length <= 0) {
                return L"";
            }

            std::wstring wide(static_cast<size_t>(length), L'\0');
            MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(), static_cast<int>(value.size()), wide.data(), length);

            return wide;
        }

        static bool write_console(HANDLE handle, const std::string& value) {
            if (handle == INVALID_HANDLE_VALUE || handle == nullptr) {
                return false;
            }

            DWORD mode = 0;

            if (!GetConsoleMode(handle, &mode)) {
                return false;
            }

            std::wstring wide = utf8_to_wide(value);

            if (wide.empty() && !value.empty()) {
                return false;
            }

            DWORD written = 0;
            return WriteConsoleW(handle, wide.data(), static_cast<DWORD>(wide.size()), &written, nullptr) != 0;
        }

        static void enable_virtual_terminal(HANDLE handle) {
            if (handle == INVALID_HANDLE_VALUE || handle == nullptr) {
                return;
            }

            DWORD mode = 0;

            if (!GetConsoleMode(handle, &mode)) {
                return;
            }

            SetConsoleMode(handle, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
        }
#endif
};

#endif
