# Auralis - Live Sound Automation for Worship

Auralis is a modern mixing assistant that makes live worship services sound professional with minimal effort. 

## Features
- 32 Input Channels
- Smart Soundcheck: automatic EQ, compression, gate, leveling
- Real VST3 plugin chains for every channel
- FX Buses for vocals, instruments, drums
- Master Bus loudness optimization
- One-knob vocal tuning
- Manual and assisted input routing
- Subscription management and offline grace period

## Technologies
- JUCE Framework (C++)
- Modular DSP architecture
- Lightweight tone fingerprint analysis (no heavy ML yet)
- Stripe for subscription management

## Roadmap
- Advanced ML models (future)
- VST3 plugin marketplace
- Multi-user profiles
- Full automation (auto mute during speaking, livestream output tailoring)

## Getting Started
See `DEV_PLAN.md` for detailed architecture and setup.

The build assumes JUCE is located in `../JUCE` relative to this repository. If
your JUCE copy lives elsewhere, pass `-DJUCE_PATH=/path/to/JUCE` to CMake when
configuring the project.
