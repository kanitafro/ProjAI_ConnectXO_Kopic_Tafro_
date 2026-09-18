// don't change

#include "Application.h"
#include <td/StringConverter.h>
#include <gui/WinMain.h>
#include "Theme.h"

extern "C" void setThemeIndex(int idx);

int main(int argc, const char * argv[])
{
    Application app(argc, argv);
    mu::dbgLog("INFO: Application just created!");
    //load properties from OS environment (registry on windows, plist on mac, settings scheme on linux...)
    auto appProperties = app.getProperties();
    td::String trLang = appProperties->getValue("translation", "EN");

    // Load persisted theme and apply it before initializing views
    int themeIdx = appProperties->getValue("theme", to_int(ThemeIndex::Classic)); // default to Classic
    try
    {
        setThemeIndex(themeIdx);
        mu::dbgLog("INFO: Theme loaded: %d", themeIdx);
    }
    catch (...)
    {
        mu::dbgLog("WARNING: setThemeIndex failed, continuing with default theme");
    }

    try
    {
        app.init(trLang);
    }
    catch (...)
    {
        mu::dbgLog("ERROR: app.init() threw exception");
        return -1;
    }

    return app.run();
}

