# Exact next Codex task
Work on this repository branch. Read docs/POCKET_PLAN.md first.

## Already present
plugins/vocal-strip is restored source, not the finished Pocket plugin.
Reuse APVTS serialization and the existing build approach. Do not rename the old plugin IDs or silently change existing Vocal Strip sessions.
Do not copy the old channel-independent compressor into the stereo AI bus.

## Create these files
| File | Responsibility |
| --- | --- |
| plugins/pocket/CMakeLists.txt | Copy ../pocket-plan/CMakeLists.txt.in; actual build entry |
| plugins/pocket/Source/PluginProcessor.h | Processor interface, APVTS, engine and meter atomics |
| plugins/pocket/Source/PluginProcessor.cpp | Bus validation, callbacks, state, parameter handoff and factory |
| plugins/pocket/Source/PluginEditor.h | Main editor and APVTS attachment lifetimes |
| plugins/pocket/Source/PluginEditor.cpp | Compact main screen, advanced foldout, help panel, UI meter timer |
| plugins/pocket/Source/Parameters.h | Ordered parameter IDs, ranges/defaults and layout factory |
| plugins/pocket/Source/dsp/KeyDetector.h | Prepared detector HPF, linked absolute peak and fixed envelope |
| plugins/pocket/Source/dsp/PocketDSP.h | Per-sample gain computer, reduction smoothing, linked duck, trim, bypass |
| plugins/pocket/tests/PocketTests.cpp | Console runner; deterministic DSP and processor/state tests |
| plugins/pocket/VALIDATION.md | Actual commands, platform, result and remaining host checks |

Start with getName/prepareToPlay/releaseResources/processBlock/isBusesLayoutSupported,
hasEditor/createEditor, state callbacks, MIDI/program metadata and createPluginFilter.
Expose getBypassParameter for the APVTS bypass parameter; implement processBlockBypassed consistently.
Declare float processing; do not claim unsupported double precision.

Build bus shell before adding gain reduction; keep all audio behavior specified in POCKET_PLAN.
Use juce::AudioProcessorValueTreeState parameter attachments. Declare attachments after their widgets so destruction order is safe.
Tests must return nonzero on failure even in Release builds; do not rely on assert().

## Required tests
1. Main signal + absent/silent key: unity at default trim after release settles; no key leakage.
2. Main silence + loud key: output remains exactly silent.
3. Key above threshold: expected steady-state attenuation, capped at depthDb.
4. Equal L/R main stays equal; opposite-polarity L/R key still triggers.
5. Key ends: reduction releases toward zero; no hard mute.
6. Depth 0 and Output 0: unity/null after settled ramps.
7. Internal and host bypass: dry main, no sidechain leakage; switch automation without spikes.
8. 44.1/48/96 kHz; mono/stereo main; disabled/mono/stereo key; block sizes 0, 1, 64, 511, 1024 and larger than advertised.
9. Parameter extremes and NaN/Inf handling: finite output; no runaway gain.
10. Save/reload into a fresh processor: all parameters including bypass return; invalid state ignored.
11. No allocation on the processing path after prepare; verify with an allocation counter or equivalent instrumentation.
12. Compare block partitions for the same sample/control schedule; envelope remains sample-rate and block-size consistent.

Then build VST3 and verify on the user's Mac in FL Studio: scan, route external key,
observe GR, compare at matched loudness, automate, save/reopen, export offline.
Capture exact results. Do not call the plugin ready because engine tests alone pass.

## Finish
Return changed files, build output, test results and the exact VST3 bundle path.
If no Mac/FL runtime is available, label those checks pending and give docs/BUILD_MAC.md.
Do not publish binaries, replace the vault site or add cloud services.
