#!/bin/bash
# ──────────────────────────────────────────────
#  FastRenderer-Android Build Script (Linux)
#  Prerequisites: NDK, CMake, Ninja, ImGui, Preloader SDK
# ──────────────────────────────────────────────
set -e

echo "Building FastRenderer-Android..."

if [ -z "$ANDROID_NDK_HOME" ]; then
    echo "ERROR: ANDROID_NDK_HOME is not set"
    exit 1
fi

echo "ANDROID_NDK_HOME=$ANDROID_NDK_HOME"

# Configure
cmake --preset android-release -S .

# Build
cmake --build --preset android-release

echo "───────────────────────────────────────────────"
echo "Build SUCCESS"
echo "Output: build/android-release/libFastRenderer-Android.so"
echo "───────────────────────────────────────────────"