cmake_minimum_required(VERSION 3.22)
if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version")
endif()
project(BeatwhizVocalStrip VERSION 0.1.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
# Use the existing build kit's archive offline when supplied; otherwise fetch
# the exact upstream commit represented by JUCE 8.0.6.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/JUCE-8.0.6.tar.gz")
    FetchContent_Declare(JUCE
        URL "${CMAKE_CURRENT_SOURCE_DIR}/third_party/JUCE-8.0.6.tar.gz"
        URL_HASH SHA256=95752f8e8b37d802b4c10182bc757de7d88f0c8899e879afa8798589be306a11)
else()
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 51a8a6d7aeae7326956d747737ccf1575e61e209)
endif()
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(BeatwhizVocalStrip
    COMPANY_NAME "Beatwhiz"
    BUNDLE_ID "com.beatwhiz.vocalstrip"
    PLUGIN_MANUFACTURER_CODE Btwz
    PLUGIN_CODE Bvst
    PRODUCT_NAME "Beatwhiz Vocal Strip"
    FORMATS VST3
    VST3_CATEGORIES Fx Dynamics
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE)
target_sources(BeatwhizVocalStrip PRIVATE Source/PluginProcessor.cpp Source/VocalDSP.h)
target_compile_definitions(BeatwhizVocalStrip PUBLIC
    JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(BeatwhizVocalStrip PRIVATE
    juce::juce_audio_utils juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)

enable_testing()
juce_add_console_app(VocalDSPTests PRODUCT_NAME "Vocal DSP Tests")
target_sources(VocalDSPTests PRIVATE tests/VocalDSPTests.cpp)
target_compile_definitions(VocalDSPTests PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
target_link_libraries(VocalDSPTests PRIVATE juce::juce_dsp)
add_test(NAME VocalDSPTests COMMAND VocalDSPTests)
