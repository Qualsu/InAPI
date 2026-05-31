#include <InAPI.hpp>

int main() {
    App app;

    app.get("/", [] {
        return text("Hello from InAPI!");
    });

    app.run(8080);
}
