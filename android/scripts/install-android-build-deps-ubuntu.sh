#!/usr/bin/env bash
set -euo pipefail

if ! command -v apt-get >/dev/null 2>&1; then
    echo "[ERROR] This helper supports Ubuntu/Debian systems with apt-get." >&2
    exit 1
fi

sudo apt-get update
sudo DEBIAN_FRONTEND=noninteractive apt-get install -y \
    cmake \
    clang \
    default-jdk \
    libstdc++-14-dev \
    google-android-ndk-r26c-installer

ndk_root="/usr/lib/android-sdk/ndk/26.2.11394342"
if [[ -f "${ndk_root}/build/cmake/android.toolchain.cmake" ]]; then
    echo "[INFO] Android NDK installed at: ${ndk_root}"
    echo "[INFO] Export it with:"
    echo "       export ANDROID_NDK_HOME=${ndk_root}"
else
    echo "[WARN] NDK package installed, but expected toolchain file was not found at ${ndk_root}." >&2
fi
