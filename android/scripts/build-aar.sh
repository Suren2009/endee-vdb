#!/usr/bin/env bash
set -euo pipefail

variant="${1:-release}"
if [[ "${variant}" != "release" && "${variant}" != "debug" ]]; then
    echo "[ERROR] Usage: $0 [release|debug]" >&2
    exit 1
fi

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
android_root="$(cd "${script_dir}/.." && pwd)"

endee_version="0.1.0"
if [[ -f "${android_root}/gradle.properties" ]]; then
    line="$(grep -E '^endeeVersion=' "${android_root}/gradle.properties" || true)"
    if [[ -n "${line}" ]]; then
        endee_version="${line#endeeVersion=}"
    fi
fi

if [[ -z "${ANDROID_HOME:-}" ]]; then
    if [[ -d "${HOME}/Android/Sdk" ]]; then
        export ANDROID_HOME="${HOME}/Android/Sdk"
    elif [[ -d "/usr/lib/android-sdk" ]]; then
        export ANDROID_HOME="/usr/lib/android-sdk"
    fi
fi

if [[ -z "${ANDROID_HOME:-}" ]]; then
    echo "[ERROR] ANDROID_HOME is not set." >&2
    exit 1
fi

if [[ -z "${ANDROID_NDK_HOME:-}" && -d "${ANDROID_HOME}/ndk" ]]; then
    latest_ndk="$(find "${ANDROID_HOME}/ndk" -mindepth 1 -maxdepth 1 -type d | sort -V | tail -n 1)"
    if [[ -n "${latest_ndk}" ]]; then
        export ANDROID_NDK_HOME="${latest_ndk}"
    fi
fi

capitalized="$(echo "${variant}" | awk '{print toupper(substr($0,1,1)) tolower(substr($0,2))}')"
aar_name="endee-vdb-${variant}-${endee_version}.aar"

cd "${android_root}"
./gradlew ":library:stage${capitalized}Aar" --no-daemon

dest="${android_root}/release/${aar_name}"
if [[ ! -f "${dest}" ]]; then
    echo "[ERROR] AAR was not produced at ${dest}" >&2
    exit 1
fi

printf "[INFO] AAR: %s\n" "${dest}"
