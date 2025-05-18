# Auralis - Developer Plan

This document outlines the technical modules, responsibilities, and build order for Auralis Basic Tier.

---

## 1. Core Application Structure

- **/src**
  - `MainApp.cpp` / `MainApp.h` → JUCE Main Window and Startup
  - `UI/` → UI components
  - `Audio/` → Audio processing logic
  - `Routing/` → Input/Output assignment
  - `Soundcheck/` → Sound analysis and auto-correction
  - `FX/` → VST3 Plugins and FX Buses
  - `Subscription/` → Subscription management system
  - `State/` → State persistence (last settings, channel layouts, etc.)
  - `Utils/` → Reusable helpers

---

## 2. Modules

| Module | Responsibility |
|:--|:--|
| **Main Window** | Hosts tabbed navigation for Routing, Channels, FX Buses, Master, Settings. |
| **Routing Manager** | Assign hardware inputs -> Logical channels -> User Labels (Guitar, Vocal 1, etc.). |
| **Channel Strip** | Each channel holds:<br>Trim -> Gate -> EQ -> Compressor -> FX Send -> Tuner (optional). |
| **Soundcheck Engine** | Analyze ~30s audio input per channel, apply smart EQ, Comp, Gate settings. |
| **FX Bus Manager** | Vocal FX Bus, Instrument FX Bus, Drum FX Bus: Reverb, Delay loading. |
| **Master Bus** | Final polish before output. (Limiter, minor EQ). |
| **Subscription Manager** | Communicates with Stripe, locks features by plan, offline grace logic. |
| **UI Style Manager** | Unified font/color/style pulled from Blackway FX Kit assets. |

---

## 3. Build Order

| Phase | Tasks |
|:--|:--|
| **Phase 1** | Setup JUCE Project / Main Window / Tabbed Navigation |
| **Phase 2** | Build Routing Page and basic Channel View |
| **Phase 3** | Implement simple per-channel plugin chains |
| **Phase 4** | Build FX Bus Manager |
| **Phase 5** | Implement Master Bus with loudness meter |
| **Phase 6** | Implement Soundcheck Engine (simple version) |
| **Phase 7** | Build Subscription Manager |
| **Phase 8** | Polish UI with Blackway FX Kit styling |
| **Phase 9** | Beta Testing and Bug Fixing |

---

## 4. Additional Notes
- **Future-Proof**: Soundcheck Engine must be a separate service, NOT wired deep into audio pipeline, so it can be replaced with ML later.
- **Marketplace Ready**: All channel plugins should be swappable/upgradable without app crash risk.

---
