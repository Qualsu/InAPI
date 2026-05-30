#ifndef APP
#define APP

#pragma once

#include <httplib.h>
#include <functional>
#include <string>
#include <response.hpp>
#include <json.hpp>

class App {
    private:
        httplib::Server server;

    public:
        using Handler = std::function<Response()>;

        void get(const std::string& path, Handler handler) {
            server.Get(path, [handler](const httplib::Request&, httplib::Response& res) {
                Response response = handler();

                res.status = response.status;
                res.set_content(response.body, response.content_type);
            });
        }

        void run(int port, const std::string& host = "0.0.0.0") {
            server.listen(host, port);
        }
};

#endif