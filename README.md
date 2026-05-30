<img src="logo.png" width="300px">

# InAPI

![C++](https://img.shields.io/badge/c++-%2300599C.svg?style=for-the-badge&logo=c%2B%2B&logoColor=white)

**InAPI** - C++ библиотека для создания HTTP серверов с FastAPI-подобным синтаксисом

Проект использует `cpp-httplib` и `nlohmann/json`

[Документация](DOCUMENTATION.md)

## Пример сервера на InAPI

```cpp
#include <InAPI.hpp>

int main() {
    App app;

    app.get("/", []() {
        return text("InAPI 2026 by Qualsu");
    });

    app.get("/users/{id:int}", [](Request request) {
        return json({
            {"id", request.param_int("id")},
            {"name", "Alex"}
        });
    });

    app.post("/users", [](Request request) {
        Json body = request.json();

        return json({
            {"created", true},
            {"name", body.value("name", "")}
        }, 201);
    });

    app.error_handler(404, [](Request) {
        return error(404, "Route not found");
    });

    app.run(8080);
}
```

## То же самое на Crow

```cpp
#include <crow.h>

int main() {
    crow::SimpleApp app;

    CROW_ROUTE(app, "/")([]() {
        return crow::response("Crow works");
    });

    CROW_ROUTE(app, "/users/<int>")([](int id) {
        crow::json::wvalue body;
        body["id"] = id;
        body["name"] = "Alex";

        crow::response response(body);
        response.set_header("Content-Type", "application/json");
        return response;
    });

    CROW_ROUTE(app, "/users").methods(crow::HTTPMethod::Post)([](const crow::request& request) {
        auto body = crow::json::load(request.body);

        if (!body) {
            return crow::response(400, "{\"error\":\"Bad request\"}");
        }

        crow::json::wvalue result;
        result["created"] = true;
        result["name"] = body["name"].s();

        crow::response response(result);
        response.code = 201;
        response.set_header("Content-Type", "application/json");
        return response;
    });

    app.port(8080).run();
}
```