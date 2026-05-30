#include <InAPI.hpp>

int main() {
    App app;

    app.get("/", [] {
        return "Hello from InAPI!";
    });

    app.get("/about", [] {
        return "This is simple C++ web framework by Qualsu";
    });

    app.get("/user", [] {
        json data = {
            {"name", "Kenyka"},
            {"language", "C++"},
            {"framework", "InAPI"}
        };

        return Response{
            data.dump(),
            "application/json",
            200
        };
    });

    app.run(8080);
}