# Beatwhiz Pocket — smallest useful release
Prepared 2026-10-12.

## Evidence and starting point
Repository main inspected at f8df34f521fb8575690f3e47f358e49e3cb89ece:
README.md and index.html; no plugin sources, build system or open pull requests.
Only main was listed. The website stays at the root.

The existing Beatwhiz_Vocal_Strip_Mac_Build_Kit.zip supplies a better baseline than the earlier single SimpleVocalStrip.cpp:
- Source/PluginProcessor.cpp: processor, APVTS state, generic controls and meters.
- Source/VocalDSP.h: 80 Hz HPF, compressor, smoothed input/output gain.
- tests/VocalDSPTests.cpp: engine regression tests.
- CMakeLists.txt and BUILD_MAC.command: VST3 target and Mac installer.

Those source files are imported under plugins/vocal-strip. DSP, processor, tests and installer are unchanged. CMake now falls back to upstream JUCE 8.0.6 commit 51a8a6d7aeae7326956d747737ccf1575e61e209 when the kit archive is absent. Historical validation is explicitly labelled; it is not a new Mac test.

## Product promise
Make space for the human lead inside an AI-generated backing without flattening the backing or drowning the voice.

Version 0.1 is an external-sidechain ducking effect placed on the AI bus.
The human vocal is the detector input, not audio mixed into Pocket's output.
Keep Vocal Strip as a separate optional human-vocal processor. This avoids a confusing two-role plugin and lets each stage be tested independently.

## Routing
1. Human lead → optional Vocal Strip → vocal bus → master.
2. Human lead also sends sidechain-only to Pocket's Human Key input.
3. AI accompaniment or AI vocal bus → Pocket main input → master.
4. Human vocal reverb/delay remains on the existing shared send bus, outside the detector.

Use an AI vocal stem first if that is what masks the human. A plugin on one AI vocal stem cannot make space in unrelated instrumental inserts.
Use a grouped accompaniment bus only when the whole backing needs subtle ducking.
If replacing the AI lead, mute that lead while recording; ducking cannot remove a baked-in lead from a full mix.
Avoid sending the human's audible signal into the AI bus as well as its normal vocal path.

FL Studio setup: create a sidechain-only send from the human insert to the AI/Pocket insert, then assign that sidechain in the VST wrapper Processing/Connections panel if required. Confirm the Key meter moves. Mixer sends can be post-fader, so changes to the human level may also change triggering.

## First DSP modules
| Module | First implementation | Purpose |
| --- | --- | --- |
| HumanKeyDetector | fixed 120 Hz high-pass on each key channel; max absolute channel level; attack/release envelope | Reduce rumble triggers without cancelling opposite-polarity stereo keys |
| DuckGain | linked, bounded broadband gain on both AI channels | Open a small amount of space only during human phrases |
| OutputTrim | 20 ms smoothed manual gain | Compare fairly; no automatic makeup gain |
| Meters | main input/output sample peaks, key envelope and reduction in dB | Prove routing and spot excessive reduction |

For detector amplitude e: L = 20 log10(max(e, 1e-6)).
Requested reduction = clamp((L - thresholdDb) * (1 - 1/2), 0, depthDb).
Ratio is fixed at 2:1 for v0.1.
Smooth requested reduction using attack/release coefficients exp(-1/(timeSeconds*sampleRate)).
Apply g = 10^(-reductionDb/20) equally to L/R, then output trim.
Use a separate fixed fast detector envelope (1 ms attack, 50 ms release); the user Attack/Release controls the gain envelope. Both stages run per sample.
No lookahead; declared latency 0. The first consonant can precede the duck.

Missing/disabled key: target zero reduction; release existing reduction smoothly.
Never substitute the AI main signal for the missing key.
Digital silence: same release-to-unity behavior.
Threshold does not detect a person semantically: noise above threshold can trigger it.

