# Beatwhiz Vocal Strip — 0.1.0

A real JUCE VST3 source project for your human vocal channel. Includes an automatic Mac build/install script and executable
audio-processing tests. This download is a build kit, not a prebuilt Mac binary.

## On your Mac

1. Unzip this folder somewhere permanent, such as Documents.
2. Close FL Studio before installing or replacing this plugin.
3. Open Terminal, type `bash ` (including the space), drag `BUILD_MAC.command`
   into Terminal, and press Return. This works even if double-clicking the
   downloaded command file is unavailable.
4. If the script reports missing Apple Command Line Tools, run
   `xcode-select --install`, complete Apple's installer, then repeat step 3.
5. Wait for **INSTALL COMPLETE**. The script builds Apple Silicon + Intel,
   runs the DSP tests, applies a local signature and installs into
   `~/Library/Audio/Plug-Ins/VST3/Beatwhiz Vocal Strip.vst3`.
6. Open FL Studio's Plugin Manager, scan installed plugins, and select
   **Beatwhiz Vocal Strip** as a mixer effect. If your custom scan configuration
   omits your user VST3 folder, add the folder shown above.

If CMake is missing, the script downloads a private copy from Kitware's official
GitHub release and checks its published SHA-256. Internet is required for this
first download. The repository fetches JUCE 8.0.6 at a pinned commit on the first build. Alternatively, copy third_party/JUCE-8.0.6.tar.gz from your existing build kit into this folder for offline JUCE access. No Homebrew or manual source editing
is required. Existing plugin installations are moved to a backup folder before
replacement, with automatic rollback if the final move fails.

If anything fails, send `logs/mac-build.log`. Do not interpret the existence of
a source folder or build folder as a successful installation.

## Using it on your voice

Put it on your HUMAN lead insert, before shared reverb/delay sends. It filters
below 80 Hz, compresses, and sets output level. It does not process other inserts.

- Input: level into the processor, smoothed over 20 ms.
- Threshold: lower it until loud phrases show useful gain reduction.
- Ratio: begin around 2:1–3:1; the original 4:1 remains the default.
- Attack/release: original starting values are 15 ms / 100 ms.
- Output: manually match the loudness with FL's effect bypass.
- IN/OUT meters show sample peaks; GR shows the largest compressor reduction
  across channels, with a visual decay. Red OUT means peaks reached 0 dBFS.

There is no output limiter, auto gain, de-esser, gate, pitch correction,
alignment, AI matching or voice conversion. Excessive gain can clip downstream.
The two stereo channels compress independently; this version is primarily
for mono vocals or identical dual-mono recordings, not stereo master processing.
Compressor parameter changes update per block; only input/output gain is ramped.
Settings are stored in the DAW project. No telemetry, account or API is used.

## What was verified

See VALIDATION.txt for the original build kit’s recorded results, not a new run of this repository import. The audio engine is compiled
against real JUCE source and tested separately from the GUI/host wrapper.
The complete processor/editor source has passed a compiler syntax check.
The Mac VST3 still needs its first native build and FL Studio host check.
Its sound has not been auditioned on your recording.

## First FL Studio check

At low monitor volume: verify clean playback, move each control, confirm gain
reduction on louder phrases, compare bypass at equal loudness, save your project,
close/reopen it, and confirm the values return. Test mono/dual-mono vocals at your
normal project sample rate before relying on it in a finished session.

## Manual build

With CMake 3.22+ and a C++17 compiler:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel 2
ctest --test-dir build -C Release --output-on-failure
```

On Linux, JUCE also needs its usual development packages (X11, ALSA, FreeType,
Fontconfig, etc.). On macOS the deployment target is macOS 11 or later.

## Source and third-party notices

JUCE source: https://github.com/juce-framework/JUCE/tree/8.0.6
CMake build API: https://github.com/juce-framework/JUCE/blob/8.0.6/docs/CMake%20API.md
CMake: https://cmake.org/

The upstream JUCE dependency includes its own licences/notices.
Those terms still apply to JUCE and derived plugin distribution. The generated
Beatwhiz application source does not replace or grant JUCE licensing rights.
