@echo off
cls
set VERSION=1_3_0
set VERSIONn=1.3.0

set WIX_DIR="C:\Program Files (x86)\WiX Toolset v3.9\bin"
if not exist %WIX_DIR% set WIX_DIR="C:\Program Files\WiX Toolset v3.9\bin"

set HEAT=%WIX_DIR%\heat.exe
set CANDLE=%WIX_DIR%\candle.exe
set LIGHT=%WIX_DIR%\light.exe
set TORCH=%WIX_DIR%\torch.exe
set PYRO=%WIX_DIR%\pyro.exe


set WIX_BUILD_DIR=wixBuildDir
set CANDLE_OPTIONS=-ext WixUIExtension -ext WixDifxAppExtension.dll

set LIGHT_OPTIONS=-ext WixUIExtension -ext WixUtilExtension -ext WixDifxAppExtension.dll -sw0204 
set LIGHT_DIRS=
rem -b bin\DeviceData 

set HEAT_OPTIONS=-srd -ke -gg -sfrag -template fragment -sw5150
set MSI_FILE=CDC_Drivers_%VERSION%_Win

if "%1"=="clean" goto doMake
if "%1"==""      goto doMake
echo "Unknown option %1"
goto finish

:doMake
cmd /c "%~dp0\Make Cat Files.cmd"
if exist %MSI_FILE%.msi    del %MSI_FILE%.msi

if "%1"=="clean" goto finish

if not exist %WIX_BUILD_DIR% mkdir %WIX_BUILD_DIR%
%CANDLE% %CANDLE_OPTIONS% -dProductVersion=%VERSIONn% -dTargetArchCondition="Msix64" -o %WIX_BUILD_DIR%\%MSI_FILE%.wixobj Driver.wxs
%LIGHT% %LIGHT_OPTIONS% %LIGHT_DIRS% -out %MSI_FILE%    %WIX_BUILD_DIR%\%MSI_FILE%.wixobj    %WIX_DIR%\difxapp_x64.wixlib

del *.wixpdb
rmdir /S /Q %WIX_BUILD_DIR%

set STORE=My
set SIGNTOOL=C:\Apps\WinDDK\7600.16385.1\bin\amd64\SignTool.exe
set TIMESTAMP_URL=http://timestamp.verisign.com/scripts/timstamp.dll
set SHA=19B5A37F5AF7C74D9909519F16ED2B5AA53F1C10

echo Signing files
for %%f in (%~dp0\*.msi) do %SIGNTOOL% sign /s %STORE% /t %TIMESTAMP_URL% /sha1 %SHA% %%f

goto finish

:finish
pause