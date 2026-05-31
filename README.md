<img src="logo.png" width="300px">

# InAPI

![C++](https://img.shields.io/badge/c++-%2300599C.svg?style=for-the-badge&logo=c%2B%2B&logoColor=white)

**InAPI** is a C++ library for building HTTP servers with FastAPI-like syntax.

The project uses `cpp-httplib` and `nlohmann/json`.

### [English documentation](docs/EN.md) / [Russian documentation](docs/RU.md)

## Build

By default, the `makefile` builds `src/main.cpp`. Pass `SRC` to build another file:

```sh
make compile SRC=src/app.cpp
```

```sh
make SRC=src/app.cpp
```

## Application Example

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
