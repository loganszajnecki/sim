// main.cpp
#include "app/Application.hpp"
#include <string>

int main(int argc, char** argv)
{
    app::ApplicationOptions opts{};

    // First non-flag argument is treated as the config path (optional).
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg.rfind("--", 0) != 0) {
            opts.config_path = arg;
            break;
        }
    }

    app::Application app(opts);
    return app.run();
}