## Stable parameter contract
IDs and order become permanent when shipped.
| ID | Label | Range | Default |
| --- | --- | --- | --- |
| thresholdDb | Trigger | -60 to 0 dBFS | -30 |
| depthDb | Pocket | 0 to 6 dB | 2 |
| attackMs | Attack | 1 to 50 ms | 10 |
| releaseMs | Release | 40 to 600 ms | 180 |
| outputDb | Output | -12 to +6 dB | 0 |
| bypass | Bypass | boolean | false |

These are design starting values, not settings inferred from a recording.
Ramp continuous parameter targets over 20 ms; ramp internal bypass over 10 ms.
Depth=0 and Output=0 must be an exact dry path after ramps settle.
Bypass returns original main audio at unity, excluding trim. Keep detector running during internal bypass so re-entry behaves predictably.
Save APVTS state with root BeatwhizPocketState and schema version 1.

## JUCE target and bus contract
- C++17, CMake 3.22+, pinned JUCE 8.0.6 to reuse the existing kit.
- Target BeatwhizPocket, product Beatwhiz Pocket, manufacturer Btwz, plugin code Bpkt, bundle com.beatwhiz.pocket. Distinct from Vocal Strip.
- VST3 only for first Mac/FL Studio release. AU after VST3 validation. No Standalone sidechain detour.
- Mono or stereo main input with matching output; optional disabled/mono/stereo Human Key input; no MIDI.
- Obtain buses using getBusBuffer(buffer, true, 0/1) and main output with getBusBuffer(buffer, false, 0).
- Read the sidechain before any potentially overlapping output writes. Never clear or process it as an output channel.
- No allocations, locks, logging, file/network operations or coefficient factories in processBlock.
- Preallocate in prepareToPlay; handle zero-length and varying/oversized blocks safely with sample loops or bounded chunks.
- Publish meter snapshots with atomic stores. UI timer reads at 20–30 Hz, applies decay; audio thread never touches components.
- Sanitize non-finite input/control values to prevent NaN propagation. Reset detector/filter/gain state on prepare/reset.

## UI: one main screen and a routing help panel
Main screen: 640×360 starting layout, near-black, bone text and one muted red accent.
Header: BEATWHIZ POCKET, Bypass and ?.
Center: two large controls — Pocket and Trigger.
Bottom: Key level, reduction and Output level, plus compact Output trim.
Advanced foldout: Attack and Release.
No moving spectrum, four-column rack, waveform editor or decorative meters.

Help panel: short routing steps and three honest states:
- Sidechain disabled — connect Human Key.
- Sidechain enabled; no signal detected — play the human track/check routing.
- Key signal detected — show level.
Do not claim to distinguish an enabled-but-unrouted key from a silent performance.

## Roadmap with release gates
1. Import and build the existing Vocal Strip. Record native Mac/FL results; keep its product IDs stable.
2. Pocket bus shell and APVTS: main audio passes through; key is visible but never audible; session recall works.
3. Detector + bounded linked ducking + trim. Pass deterministic engine and processor bus tests.
4. Compact UI and routing help. Verify automation, bypass and recall in FL Studio.
5. Audition on one real dry human vocal, AI stem/backing and blend. Compare at matched loudness; reject pumping, disappearing ambience or softened diction. Tune defaults only from evidence.
6. Only after that: one broad dynamic presence-band cut, proposed center 2.5 kHz/Q 0.7, max 3 dB. Validate filter stability, automation and stereo image. It is not part of v0.1.

No pitch matching, performance grading, MIDI extraction, stem separation, voice conversion, master limiter, subscription or cloud account in this release.
A ducking plugin cannot repair timing/pitch differences or separate a voice embedded in a stereo master.

## Source references
- JUCE pinned CMake API: https://github.com/juce-framework/JUCE/blob/8.0.6/docs/CMake%20API.md
- JUCE plugin example: https://github.com/juce-framework/JUCE/tree/8.0.6/examples/CMake/AudioPlugin
- FL Studio wrapper routing: https://www.image-line.com/fl-studio-learning-content/fl-studio-online-manual/html/plugins/wrapper.htm
- FL Studio mixer routing: https://www.image-line.com/fl-studio-learning/fl-studio-online-manual/html/mixer.htm
