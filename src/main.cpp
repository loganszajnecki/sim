#include "app/Application.hpp"

int main(int argc, char** argv) {
    app::ApplicationOptions opts{};
    // first non-flag arg = config path (optional)
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a.rfind("--", 0) != 0) { opts.config_path = a; break; }
    }
    app::Application app(opts);
    return app.run();
}
