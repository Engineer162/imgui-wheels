@REM Build for Visual Studio compiler. Run your copy of vcvars64.bat or vcvarsall.bat to setup command-line compiler.

@set OUT_EXE=imgui-wheels-example_sdl3_opengl3

@where cl >nul 2>nul
@if errorlevel 1 (
	@set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
	@if exist "%VSWHERE%" (
		@for /f "usebackq delims=" %%I in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do @call "%%I\VC\Auxiliary\Build\vcvars64.bat" >nul
	)
	@if exist "%ProgramFiles%\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" @call "%ProgramFiles%\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
	@if exist "%ProgramFiles%\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat" @call "%ProgramFiles%\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat" >nul
	@if exist "%ProgramFiles%\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat" @call "%ProgramFiles%\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat" >nul
	@if exist "%ProgramFiles%\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" @call "%ProgramFiles%\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
)

@where cl >nul 2>nul
@if errorlevel 1 (
	@echo cl.exe was not found. Run this script from a Visual Studio Developer Command Prompt,
	@echo or install Visual Studio C++ build tools.
	@exit /b 1
)

@if "%SDL3_DIR%"=="" set SDL3_DIR=%~dp0..\external\SDL
@set SDL3_DIR=%SDL3_DIR:/=\%

@if not exist "%SDL3_DIR%\include\SDL3\SDL.h" (
	@echo SDL3 headers not found at "%SDL3_DIR%".
	@echo Set SDL3_DIR or initialize submodules: git submodule update --init --recursive
	@exit /b 1
)

@set SDL3_LIB_DIR=%SDL3_DIR%\lib\x64
@if not exist "%SDL3_LIB_DIR%\SDL3.lib" set SDL3_LIB_DIR=%SDL3_DIR%\build\Release
@if not exist "%SDL3_LIB_DIR%\SDL3.lib" set SDL3_LIB_DIR=%SDL3_DIR%\build\RelWithDebInfo
@if not exist "%SDL3_LIB_DIR%\SDL3.lib" set SDL3_LIB_DIR=%SDL3_DIR%\build-x64\Release
@if not exist "%SDL3_LIB_DIR%\SDL3.lib" set SDL3_LIB_DIR=%SDL3_DIR%\build-x64\RelWithDebInfo
@if not exist "%SDL3_LIB_DIR%\SDL3.lib" (
	@echo SDL3.lib not found.
	@echo Build SDL first, for example:
	@echo   cmake -S "%SDL3_DIR%" -B "%SDL3_DIR%\build" -A x64
	@echo   cmake --build "%SDL3_DIR%\build" --config Release
	@exit /b 1
)

@set INCLUDES=/I. /I.. /I"%SDL3_DIR%\include" /I"%SDL3_DIR%\include\SDL3"
@set SOURCES=main.cpp imgui_impl_sdl3.cpp imgui_impl_opengl3.cpp imgui.cpp imgui_draw.cpp imgui_tables.cpp imgui_widgets.cpp ..\imgui-wheels.cpp
@set LIBS=/LIBPATH:"%SDL3_LIB_DIR%" SDL3.lib opengl32.lib shell32.lib

@set OUT_DIR=Debug
mkdir %OUT_DIR%
cl /nologo /Zi /MD /std:c++17 /utf-8 %INCLUDES% %SOURCES% /Fe%OUT_DIR%/%OUT_EXE%.exe /Fo%OUT_DIR%/ /link %LIBS% /subsystem:console
@if exist "%SDL3_LIB_DIR%\SDL3.dll" copy /Y "%SDL3_LIB_DIR%\SDL3.dll" %OUT_DIR%\ >nul

@set OUT_DIR=Release
mkdir %OUT_DIR%
cl /nologo /Zi /MD /std:c++17 /utf-8 /Ox /Oi %INCLUDES% %SOURCES% /Fe%OUT_DIR%/%OUT_EXE%.exe /Fo%OUT_DIR%/ /link %LIBS% /subsystem:console
@if exist "%SDL3_LIB_DIR%\SDL3.dll" copy /Y "%SDL3_LIB_DIR%\SDL3.dll" %OUT_DIR%\ >nul