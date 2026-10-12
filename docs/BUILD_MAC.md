# Mac build and first host check

## Existing code: build this now
The source is in plugins/vocal-strip. It is a build kit, not an installed plugin.

1. On your Mac, clone the implementation branch and build the existing Vocal Strip:
```sh
git clone --branch pocket/implementation-plan https://github.com/livai230-cell/beatwhiz.git beatwhiz-pocket
cd beatwhiz-pocket
bash plugins/vocal-strip/BUILD_MAC.command
```
If you already have a clone, fetch and switch to pocket/implementation-plan there instead.
Alternatively, unzip Beatwhiz_Pocket_Source_and_Plan.zip, open Terminal in its beatwhiz folder and run the same bash command.
If Git/toolchain is unavailable, install Apple's Command Line Tools with `xcode-select --install` and finish the installation first.
The script finds CMake or downloads its checked official build; JUCE fetch requires internet on the first repo build.
For offline JUCE, copy third_party/JUCE-8.0.6.tar.gz from the old kit into plugins/vocal-strip/third_party/.
2. Close FL Studio before the installer replaces the plugin. It backs up an existing installation.
3. Wait for INSTALL COMPLETE. On failure inspect plugins/vocal-strip/logs/mac-build.log.
4. Scan plugins in FL Studio; load Beatwhiz Vocal Strip on the human lead.
5. Test playback, bypass at equal loudness and project save/reopen.

Installed path: ~/Library/Audio/Plug-Ins/VST3/Beatwhiz Vocal Strip.vst3.
This does not install Pocket; Vocal Strip currently only cleans, compresses and trims the human audio.

## Pocket: run after the next implementation pass
Pocket source files listed in NEXT_IMPLEMENTATION.md do not exist yet.
The template is intentionally named CMakeLists.txt.in and is not an active build.
Prerequisites: Apple Command Line Tools, CMake 3.22+ on PATH, Git and internet for JUCE.

From the repo root, after creating the specified files:
```sh
cmake -S plugins/pocket -B build-pocket -G "Unix Makefiles" \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES=arm64
cmake --build build-pocket --config Release \
  --target BeatwhizPocket_VST3 PocketTests --parallel 2
ctest --test-dir build-pocket -C Release --output-on-failure
```
arm64 is the first native Apple Silicon target; run FL Studio natively.
For an Intel/Rosetta host, use a separate universal build directory with
`'-DCMAKE_OSX_ARCHITECTURES=arm64;x86_64'`.
Do not switch architectures in an existing CMake build directory.

Expected bundle:
build-pocket/BeatwhizPocket_artefacts/Release/VST3/Beatwhiz Pocket.vst3.

For first installation only, close FL Studio, then:
```sh
pocket_bundle="build-pocket/BeatwhizPocket_artefacts/Release/VST3/Beatwhiz Pocket.vst3"
codesign --force --deep --sign - "$pocket_bundle"
codesign --verify --deep --strict "$pocket_bundle"
mkdir -p "$HOME/Library/Audio/Plug-Ins/VST3"
```
Copy the verified bundle in Finder into ~/Library/Audio/Plug-Ins/VST3.
If an older Pocket is present, move it to a backup folder before copying.
Local ad-hoc signing is for testing; this is not a notarized public installer.

Scan in FL Studio, insert Pocket on the AI bus, connect the human sidechain and confirm
Key activity and bounded gain reduction. Leave the human audible through its normal bus.
Export a short passage as well as testing live playback. Save/reopen the session.
Record sample rate, buffer size, Mac architecture, FL version and results in VALIDATION.md.

## Validation of this import
2026-10-12: imported processor, DSP, tests and installer compared byte-for-byte with the kit.
Installer shell syntax checked. Upstream JUCE 8.0.6 commit resolved; kit archive SHA-256 verified.
No new native Mac build, FL Studio host validation or vocal audition performed.
Original kit test results are retained separately as historical evidence.
