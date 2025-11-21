#include "Core/App.hpp"
#include "Core/Configuration.hpp"

UPtr<App> lApplication;

int main()
{
    // 1. Load Nebula configuration
    auto* config = Configuration::getInstance();

    // 2. Create Application instance and set global reference.
    lApplication = App::create();
    gApplication = lApplication.get();

    // 3. Run application
    lApplication->run();

    lApplication.reset();

    return 0;
}