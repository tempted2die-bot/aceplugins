cmake_minimum_required(VERSION 3.22)
project(SonarWraith VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(MSVC)
  # static runtime: the plugin works on machines without the VC++ redistributable
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# JUCE is downloaded automatically the first time you configure.
# (Set FETCHCONTENT_SOURCE_DIR_JUCE=/path/to/JUCE to use a local copy instead.)
include(FetchContent)
FetchContent_Declare(JUCE
  GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
  GIT_TAG 8.0.4
  GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(SonarWraith
  COMPANY_NAME "Ace Corp"
  BUNDLE_ID com.acecorp.sonarwraith
  PLUGIN_MANUFACTURER_CODE AcCp
  PLUGIN_CODE SnWr
  FORMATS VST3 Standalone
  PRODUCT_NAME "SonarWraith"
  IS_SYNTH TRUE
  NEEDS_MIDI_INPUT TRUE
  NEEDS_MIDI_OUTPUT FALSE
  IS_MIDI_EFFECT FALSE
  EDITOR_WANTS_KEYBOARD_FOCUS FALSE
  COPY_PLUGIN_AFTER_BUILD FALSE
  VST3_CATEGORIES Instrument Synth)

target_sources(SonarWraith PRIVATE
  Source/PluginProcessor.cpp
  Source/PluginEditor.cpp)

target_include_directories(SonarWraith PRIVATE Source)

target_compile_definitions(SonarWraith PUBLIC
  JUCE_WEB_BROWSER=0
  JUCE_USE_CURL=0
  JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(SonarWraith
  PRIVATE
    juce::juce_audio_utils
  PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags)

# ---- optional headless test app (cmake -DSW_BUILD_TESTS=ON) ----
option(SW_BUILD_TESTS "Build the headless plugin test" OFF)
if(SW_BUILD_TESTS)
  juce_add_gui_app(SonarWraithTest PRODUCT_NAME "SonarWraithTest")
  target_sources(SonarWraithTest PRIVATE Tests/plugin_test.cpp Source/PluginProcessor.cpp Source/PluginEditor.cpp)
  target_include_directories(SonarWraithTest PRIVATE Source)
  target_compile_definitions(SonarWraithTest PRIVATE JucePlugin_Name="SonarWraith" JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
  target_link_libraries(SonarWraithTest PRIVATE juce::juce_audio_utils juce::juce_recommended_config_flags)
endif()
