#include <InAPI.hpp>

int main() {
    App app;

    app.get("/", [] {
        return text("Hello from InAPI!");
    });

    app.get("/echo/{message:path}", [](Request req) {
        return text(req.param("message"));
    });

    app.run(8080);
}
