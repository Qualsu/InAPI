# InAPI Документация

**InAPI** - С++ библиотека для создания HTTP приложений с FastAPI-подобным синтаксисом
Внутри используются `cpp-httplib` и `nlohmann/json`

## Навигация

- [Быстрый старт](#быстрый-старт)
- [Сборка](#сборка)
- [Подключение](#подключение)
- [App](#app)
- [Маршруты](#маршруты)
- [Request](#request)
- [Response](#response)
- [JSON](#json)
- [Параметры пути](#параметры-пути)
- [Query-параметры](#query-параметры)
- [Заголовки и cookies](#заголовки-и-cookies)
- [Формы и загрузка файлов](#формы-и-загрузка-файлов)
- [Middleware](#middleware)
- [Router](#router)
- [CORS](#cors)
- [Авторизация](#авторизация)
- [Валидация](#валидация)
- [Обработка ошибок](#обработка-ошибок)
- [Статические файлы](#статические-файлы)
- [OpenAPI и Swagger](#openapi-и-swagger)
- [Config](#config)
- [SSL](#ssl)
- [Логирование](#логирование)
- [Полный пример](#полный-пример)
- [Краткая шпаргалка](#краткая-шпаргалка)

## Быстрый старт

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

## Сборка

В репозитории есть `makefile` для сборки примера из `src/main.cpp`. Он использует `g++` и складывает результат в папку `build`.

Обычная сборка без запуска:

```sh
make compile
```

Сборка и запуск обычного примера:

```sh
make
```

Сборка своего файла вместо `src/main.cpp`:

```sh
make compile SRC=src/app.cpp
```

Сборка и запуск своего файла:

```sh
make SRC=src/app.cpp
```

Сборка с OpenSSL без запуска:

```sh
make compile_ssl
```

Сборка и запуск SSL-варианта:

```sh
make ssl
```

Запуск уже собранного обычного бинарника:

```sh
make run
```

Очистка собранных бинарников:

```sh
make clean
```

**Для сборки нужны:**

- компилятор с поддержкой C++17 или новее, например `g++`;
- `make`;
- для SSL-сборки - установленные заголовки и библиотеки OpenSSL (`ssl` и `crypto`).

Если компилятор называется иначе, его можно передать через переменную `CXX`:

```sh
make compile CXX=clang++
```

## Подключение

Подключите основной заголовок:

```cpp
#include <InAPI.hpp>
```

Если подключаете заголовки вручную, файлы лежат в `include/InAPI`.

## App

`App` - основной класс приложения. Через него добавляют маршруты, middleware, CORS, авторизацию, обработчики ошибок, статические файлы и Swagger.

```cpp
App app;
```

Основные методы:

| Метод | Назначение |
| --- | --- |
| `get(path, handler)` | Регистрирует GET маршрут |
| `post(path, handler)` | Регистрирует POST маршрут |
| `put(path, handler)` | Регистрирует PUT маршрут |
| `patch(path, handler)` | Регистрирует PATCH маршрут |
| `del(path, handler)` | Регистрирует DELETE маршрут |
| `options(path, handler)` | Регистрирует OPTIONS маршрут |
| `middleware(handler)` | Добавляет глобальный middleware |
| `include(prefix, router)` | Подключает `Router` с префиксом |
| `Cors(options)` | Включает CORS |
| `BearerAuth(token)` | Включает Bearer авторизацию по токену |
| `BearerAuth(hook)` | Включает Bearer авторизацию через функцию |
| `error_handler(status, handler)` | Добавляет обработчик HTTP ошибки |
| `exception_handler(handler)` | Добавляет обработчик исключений |
| `mount(path, directory)` | Отдает статические файлы из папки |
| `run(...)` | Запускает сервер |

## Маршруты

Обработчик может принимать `Request`:

```cpp
app.get("/hello", [](Request request) {
    return text("Path: " + request.path());
});
```

Если данные запроса не нужны, обработчик может быть без аргументов:

```cpp
app.get("/", []() {
    return text("Home");
});
```

Поддерживаемые HTTP методы:

```cpp
app.get("/items", handler);
app.post("/items", handler);
app.put("/items/{id:int}", handler);
app.patch("/items/{id:int}", handler);
app.del("/items/{id:int}", handler);
app.options("/items", handler);
```

Каждый обработчик должен вернуть `Response`.

## Request

`Request` содержит данные входящего HTTP запроса.

Основные методы:

| Метод | Что возвращает |
| --- | --- |
| `method()` | HTTP метод |
| `path()` | Путь запроса |
| `body()` | Тело запроса строкой |
| `json()` | Тело запроса как `nlohmann::json` |
| `body(schema)` | JSON body после валидации |
| `header(name)` | Значение заголовка |
| `has_header(name)` | Есть ли заголовок |
| `query(name)` | Query-параметр |
| `query_or(name, default_value)` | Query-параметр или значение по умолчанию |
| `query_int(name, default_value)` | Query-параметр как `int` |
| `query_bool(name, default_value)` | Query-параметр как `bool` |
| `has_query(name)` | Есть ли query параметр |
| `query(schema)` | Query-параметры после валидации |
| `param(name)` | Параметр пути |
| `param_int(name)` | Параметр пути как `int` |
| `has_param(name)` | Есть ли параметр пути |
| `params(schema)` | Параметры пути после валидации |
| `cookie(name)` | Cookie |
| `has_cookie(name)` | Есть ли cookie |
| `form(name)` | Поле формы |
| `form_or(name, default_value)` | Поле формы или значение по умолчанию |
| `has_form(name)` | Есть ли поле формы |
| `has_file(name)` | Есть ли файл |
| `file(name)` | Один загруженный файл |
| `files()` | Все загруженные файлы |
| `files(name)` | Все файлы с указанным именем поля |
| `bearer_token()` | Bearer-токен из `Authorization` |
| `basic_auth()` | Данные Basic Auth |
| `content_type()` | `Content-Type` |
| `user_agent()` | `User-Agent` |
| `ip()` | IP клиента |
| `port()` | Порт клиента |
| `http_version()` | Версия HTTP |

## Response

`Response` описывает ответ сервера.

```cpp
return Response(200, "OK", "text/plain; charset=utf-8");
```

Обычно удобнее использовать вспомогательные функции:

| Helper | Назначение |
| --- | --- |
| `text(body, status = 200)` | Текстовый ответ |
| `html(body, status = 200)` | HTML ответ |
| `json(body, status = 200)` | JSON ответ |
| `redirect(url, status = 302)` | Redirect с заголовком `Location` |
| `status(code)` | Пустой ответ с указанным статусом |
| `file(path, status = 200)` | Ответ с файлом |
| `error(status)` | JSON ошибка по статусу |
| `error(status, message)` | JSON ошибка с текстом |

Примеры:

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

### Заголовки ответа

```cpp
app.get("/headers", []() {
    Response response = json({{"ok", true}});
    response.header("X-App", "InAPI");
    return response;
});
```

### Cookies ответа

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

`set_cookie` принимает аргументы:

```cpp
set_cookie(name, value, path, max_age, http_only, secure, same_site)
```

Значения по умолчанию:

- `path = "/"`
- `max_age = -1`
- `http_only = true`
- `secure = false`
- `same_site = "Lax"`

## JSON

InAPI использует `nlohmann::json`.

```cpp
app.post("/echo", [](Request request) {
    Json body = request.json();

    return json({
        {"you_sent", body}
    });
});
```

Если тело запроса содержит невалидный JSON, `request.json()` бросит `nlohmann::json::parse_error`.

Для автоматического ответа `422` используйте `request.body(schema)`.

## Параметры пути

Параметры пути пишутся в фигурных скобках:

```cpp
app.get("/users/{id}", [](Request request) {
    return text(request.param("id"));
});
```

Поддерживаемые типы:

| Синтаксис | Что принимает |
| --- | --- |
| `{name}` | Любой текст до `/` |
| `{name:int}` | Целое число, включая отрицательное |
| `{name:path}` | Остаток пути, включая `/` |

Примеры:

```cpp
app.get("/users/{id:int}", [](Request request) {
    int id = request.param_int("id");
    return json({{"id", id}});
});

app.get("/files/{path:path}", [](Request request) {
    return json({{"path", request.param("path")}});
});
```

## Query-параметры

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

`query_bool` понимает такие значения:

- `true`: `1`, `true`, `yes`, `on`
- `false`: `0`, `false`, `no`, `off`

## Заголовки и cookies

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

Bearer-токен:

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

## Формы и загрузка файлов

Поля формы:

```cpp
app.post("/form", [](Request request) {
    std::string name = request.form_or("name", "anonymous");

    return json({
        {"name", name}
    });
});
```

Один файл:

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

Несколько файлов:

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

`UploadedFile` содержит:

| Поле или метод | Назначение |
| --- | --- |
| `name` | Имя поля формы |
| `filename` | Имя файла |
| `content_type` | MIME-тип |
| `content` | Содержимое файла |
| `headers` | Заголовки части формы |
| `size()` | Размер содержимого |
| `empty()` | Проверяет, пустой ли файл |
| `save(path)` | Сохраняет файл |

## Middleware

Middleware выполняется перед обработчиком маршрута. Он может:

- передать запрос дальше через `next(request)`;
- сразу вернуть ответ;
- изменить ответ после выполнения обработчика.

```cpp
app.middleware([](Request request, Next next) {
    Response response = next(request);
    response.header("X-Powered-By", "InAPI");
    return response;
});
```

Проверка заголовка:

```cpp
app.middleware([](Request request, Next next) {
    if (request.header("X-API-Key") != "secret") {
        return error(401, "Invalid API key");
    }

    return next(request);
});
```

Порядок выполнения:

1. Глобальные middleware приложения.
2. Middleware роутера, если маршрут подключен через `include`.
3. Обработчик маршрута.

## Router

`Router` помогает разделять API на группы маршрутов.

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

Итоговые пути:

- `GET /users`
- `GET /users/{id:int}`

У роутера могут быть свои middleware:

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

Самый короткий вариант:

```cpp
app.Cors();
```

Значения по умолчанию:

- origins: `*`
- methods: `GET`, `POST`, `PUT`, `PATCH`, `DELETE`, `OPTIONS`
- headers: `Content-Type`, `Authorization`

Настройка:

```cpp
app.Cors(CorsOptions(
    {"https://example.com"},
    {"GET", "POST"},
    {"Content-Type", "Authorization", "X-API-Key"}
));
```

InAPI сам отвечает на preflight `OPTIONS` запросы.

## Авторизация

### Bearer-токен для всего приложения

```cpp
app.BearerAuth("secret-token");
```

После этого запрос должен передавать заголовок:

```http
Authorization: Bearer secret-token
```

### BearerAuth через функцию

```cpp
app.BearerAuth([](Request request) {
    auto token = request.bearer_token();
    return token && token->size() > 10;
});
```

### Bearer-токен для Router

```cpp
Router api;
api.BearerAuth("router-token");

api.get("/private", []() {
    return json({{"private", true}});
});

app.include("/api", api);
```

### Basic Auth

Для Basic Auth используйте middleware `require_auth` и вспомогательную функцию `basic_auth`.

```cpp
app.middleware(require_auth(
    basic_auth("admin", "password"),
    "Basic"
));
```

### Ответы авторизации

```cpp
return unauthorized();
return unauthorized("Login required", "Bearer");
return forbidden();
return forbidden("Access denied");
```

## Валидация

Валидация строится через `ValidationSchema` и `field`.

```cpp
ValidationSchema user_schema = {
    field("name").string().required().min_len(2).max_len(50),
    field("age").integer().optional().min(0).max(150),
    field("email").string().required().email()
};
```

### Валидация JSON body

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

Если валидация не прошла, InAPI вернет `422`:

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

### Валидация query

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

### Валидация параметров пути

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

### Доступные правила

| Правило | Назначение |
| --- | --- |
| `string()` | Строка |
| `integer()` | Целое число |
| `number()` | Число |
| `boolean()` | Boolean |
| `array()` | Массив |
| `array(field(...))` | Массив элементов по правилу |
| `object({...})` | Объект с вложенными полями |
| `required()` | Поле обязательно |
| `optional()` | Поле необязательно |
| `nullable()` | Разрешает `null` |
| `default_value(value)` | Значение по умолчанию |
| `min(value)` | Минимальное число |
| `max(value)` | Максимальное число |
| `min_len(value)` | Минимальная длина строки или массива |
| `max_len(value)` | Максимальная длина строки или массива |
| `one_of({...})` | Одно из разрешенных значений |
| `regex(pattern)` | Проверка регулярным выражением |
| `email()` | Email |
| `url()` | URL |
| `uuid()` | UUID |
| `custom(message, callback)` | Пользовательская проверка |

### Вложенные объекты и массивы

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

### Custom правило

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

## Обработка ошибок

### HTTP ошибки

```cpp
app.error_handler(404, [](Request request) {
    return json({
        {"error", "Route not found"},
        {"path", request.path()}
    }, 404);
});
```

Если для статуса нет своего обработчика, InAPI вернет стандартный JSON:

```json
{
  "error": "Not found"
}
```

Стандартные сообщения есть для статусов:

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

### Исключения

```cpp
app.exception_handler([](const std::exception& exception) {
    return json({
        {"error", exception.what()}
    }, 500);
});
```

`ValidationException` обрабатывается отдельно и превращается в ответ `422`.

## Статические файлы

```cpp
app.mount("/static", "public");
```

После этого файлы из папки `public` доступны по пути `/static`.

Поведение:

- если запрошена директория, InAPI ищет `index.html`;
- если файл не найден, InAPI пробует отдать `index.html` из корня папки, что удобно для SPA;
- путь защищен от выхода за пределы папки через `..`;
- для статических файлов выставляются `Cache-Control`, `ETag` и `Last-Modified`;
- `If-None-Match` и `If-Modified-Since` поддерживаются с ответом `304`.

## OpenAPI и Swagger

InAPI может сгенерировать OpenAPI документ и Swagger UI.

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

После запуска доступны:

- Swagger UI: `/docs`
- OpenAPI JSON: `/docs/openapi.json`

### Документирование маршрута

Методы маршрутов возвращают `RouteDoc`, поэтому описание можно добавлять цепочкой:

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

### Схемы ответов

Тип может описать OpenAPI схему через статические методы:

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

Использование:

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

Если `openapi_name()` не указан, имя берется из типа. Если `openapi_schema()` не указан, схема будет такой:

```json
{
  "type": "object"
}
```

### Bearer auth в OpenAPI

Если включить `app.BearerAuth(...)`, Swagger автоматически получит схему `BearerAuth`.

Для отдельного маршрута:

```cpp
app.get("/private", []() {
    return json({{"private", true}});
})
   .bearer_auth()
   .summary("Private route");
```

Bearer auth можно включить и на уровне Swagger:

```cpp
Swagger swagger(true, "/docs", "API", "1.0.0", "", true);
```

## Config

`Config` настраивает сервер.

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

Параметры:

| Параметр | Значение |
| --- | --- |
| `logger` | Включает логирование |
| `threads` | Количество потоков |
| `max_body_size` | Максимальный размер тела запроса |
| `read_timeout_seconds` | Таймаут чтения |
| `write_timeout_seconds` | Таймаут записи |
| `idle_timeout_seconds` | Таймаут простоя Keep-Alive соединения |
| `ssl` | SSL настройки |

Значения по умолчанию:

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

Поддерживаемые значения `max_body_size`:

- `512kb`
- `1mb`
- `10mb`
- `1gb`

Если указать другое значение, будет использовано `1mb`.

Варианты запуска:

```cpp
app.run(8080);
app.run(8080, "127.0.0.1");
app.run("127.0.0.1", 8080);
app.run("0.0.0.0", 8080, config);
app.run("0.0.0.0", 8080, config, swagger);
```

## SSL

SSL настраивается через `SSL` внутри `Config`.

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

Важно:

- для SSL библиотеку нужно собрать с `CPPHTTPLIB_OPENSSL_SUPPORT`;
- также нужны библиотеки OpenSSL;
- если сертификат или ключ не найдены, InAPI бросит исключение.

## Логирование

Логирование включено по умолчанию.

InAPI пишет:

- сообщение при старте сервера;
- строку на каждый запрос;
- HTTP статус цветом, если терминал поддерживает ANSI цвета.

Отключение:

```cpp
Config config(false);
app.run("0.0.0.0", 8080, config);
```

Ручное сообщение:

```cpp
InAPILogger::info("Server is ready");
```

## Полный пример

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

## Краткая шпаргалка

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
