cmake_minimum_required(VERSION 3.22)
project(WraithCore VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(MSVC)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

include(FetchContent)
FetchContent_Declare(JUCE
  GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
  GIT_TAG 8.0.4
  GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

juce_add_binary_data(WraithCoreData SOURCES
  Resources/Anton-Regular.ttf
  Resources/AnonymousPro-Bold.ttf)

set(WC_SOURCES
  Source/PluginProcessor.cpp
  Source/PluginEditor.cpp
  Source/UI/Industrial.cpp
  Source/UI/Pages.cpp)

juce_add_plugin(WraithCore
  COMPANY_NAME "Ace Corp"
  BUNDLE_ID com.acecorp.wraithcore
  PLUGIN_MANUFACTURER_CODE AcCp
  PLUGIN_CODE WrCr
  FORMATS VST3 Standalone
  PRODUCT_NAME "WRAITHCORE"
  IS_SYNTH TRUE
  NEEDS_MIDI_INPUT TRUE
  NEEDS_MIDI_OUTPUT FALSE
  IS_MIDI_EFFECT FALSE
  EDITOR_WANTS_KEYBOARD_FOCUS TRUE
  COPY_PLUGIN_AFTER_BUILD FALSE
  VST3_CATEGORIES Instrument Synth)

target_sources(WraithCore PRIVATE ${WC_SOURCES})
target_include_directories(WraithCore PRIVATE Source)
target_compile_definitions(WraithCore PUBLIC
  JUCE_WEB_BROWSER=0
  JUCE_USE_CURL=0
  JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(WraithCore
  PRIVATE
    WraithCoreData
    juce::juce_audio_utils
  PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags)

# ---- optional headless test app (cmake -DWC_BUILD_TESTS=ON) ----
option(WC_BUILD_TESTS "Build the headless plugin test" OFF)
if(WC_BUILD_TESTS)
  juce_add_gui_app(WraithCoreTest PRODUCT_NAME "WraithCoreTest")
  target_sources(WraithCoreTest PRIVATE Tests/plugin_test.cpp ${WC_SOURCES})
  target_include_directories(WraithCoreTest PRIVATE Source)
  target_compile_definitions(WraithCoreTest PRIVATE JucePlugin_Name="WRAITHCORE" JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
  target_link_libraries(WraithCoreTest PRIVATE WraithCoreData juce::juce_audio_utils juce::juce_recommended_config_flags)
endif()
