@echo off
setlocal EnableExtensions
rem ---- reopen in a window that STAYS OPEN, so errors can be read ----
if /i "%~1"=="--inner" goto :main
start "DepthCharge build" cmd /k call "%~f0" --inner
exit /b

:main
cd /d "%~dp0"
echo ==========================================================
echo    DEPTHCHARGE builder
echo ==========================================================
echo  Folder : %CD%
echo  Started: %TIME%
echo.

rem ---- 0. make sure we're in the extracted folder, not inside the zip ----
echo %CD% | find /i "\Temp\" >nul
if not errorlevel 1 goto :inzip
if not exist "CMakeLists.txt" goto :inzip
if not exist "Source\DepthEngine.h" goto :inzip

rem ---- 1. Git ----
echo [1/4] Checking for Git...
where git >nul 2>nul
if errorlevel 1 goto :nogit
echo       OK

rem ---- 2. Visual Studio C++ tools (they include CMake) ----
echo [2/4] Checking for Visual Studio C++ tools...
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" goto :novs
set "VSPATH="
for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSPATH=%%i"
if not defined VSPATH goto :novs
echo       Found: %VSPATH%
call "%VSPATH%\Common7\Tools\VsDevCmd.bat" -arch=amd64 -host_arch=amd64 >nul
where cmake >nul 2>nul
if errorlevel 1 goto :nocmake
echo       OK

rem clear out a half-finished build folder from an earlier failed attempt
if exist "build\CMakeCache.txt" if not exist "build\DepthCharge.sln" rmdir /s /q build

rem ---- 3. configure ----
echo.
echo [3/4] Configuring. First time downloads JUCE (about 1-3 minutes)...
echo.
cmake -B build -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 goto :fail

rem ---- 4. compile ----
echo.
echo [4/4] Compiling. First build takes about 5-15 minutes.
echo       Lots of scrolling text is normal - let it run.
echo.
cmake --build build --config Release --target DepthCharge_VST3 DepthCharge_Standalone --parallel
if errorlevel 1 goto :fail

echo.
echo ==========================================================
echo  DONE!   Started %TIME% ... finished now.
echo ==========================================================
echo  Your plugin:
echo    %CD%\build\DepthCharge_artefacts\Release\VST3\DepthCharge.vst3
echo.
echo  Copy that whole DepthCharge.vst3 folder into:
echo    C:\Program Files\Common Files\VST3
echo  then in FL Studio: Options - Manage plugins - Find installed plugins.
echo.
explorer "%CD%\build\DepthCharge_artefacts\Release\VST3"
goto :end

:inzip
echo  PROBLEM: build.bat is not running from the extracted project folder.
echo.
echo  Fix: right-click DepthCharge.zip - "Extract All..." - Extract.
echo       Then open the extracted DepthCharge folder and run build.bat there.
echo       (Double-clicking it from inside the zip doesn't work.)
goto :end

:nogit
echo.
echo  PROBLEM: Git is not installed (needed to download JUCE).
echo.
echo  Fix: install it from https://git-scm.com/download/win
echo       (just click Next through the installer), then run build.bat again.
start "" "https://git-scm.com/download/win"
goto :end

:novs
echo.
echo  PROBLEM: Visual Studio's C++ tools are not installed.
echo.
echo  Fix: 1. Install "Visual Studio Community" (free) from
echo          https://visualstudio.microsoft.com/vs/community/
echo       2. In the installer, tick "Desktop development with C++" and install
echo          (it's a big download, around 7 GB).
echo       3. If Visual Studio is already installed: open "Visual Studio Installer",
echo          click Modify, tick "Desktop development with C++".
echo       4. Run build.bat again.
start "" "https://visualstudio.microsoft.com/vs/community/"
goto :end

:nocmake
echo.
echo  PROBLEM: Visual Studio was found but CMake wasn't.
echo.
echo  Fix: open "Visual Studio Installer" - Modify - Individual components,
echo       tick "C++ CMake tools for Windows", install, then run build.bat again.
goto :end

:fail
echo.
echo ==========================================================
echo  BUILD FAILED. Scroll up to find the first line containing "error".
echo  Copy the last ~30 lines and send them over to get it fixed.
echo ==========================================================
goto :end

:end
echo.
echo  (This window stays open. Close it when you're done.)
