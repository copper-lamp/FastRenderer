@echo off
REM ──────────────────────────────────────────────
REM  FastRenderer-Android Build Script
REM  Prerequisites:
REM    1. Android NDK r28+ installed (set ANDROID_NDK_HOME)
REM    2. CMake 3.22+ installed
REM    3. Ninja build system installed
REM    4. Dear ImGui (docking branch) in deps/imgui/
REM    5. LeviLaunchroid Preloader SDK in sdk/
REM    6. GlossHook pre-compiled lib in libs/arm64/
REM ──────────────────────────────────────────────

echo Building FastRenderer-Android...

if "%ANDROID_NDK_HOME%"=="" (
    echo ERROR: ANDROID_NDK_HOME is not set
    echo Set it to your NDK installation path, e.g.:
    echo   set ANDROID_NDK_HOME=C:/Users/xxx/AppData/Local/Android/Sdk/ndk/28.2.13676358
    exit /b 1
)

echo ANDROID_NDK_HOME=%ANDROID_NDK_HOME%

REM Configure
cmake --preset android-release -S . -DCMAKE_TOOLCHAIN_FILE="%ANDROID_NDK_HOME%/build/cmake/android.toolchain.cmake"

if %ERRORLEVEL% neq 0 (
    echo Configure FAILED
    exit /b %ERRORLEVEL%
)

REM Build
cmake --build --preset android-release

if %ERRORLEVEL% equ 0 (
    echo ───────────────────────────────────────────
    echo Build SUCCESS
    echo Output: build/android-release/libFastRenderer-Android.so
    echo ───────────────────────────────────────────
) else (
    echo Build FAILED
    exit /b %ERRORLEVEL%
)