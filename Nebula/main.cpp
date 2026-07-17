#include <spdlog/spdlog.h>
#include "Core/App.hpp"
#include "Core/Configuration.hpp"

#include "ShaderBuild.hpp"

UPtr<App> lApplication;

int main()
{
    // 0. Initial logger config
    spdlog::set_pattern("[%^%l%$] %v");
    #ifdef NDEBUG
        spdlog::set_level(spdlog::level::debug);
    #else
        spdlog::set_level(spdlog::level::debug);
    #endif

    // 1. Load Nebula configuration
    auto* config = Configuration::getInstance();

    // 1.5. Compile shaders
    std::ignore = nbl::ShaderBuild();

    // 2. Create Application instance and set global reference.
    lApplication = App::create();
    gApplication = lApplication.get();

    // 3. Run application
    lApplication->run();

    lApplication.reset();

    return 0;
}
