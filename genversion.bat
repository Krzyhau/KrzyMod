for /f "tokens=* usebackq" %%x in (`git describe --tags`) do (set version=%%x)
echo #define PLUGIN_VERSION "%version%">Version.hpp
if "%RELEASE_BUILD%"=="" echo #define PLUGIN_DEV_BUILD 1>>Version.hpp
