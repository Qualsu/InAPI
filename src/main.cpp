#include <InAPI.hpp>

int main() {
    App app;

    app.get("/", [] {
        return text("Hello from InAPI!");
    });

    app.get("/about", [] {
        return text("This is simple C++ web framework by Qualsu");
    });

    app.get("/user", [] {
        return json({
            {"name", "Kenyka"},
            {"language", "C++"},
            {"framework", "InAPI"}
        });
    });

    app.run(8080);
}
