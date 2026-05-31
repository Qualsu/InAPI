# InAPI Documentation

**InAPI** is a C++ library for building HTTP app with FastAPI-like syntax

Internally `cpp-httplib` and `nlohmann/json` are used

## Navigation

- [Quick Start](#quick-start)
- [Build](#build)
- [Include](#include)
- [App](#app)
- [Routes](#routes)
- [Request](#request)
- [Response](#response)
- [JSON](#json)
- [Path Parameters](#path-parameters)
- [Query Parameters](#query-parameters)
- [Headers and Cookies](#headers-and-cookies)
- [Forms and File Uploads](#forms-and-file-uploads)
- [Middleware](#middleware)
- [Router](#router)
- [CORS](#cors)
- [Authorization](#authorization)
- [Validation](#validation)
- [Error Handling](#error-handling)
- [Static Files](#static-files)
- [OpenAPI and Swagger](#openapi-and-swagger)
- [Config](#config)
- [SSL](#ssl)
- [Logging](#logging)
- [Full Example](#full-example)
- [Cheat Sheet](#cheat-sheet)

## Quick Start

```cpp
#include <InAPI.hpp>

int main() {
    App app;

    app.get("/", [] {
        return text("Hello from InAPI");
    });

    app.run(8080);
}
```

## Build

The repository includes a `makefile` for building the example from `src/main.cpp`. It uses `g++` and places the result in the `build` directory.

Regular build without running:

```sh
make compile
```

Build and run the regular example:

```sh
make
```

Build a custom source file instead of `src/main.cpp`:

```sh
make compile SRC=src/app.cpp
```

Build and run a custom source file:

```sh
make SRC=src/app.cpp
```

Build with OpenSSL without running:

```sh
make compile_ssl
```

Build and run the SSL variant:

```sh
make ssl
```

Run an already built regular binary:

```sh
make run
```

Clean built binaries:

```sh
make clean
```

**Build requirements:**

- a compiler with C++17 support or newer, such as `g++`;
- `make`;
- for SSL builds, installed OpenSSL headers and libraries (`ssl` and `crypto`).

If your compiler has a different name, pass it through the `CXX` variable:

```sh
make compile CXX=clang++
```

## Include

Include the main header:

```cpp
#include <InAPI.hpp>
```

If you include headers manually, the files are located in `include/InAPI`.

## App

`App` is the main application class. It is used to add routes, middleware, CORS, authorization, error handlers, static files, and Swagger.

```cpp
App app;
```

Main methods:

| Method | Purpose |
| --- | --- |
| `get(path, handler)` | Registers a GET route |
| `post(path, handler)` | Registers a POST route |
| `put(path, handler)` | Registers a PUT route |
| `patch(path, handler)` | Registers a PATCH route |
| `del(path, handler)` | Registers a DELETE route |
| `options(path, handler)` | Registers an OPTIONS route |
| `middleware(handler)` | Adds global middleware |
| `include(prefix, router)` | Includes a `Router` with a prefix |
| `Cors(options)` | Enables CORS |
| `BearerAuth(token)` | Enables Bearer authorization by token |
| `BearerAuth(hook)` | Enables Bearer authorization through a function |
| `error_handler(status, handler)` | Adds an HTTP error handler |
| `exception_handler(handler)` | Adds an exception handler |
| `mount(path, directory)` | Serves static files from a directory |
| `run(...)` | Starts the server |

## Routes

A handler can accept `Request`:

```cpp
app.get("/hello", [](Request request) {
    return text("Path: " + request.path());
});
```

If request data is not needed, the handler can have no arguments:

```cpp
app.get("/", []() {
    return text("Home");
});
```

Supported HTTP methods:

```cpp
app.get("/items", handler);
app.post("/items", handler);
app.put("/items/{id:int}", handler);
app.patch("/items/{id:int}", handler);
app.del("/items/{id:int}", handler);
app.options("/items", handler);
```

Every handler must return `Response`.

## Request

`Request` contains the incoming HTTP request data.

Main methods:

| Method | Returns |
| --- | --- |
| `method()` | HTTP method |
| `path()` | Request path |
| `body()` | Request body as a string |
| `json()` | Request body as `nlohmann::json` |
| `body(schema)` | JSON body after validation |
| `header(name)` | Header value |
| `has_header(name)` | Whether the header exists |
| `query(name)` | Query parameter |
| `query_or(name, default_value)` | Query parameter or default value |
| `query_int(name, default_value)` | Query parameter as `int` |
| `query_bool(name, default_value)` | Query parameter as `bool` |
| `has_query(name)` | Whether the query parameter exists |
| `query(schema)` | Query parameters after validation |
| `param(name)` | Path parameter |
| `param_int(name)` | Path parameter as `int` |
| `has_param(name)` | Whether the path parameter exists |
| `params(schema)` | Path parameters after validation |
| `cookie(name)` | Cookie |
| `has_cookie(name)` | Whether the cookie exists |
| `form(name)` | Form field |
| `form_or(name, default_value)` | Form field or default value |
| `has_form(name)` | Whether the form field exists |
| `has_file(name)` | Whether a file exists |
| `file(name)` | One uploaded file |
| `files()` | All uploaded files |
| `files(name)` | All files with the specified field name |
| `bearer_token()` | Bearer token from `Authorization` |
| `basic_auth()` | Basic Auth data |
| `content_type()` | `Content-Type` |
| `user_agent()` | `User-Agent` |
| `ip()` | Client IP |
| `port()` | Client port |
| `http_version()` | HTTP version |

## Response

`Response` describes the server response.

```cpp
return Response(200, "OK", "text/plain; charset=utf-8");
```

It is usually more convenient to use helper functions:

| Helper | Purpose |
| --- | --- |
| `text(body, status = 200)` | Text response |
| `html(body, status = 200)` | HTML response |
| `json(body, status = 200)` | JSON response |
| `redirect(url, status = 302)` | Redirect with the `Location` header |
| `status(code)` | Empty response with the specified status |
| `file(path, status = 200)` | File response |
| `error(status)` | JSON error by status |
| `error(status, message)` | JSON error with text |

Examples:

```cpp
app.get("/text", []() {
    return text("Hello");
});

app.get("/html", []() {
    return html("<h1>Hello</h1>");
});

app.get("/json", []() {
    return json({{"ok", true}});
});

app.get("/redirect", []() {
    return redirect("/json");
});

app.get("/empty", []() {
    return status(204);
});
```

### Response Headers

```cpp
app.get("/headers", []() {
    Response response = json({{"ok", true}});
    response.header("X-App", "InAPI");
    return response;
});
```

### Response Cookies

```cpp
app.get("/login", []() {
    Response response = json({{"logged", true}});
    response.set_cookie("token", "abc123");
    return response;
});

app.get("/logout", []() {
    Response response = redirect("/");
    response.delete_cookie("token");
    return response;
});
```

`set_cookie` accepts these arguments:

```cpp
set_cookie(name, value, path, max_age, http_only, secure, same_site)
```

Default values:

- `path = "/"`
- `max_age = -1`
- `http_only = true`
- `secure = false`
- `same_site = "Lax"`

## JSON

InAPI uses `nlohmann::json`.

```cpp
app.post("/echo", [](Request request) {
    Json body = request.json();

    return json({
        {"you_sent", body}
    });
});
```

If the request body contains invalid JSON, `request.json()` throws `nlohmann::json::parse_error`.

Use `request.body(schema)` for an automatic `422` response.

## Path Parameters

Path parameters are written in curly braces:

```cpp
app.get("/users/{id}", [](Request request) {
    return text(request.param("id"));
});
```

Supported types:

| Syntax | Accepts |
| --- | --- |
| `{name}` | Any text up to `/` |
| `{name:int}` | Integer, including a negative one |
| `{name:path}` | The rest of the path, including `/` |

Examples:

```cpp
app.get("/users/{id:int}", [](Request request) {
    int id = request.param_int("id");
    return json({{"id", id}});
});

app.get("/files/{path:path}", [](Request request) {
    return json({{"path", request.param("path")}});
});
```

## Query Parameters

```cpp
app.get("/search", [](Request request) {
    std::string q = request.query_or("q", "");
    int page = request.query_int("page", 1);
    bool debug = request.query_bool("debug", false);

    return json({
        {"q", q},
        {"page", page},
        {"debug", debug}
    });
});
```

`query_bool` accepts these values:

- `true`: `1`, `true`, `yes`, `on`
- `false`: `0`, `false`, `no`, `off`

## Headers and Cookies

```cpp
app.get("/client", [](Request request) {
    return json({
        {"user_agent", request.user_agent()},
        {"content_type", request.content_type()},
        {"session", request.cookie("session")},
        {"has_session", request.has_cookie("session")}
    });
});
```

Bearer token:

```cpp
app.get("/token", [](Request request) {
    auto token = request.bearer_token();

    if (!token) {
        return error(401);
    }

    return json({{"token", *token}});
});
```

Basic Auth:

```cpp
app.get("/basic", [](Request request) {
    auto auth = request.basic_auth();

    if (!auth) {
        return error(401);
    }

    return json({
        {"username", auth->username}
    });
});
```

## Forms and File Uploads

Form fields:

```cpp
app.post("/form", [](Request request) {
    std::string name = request.form_or("name", "anonymous");

    return json({
        {"name", name}
    });
});
```

One file:

```cpp
app.post("/upload", [](Request request) {
    if (!request.has_file("file")) {
        return error(400, "File is required");
    }

    UploadedFile uploaded = request.file("file");
    uploaded.save("uploads/" + uploaded.filename);

    return json({
        {"name", uploaded.name},
        {"filename", uploaded.filename},
        {"content_type", uploaded.content_type},
        {"size", uploaded.size()}
    });
});
```

Multiple files:

```cpp
app.post("/uploads", [](Request request) {
    Json result = Json::array();

    for (const UploadedFile& uploaded : request.files("files")) {
        result.push_back({
            {"filename", uploaded.filename},
            {"size", uploaded.size()}
        });
    }

    return json(result);
});
```

`UploadedFile` contains:

| Field or Method | Purpose |
| --- | --- |
| `name` | Form field name |
| `filename` | File name |
| `content_type` | MIME type |
| `content` | File contents |
| `headers` | Form part headers |
| `size()` | Content size |
| `empty()` | Checks whether the file is empty |
| `save(path)` | Saves the file |

## Middleware

Middleware runs before the route handler. It can:

- pass the request forward through `next(request)`;
- return a response immediately;
- modify the response after the handler has run.

```cpp
app.middleware([](Request request, Next next) {
    Response response = next(request);
    response.header("X-Powered-By", "InAPI");
    return response;
});
```

Header check:

```cpp
app.middleware([](Request request, Next next) {
    if (request.header("X-API-Key") != "secret") {
        return error(401, "Invalid API key");
    }

    return next(request);
});
```

Execution order:

1. Global application middleware.
2. Router middleware, if the route is included through `include`.
3. Route handler.

## Router

`Router` helps split an API into route groups.

```cpp
Router users;

users.get("/", []() {
    return json({{"items", Json::array()}});
});

users.get("/{id:int}", [](Request request) {
    return json({{"id", request.param_int("id")}});
});

app.include("/users", users);
```

Final paths:

- `GET /users`
- `GET /users/{id:int}`

A router can have its own middleware:

```cpp
Router admin;

admin.middleware([](Request request, Next next) {
    if (request.header("X-Admin") != "true") {
        return forbidden();
    }

    return next(request);
});

admin.get("/stats", []() {
    return json({{"users", 100}});
});

app.include("/admin", admin);
```

## CORS

The shortest option:

```cpp
app.Cors();
```

Default values:

- origins: `*`
- methods: `GET`, `POST`, `PUT`, `PATCH`, `DELETE`, `OPTIONS`
- headers: `Content-Type`, `Authorization`

Configuration:

```cpp
app.Cors(CorsOptions(
    {"https://example.com"},
    {"GET", "POST"},
    {"Content-Type", "Authorization", "X-API-Key"}
));
```

InAPI answers preflight `OPTIONS` requests automatically.

## Authorization

### Bearer Token for the Whole Application

```cpp
app.BearerAuth("secret-token");
```

After this, the request must pass the header:

```http
Authorization: Bearer secret-token
```

### BearerAuth Through a Function

```cpp
app.BearerAuth([](Request request) {
    auto token = request.bearer_token();
    return token && token->size() > 10;
});
```

### Bearer Token for Router

```cpp
Router api;
api.BearerAuth("router-token");

api.get("/private", []() {
    return json({{"private", true}});
});

app.include("/api", api);
```

### Basic Auth

For Basic Auth, use the `require_auth` middleware and the `basic_auth` helper function.

```cpp
app.middleware(require_auth(
    basic_auth("admin", "password"),
    "Basic"
));
```

### Authorization Responses

```cpp
return unauthorized();
return unauthorized("Login required", "Bearer");
return forbidden();
return forbidden("Access denied");
```

## Validation

Validation is built through `ValidationSchema` and `field`.

```cpp
ValidationSchema user_schema = {
    field("name").string().required().min_len(2).max_len(50),
    field("age").integer().optional().min(0).max(150),
    field("email").string().required().email()
};
```

### JSON Body Validation

```cpp
app.post("/users", [](Request request) {
    ValidationSchema schema = {
        field("name").string().required().min_len(2),
        field("email").string().required().email(),
        field("age").integer().default_value(18)
    };

    Json body = request.body(schema);

    return json({
        {"created", true},
        {"user", body}
    }, 201);
});
```

If validation fails, InAPI returns `422`:

```json
{
  "error": "Validation failed",
  "details": [
    {
      "field": "email",
      "code": "invalid_email",
      "message": "Invalid email"
    }
  ]
}
```

### Query Validation

```cpp
app.get("/search", [](Request request) {
    ValidationSchema schema = {
        field("q").string().required(),
        field("page").integer().default_value(1).min(1),
        field("active").boolean().default_value(true)
    };

    Json query = request.query(schema);

    return json(query);
});
```

### Path Parameter Validation

```cpp
app.get("/users/{id}", [](Request request) {
    ValidationSchema schema = {
        field("id").integer().required().min(1)
    };

    Json params = request.params(schema);

    return json({
        {"id", params["id"]}
    });
});
```

### Available Rules

| Rule | Purpose |
| --- | --- |
| `string()` | String |
| `integer()` | Integer |
| `number()` | Number |
| `boolean()` | Boolean |
| `array()` | Array |
| `array(field(...))` | Array of elements validated by a rule |
| `object({...})` | Object with nested fields |
| `required()` | Field is required |
| `optional()` | Field is optional |
| `nullable()` | Allows `null` |
| `default_value(value)` | Default value |
| `min(value)` | Minimum number |
| `max(value)` | Maximum number |
| `min_len(value)` | Minimum string or array length |
| `max_len(value)` | Maximum string or array length |
| `one_of({...})` | One of the allowed values |
| `regex(pattern)` | Regular expression check |
| `email()` | Email |
| `url()` | URL |
| `uuid()` | UUID |
| `custom(message, callback)` | Custom validation |

### Nested Objects and Arrays

```cpp
ValidationSchema schema = {
    field("title").string().required(),
    field("tags").array(field("").string().min_len(2)),
    field("author").object({
        field("name").string().required(),
        field("email").string().email()
    }).required()
};
```

### Custom Rule

```cpp
ValidationSchema schema = {
    field("password").string().required().custom(
        "Password must contain at least one digit",
        [](const Json& value) {
            std::string password = value.get<std::string>();
            return password.find_first_of("0123456789") != std::string::npos;
        }
    )
};
```

## Error Handling

### HTTP Errors

```cpp
app.error_handler(404, [](Request request) {
    return json({
        {"error", "Route not found"},
        {"path", request.path()}
    }, 404);
});
```

If a status has no custom handler, InAPI returns standard JSON:

```json
{
  "error": "Not found"
}
```

Standard messages exist for these statuses:

- `400 Bad request`
- `401 Unauthorized`
- `403 Forbidden`
- `404 Not found`
- `405 Method not allowed`
- `409 Conflict`
- `413 Payload too large`
- `422 Unprocessable entity`
- `500 Internal server error`
- `502 Bad gateway`
- `503 Service unavailable`

### Exceptions

```cpp
app.exception_handler([](const std::exception& exception) {
    return json({
        {"error", exception.what()}
    }, 500);
});
```

`ValidationException` is handled separately and becomes a `422` response.

## Static Files

```cpp
app.mount("/static", "public");
```

After this, files from the `public` directory are available under `/static`.

Behavior:

- if a directory is requested, InAPI looks for `index.html`;
- if a file is not found, InAPI tries to serve `index.html` from the directory root, which is useful for SPAs;
- the path is protected from leaving the directory through `..`;
- static files get `Cache-Control`, `ETag`, and `Last-Modified`;
- `If-None-Match` and `If-Modified-Since` are supported with a `304` response.

## OpenAPI and Swagger

InAPI can generate an OpenAPI document and Swagger UI.

```cpp
Swagger swagger(
    true,
    "/docs",
    "Users API",
    "1.0.0",
    "Example InAPI documentation"
);

app.run("0.0.0.0", 8080, Config(), swagger);
```

After startup, these are available:

- Swagger UI: `/docs`
- OpenAPI JSON: `/docs/openapi.json`

### Route Documentation

Route methods return `RouteDoc`, so descriptions can be added as a chain:

```cpp
app.get("/users/{id:int}", [](Request request) {
    return json({
        {"id", request.param_int("id")},
        {"name", "Marat"}
    });
})
   .summary("Get user by id")
   .tag("Users")
   .response(200, "OK")
   .response(404, "User not found");
```

### Response Schemas

A type can describe an OpenAPI schema through static methods:

```cpp
struct User {
    static std::string openapi_name() {
        return "User";
    }

    static Json openapi_schema() {
        return {
            {"type", "object"},
            {"properties", {
                {"id", {{"type", "integer"}}},
                {"name", {{"type", "string"}}}
            }},
            {"required", {"id", "name"}}
        };
    }
};
```

Usage:

```cpp
app.get("/users/{id:int}", [](Request request) {
    return json({
        {"id", request.param_int("id")},
        {"name", "Marat"}
    });
})
   .summary("Get user by id")
   .tag("Users")
   .response<User>(200)
   .response(404, "User not found");
```

If `openapi_name()` is not specified, the name is taken from the type. If `openapi_schema()` is not specified, the schema will be:

```json
{
  "type": "object"
}
```

### Bearer Auth in OpenAPI

If `app.BearerAuth(...)` is enabled, Swagger automatically gets the `BearerAuth` scheme.

For an individual route:

```cpp
app.get("/private", []() {
    return json({{"private", true}});
})
   .bearer_auth()
   .summary("Private route");
```

Bearer auth can also be enabled at the Swagger level:

```cpp
Swagger swagger(true, "/docs", "API", "1.0.0", "", true);
```

## Config

`Config` configures the server.

```cpp
Config config(
    true,
    8,
    "10mb",
    5,
    10,
    30
);
```

Parameters:

| Parameter | Value |
| --- | --- |
| `logger` | Enables logging |
| `threads` | Number of threads |
| `max_body_size` | Maximum request body size |
| `read_timeout_seconds` | Read timeout |
| `write_timeout_seconds` | Write timeout |
| `idle_timeout_seconds` | Keep-Alive idle timeout |
| `ssl` | SSL settings |

Default values:

```cpp
Config(
    true,
    auto_threads(),
    "1mb",
    5,
    10,
    30,
    std::nullopt
);
```

Supported `max_body_size` values:

- `512kb`
- `1mb`
- `10mb`
- `1gb`

If a different value is specified, `1mb` will be used.

Run variants:

```cpp
app.run(8080);
app.run(8080, "127.0.0.1");
app.run("127.0.0.1", 8080);
app.run("0.0.0.0", 8080, config);
app.run("0.0.0.0", 8080, config, swagger);
```

## SSL

SSL is configured through `SSL` inside `Config`.

```cpp
Config config(
    true,
    8,
    "10mb",
    5,
    10,
    30,
    SSL("cert.pem", "key.pem")
);

app.run("0.0.0.0", 443, config);
```

Important:

- for SSL, the library must be built with `CPPHTTPLIB_OPENSSL_SUPPORT`;
- OpenSSL libraries are also required;
- if the certificate or key is not found, InAPI throws an exception.

## Logging

Logging is enabled by default.

InAPI writes:

- a message when the server starts;
- one line for each request;
- the HTTP status in color if the terminal supports ANSI colors.

Disable logging:

```cpp
Config config(false);
app.run("0.0.0.0", 8080, config);
```

Manual message:

```cpp
InAPILogger::info("Server is ready");
```

## Full Example

```cpp
#include <InAPI.hpp>

struct User {
    static std::string openapi_name() {
        return "User";
    }

    static Json openapi_schema() {
        return {
            {"type", "object"},
            {"properties", {
                {"id", {{"type", "integer"}}},
                {"name", {{"type", "string"}}},
                {"email", {{"type", "string"}}}
            }},
            {"required", {"id", "name", "email"}}
        };
    }
};

int main() {
    App app;

    app.Cors();

    app.middleware([](Request request, Next next) {
        Response response = next(request);
        response.header("X-App", "InAPI");
        return response;
    });

    Router users;

    users.get("/{id:int}", [](Request request) {
        int id = request.param_int("id");

        return json({
            {"id", id},
            {"name", "Marat"},
            {"email", "marat@example.com"}
        });
    })
       .summary("Get user by id")
       .tag("Users")
       .response<User>(200)
       .response(404, "User not found");

    users.post("/", [](Request request) {
        ValidationSchema schema = {
            field("name").string().required().min_len(2),
            field("email").string().required().email()
        };

        Json body = request.body(schema);

        return json({
            {"created", true},
            {"user", body}
        }, 201);
    })
       .summary("Create user")
       .tag("Users")
       .response<User>(201)
       .response(422, "Validation failed");

    app.include("/users", users);

    app.error_handler(404, [](Request request) {
        return json({
            {"error", "Route not found"},
            {"path", request.path()}
        }, 404);
    });

    app.exception_handler([](const std::exception& exception) {
        return json({
            {"error", exception.what()}
        }, 500);
    });

    Config config(
        true,
        8,
        "10mb",
        5,
        10,
        30
    );

    Swagger swagger(
        true,
        "/docs",
        "Users API",
        "1.0.0",
        "Example InAPI documentation"
    );

    app.run("0.0.0.0", 8080, config, swagger);
}
```

## Cheat Sheet

```cpp
App app;

app.get("/", []() {
    return text("Hello");
});

app.post("/users", [](Request request) {
    Json body = request.json();
    return json(body, 201);
});

app.get("/users/{id:int}", [](Request request) {
    return json({{"id", request.param_int("id")}});
});

Router api;
api.get("/health", []() {
    return json({{"ok", true}});
});
app.include("/api", api);

app.Cors();
app.BearerAuth("secret-token");
app.mount("/static", "public");

app.error_handler(404, [](Request request) {
    return error(404, "Route not found");
});

app.run(8080);
```
