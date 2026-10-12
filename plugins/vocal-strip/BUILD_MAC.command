#!/bin/bash
# Build, test, sign locally, and install a universal Mac VST3. No sudo required.
set -euo pipefail
project_dir="$(cd "$(dirname "$0")" && pwd)"
cd "$project_dir"
mkdir -p logs
exec > >(tee "$project_dir/logs/mac-build.log") 2>&1
finish() {
    result=$?
    if [ "$result" -ne 0 ]; then
        echo "Build stopped. Nothing is ready to load unless INSTALL COMPLETE appears above."
        echo "Send logs/mac-build.log for diagnosis."
    fi
    if [ -t 0 ]; then read -r -p "Press Return to close..." _answer || true; fi
}
trap finish EXIT

if [ "$(uname -s)" != Darwin ]; then
    echo "Run this script on your Mac. This installer creates a macOS VST3."
    exit 1
fi
if ! xcrun --find clang++ >/dev/null 2>&1; then
    echo "Apple Command Line Tools are required. In Terminal run: xcode-select --install"
    echo "Complete Apple's installation, then run this script again."
    exit 1
fi

cmake_bin=""
if command -v cmake >/dev/null 2>&1; then
    cmake_bin="$(command -v cmake)"
elif [ -x /Applications/CMake.app/Contents/bin/cmake ]; then
    cmake_bin=/Applications/CMake.app/Contents/bin/cmake
else
    # Private project-local CMake; does not change system tools or PATH.
    cmake_version=3.31.6
    tools_dir="$project_dir/.build-tools"
    archive="cmake-${cmake_version}-macos-universal.tar.gz"
    cmake_bin="$tools_dir/cmake-${cmake_version}-macos-universal/CMake.app/Contents/bin/cmake"
    if [ ! -x "$cmake_bin" ]; then
        mkdir -p "$tools_dir"
        release="https://github.com/Kitware/CMake/releases/download/v${cmake_version}"
        echo "Downloading official CMake build tools (first run only)..."
        curl --fail --location --retry 2 "$release/$archive" -o "$tools_dir/$archive"
        curl --fail --location --retry 2 "$release/cmake-${cmake_version}-SHA-256.txt" \
            -o "$tools_dir/checksums.txt"
        expected="$(awk -v name="$archive" '$2 == name {print $1}' "$tools_dir/checksums.txt")"
        actual="$(shasum -a 256 "$tools_dir/$archive" | awk '{print $1}')"
        if [ -z "$expected" ] || [ "$expected" != "$actual" ]; then
            echo "CMake checksum verification failed. Stopping."
            exit 1
        fi
        tar -xzf "$tools_dir/$archive" -C "$tools_dir"
    fi
fi

echo "Building Beatwhiz Vocal Strip for Apple Silicon and Intel..."
"$cmake_bin" -S "$project_dir" -B "$project_dir/build-mac" \
    -DCMAKE_BUILD_TYPE=Release '-DCMAKE_OSX_ARCHITECTURES=arm64;x86_64'
"$cmake_bin" --build "$project_dir/build-mac" --config Release \
    --target BeatwhizVocalStrip_VST3 VocalDSPTests --parallel 2
ctest_bin="$(dirname "$cmake_bin")/ctest"
"$ctest_bin" --test-dir "$project_dir/build-mac" -C Release --output-on-failure

plugin="$project_dir/build-mac/BeatwhizVocalStrip_artefacts/Release/VST3/Beatwhiz Vocal Strip.vst3"
if [ ! -d "$plugin" ]; then echo "Expected VST3 bundle is missing."; exit 1; fi
codesign --force --deep --sign - "$plugin"
codesign --verify --deep --strict "$plugin"

plugin_dir="$HOME/Library/Audio/Plug-Ins/VST3"
mkdir -p "$plugin_dir"
staging="$(mktemp -d "$plugin_dir/.beatwhiz-install.XXXXXX")"
ditto "$plugin" "$staging/Beatwhiz Vocal Strip.vst3"
codesign --verify --deep --strict "$staging/Beatwhiz Vocal Strip.vst3"
destination="$plugin_dir/Beatwhiz Vocal Strip.vst3"
backup=""
if [ -e "$destination" ]; then
    backup_root="$HOME/Library/Application Support/Beatwhiz/Backups"
    mkdir -p "$backup_root"
    backup="$(mktemp -d "$backup_root/VocalStrip.XXXXXX")"
    mv "$destination" "$backup/Beatwhiz Vocal Strip.vst3"
fi
if ! mv "$staging/Beatwhiz Vocal Strip.vst3" "$destination"; then
    if [ -n "$backup" ]; then mv "$backup/Beatwhiz Vocal Strip.vst3" "$destination"; fi
    echo "Installation failed; previous plugin restored if present."
    exit 1
fi
rmdir "$staging"
echo "INSTALL COMPLETE: $destination"
echo "Open FL Studio's Plugin Manager and scan for Beatwhiz Vocal Strip."
echo "Load it as an effect on your HUMAN lead-vocal mixer insert."
echo "The build log is in logs/mac-build.log."
