#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "${script_dir}/../.." && pwd)"
build_dir="${repo_root}/build/android-arm64-v8a"

find_ndk() {
    if [[ -n "${ANDROID_NDK_HOME:-}" && -f "${ANDROID_NDK_HOME}/build/cmake/android.toolchain.cmake" ]]; then
        printf "%s\n" "${ANDROID_NDK_HOME}"
        return 0
    fi

    if [[ -n "${ANDROID_NDK_ROOT:-}" && -f "${ANDROID_NDK_ROOT}/build/cmake/android.toolchain.cmake" ]]; then
        printf "%s\n" "${ANDROID_NDK_ROOT}"
        return 0
    fi

    if [[ -n "${ANDROID_HOME:-}" && -d "${ANDROID_HOME}/ndk" ]]; then
        local latest_ndk
        latest_ndk="$(find "${ANDROID_HOME}/ndk" -mindepth 1 -maxdepth 1 -type d | sort -V | tail -n 1)"
        if [[ -n "${latest_ndk}" && -f "${latest_ndk}/build/cmake/android.toolchain.cmake" ]]; then
            printf "%s\n" "${latest_ndk}"
            return 0
        fi
    fi

    return 1
}

ndk_root="$(find_ndk || true)"
if [[ -z "${ndk_root}" ]]; then
    cat >&2 <<'EOF'
[ERROR] Android NDK not found.
Set ANDROID_NDK_HOME or ANDROID_NDK_ROOT to an installed NDK, or install one under $ANDROID_HOME/ndk.
EOF
    exit 1
fi

cmake -S "${repo_root}" -B "${build_dir}" \
    -DCMAKE_TOOLCHAIN_FILE="${ndk_root}/build/cmake/android.toolchain.cmake" \
    -DANDROID_ABI=arm64-v8a \
    -DANDROID_PLATFORM=android-24 \
    -DANDROID_STL=c++_shared \
    -DCMAKE_BUILD_TYPE=Release \
    -DNDD_BUILD_ANDROID_JNI=ON \
    -DUSE_NEON=ON

cmake --build "${build_dir}" --target endee_jni --parallel

printf "[INFO] Built %s/arm64-v8a/libendee.so\n" "${build_dir}"
