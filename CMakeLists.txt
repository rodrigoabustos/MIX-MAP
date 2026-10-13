cmake_minimum_required(VERSION 3.22)

# macOS: Apple Silicon only, minimum macOS 12. Must be set before project().
set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "Build architectures for macOS")
set(CMAKE_OSX_DEPLOYMENT_TARGET "12.0" CACHE STRING "Minimum macOS version")

project(MixMap VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# JUCE: downloaded from GitHub at a pinned version, unless a local copy is given.
set(MIXMAP_JUCE_DIR "" CACHE PATH "Optional: path to a local JUCE checkout")

if(MIXMAP_JUCE_DIR)
    add_subdirectory("${MIXMAP_JUCE_DIR}" juce)
else()
    include(FetchContent)
    FetchContent_Declare(
        JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        8.0.8
        GIT_SHALLOW    ON)
    FetchContent_MakeAvailable(JUCE)
endif()

juce_add_plugin(MixMap
    COMPANY_NAME                "Rodo Bustos"
    PLUGIN_MANUFACTURER_CODE    Rbus
    PLUGIN_CODE                 Mxmp
    BUNDLE_ID                   com.rodobustos.mixmap
    PRODUCT_NAME                "Mix Map"
    FORMATS                     VST3
    VST3_CATEGORIES             "Fx Tools"
    IS_SYNTH                    FALSE
    NEEDS_MIDI_INPUT            FALSE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    COPY_PLUGIN_AFTER_BUILD     FALSE)

target_sources(MixMap PRIVATE
    PluginProcessor.cpp
    PluginEditor.cpp)

target_compile_definitions(MixMap PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(MixMap
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)
