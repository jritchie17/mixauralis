cmake_minimum_required(VERSION 3.15)

project(Auralis VERSION 0.1.0)

# Find the JUCE library - adjust the path if needed
set(JUCE_PATH "../JUCE" CACHE PATH "Path to JUCE library")
if(NOT EXISTS ${JUCE_PATH})
    message(FATAL_ERROR "JUCE not found at ${JUCE_PATH}. Set JUCE_PATH to your local JUCE directory.")
endif()
add_subdirectory(${JUCE_PATH} JUCE EXCLUDE_FROM_ALL)

# Basic application setup
juce_add_gui_app(Auralis
    VERSION 0.1.0
    PRODUCT_NAME "Auralis")

# Source files
target_sources(Auralis PRIVATE
    Source/MainApp.cpp
    Source/MainApp.h
    Source/MainWindow.h
    Source/UI/MainComponent.cpp
    Source/UI/MainComponent.h
    Source/UI/ChannelsComponent.cpp
    Source/UI/ChannelsComponent.h
    Source/UI/ChannelStripComponent.cpp
    Source/UI/ChannelStripComponent.h
    Source/UI/LevelMeter.h
    Source/UI/LoudnessMeterComponent.cpp
    Source/UI/LoudnessMeterComponent.h
    Source/UI/FXBusesComponent.cpp
    Source/UI/FXBusesComponent.h
    Source/UI/GroupBusComponent.cpp
    Source/UI/GroupBusComponent.h
    Source/UI/MasterBusComponent.cpp
    Source/UI/MasterBusComponent.h
    Source/UI/RoutingComponent.cpp
    Source/UI/RoutingComponent.h
    Source/UI/SoundcheckPanel.cpp
    Source/UI/SoundcheckPanel.h
    Source/UI/SettingsComponent.h
    Source/UI/SettingsComponent.cpp
    Source/UI/AudioSettingsDialog.h
    Source/UI/AudioSettingsDialog.cpp
    Source/Audio/AudioEngine.cpp
    Source/Audio/AudioEngine.h
    Source/Audio/ChannelProcessor.cpp
    Source/Audio/ChannelProcessor.h
    Source/Audio/FXBusProcessor.cpp
    Source/Audio/FXBusProcessor.h
    Source/Audio/GroupBusProcessor.cpp
    Source/Audio/GroupBusProcessor.h
    Source/Audio/MasterBusProcessor.cpp
    Source/Audio/MasterBusProcessor.h
    Source/Routing/RoutingManager.cpp
    Source/Routing/RoutingManager.h
    Source/Audio/TunerProcessor.cpp
    Source/Audio/TunerProcessor.h
    Source/Audio/TruePeakLimiterProcessor.cpp
    Source/Audio/TruePeakLimiterProcessor.h
    Source/Soundcheck/SoundcheckEngine.cpp
    Source/Soundcheck/SoundcheckEngine.h
    Source/Soundcheck/ToneProfiles.h
    Source/State/SessionManager.h
    Source/State/SessionManager.cpp
    Source/Utils/BlackwayLookAndFeel.h
    Source/FX/TrimProcessor.cpp
    Source/FX/TrimProcessor.h
    Source/FX/GateProcessor.cpp
    Source/FX/GateProcessor.h
    Source/FX/EQProcessor.cpp
    Source/FX/EQProcessor.h
    Source/FX/CompressorProcessor.cpp
    Source/FX/CompressorProcessor.h
    Source/FX/ReverbProcessor.cpp
    Source/FX/ReverbProcessor.h
    Source/FX/DelayProcessor.cpp
    Source/FX/DelayProcessor.h
    Subscription/SubscriptionManager.h
    Subscription/SubscriptionManager.cpp
    Tests/SessionRoundTripTest.h)

# JUCE modules
target_compile_definitions(Auralis
    PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:Auralis,JUCE_PRODUCT_NAME>"
    JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:Auralis,JUCE_VERSION>")

# Add include directories
target_include_directories(Auralis PRIVATE ${CMAKE_SOURCE_DIR})

# Link JUCE modules
target_link_libraries(Auralis
    PRIVATE
    juce::juce_analytics
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_core
    juce::juce_cryptography
    juce::juce_data_structures
    juce::juce_dsp
    juce::juce_events
    juce::juce_graphics
    juce::juce_gui_basics
    juce::juce_gui_extra
    juce::juce_osc)

# Add platform-specific settings
juce_generate_juce_header(Auralis) 

# Copy assets to the build directory
add_custom_command(TARGET Auralis POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory
                   ${CMAKE_SOURCE_DIR}/Assets
                   $<TARGET_FILE_DIR:Auralis>/Assets) 

# Enable unit tests
add_definitions(-DJUCE_ENABLE_UNIT_TESTS=1) 