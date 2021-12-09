REM for /f "tokens=* usebackq" %%x in (`git describe --tags`) do (set version=%%x)
set version=1.0
echo #define PLUGIN_VERSION "%version%">Version.hpp
if "%RELEASE_BUILD%"=="" echo #define PLUGIN_DEV_BUILD 1>>Version.hpp
