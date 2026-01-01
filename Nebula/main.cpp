#include "Core/App.hpp"
#include "Core/Configuration.hpp"
#include "Utils/CIFParser.hpp"

UPtr<App> lApplication;

int main()
{
    // 0.5 Test CIFparser integration
    CIFParser cif("Resources/CIFFiles/IBP.cif");
    //cif.printPositions();

    // 1. Load Nebula configuration
    auto* config = Configuration::getInstance();

    // 2. Create Application instance and set global reference.
    lApplication = App::create();
    gApplication = lApplication.get();

    // 3. Run application
    lApplication->run_renderPathLoop();

    lApplication.reset();

    return 0;
}