# SONARWRAITH — glitch / atmos / sonar-blip synth (VST3)

A synth plugin built for dark, glitchy, underground beats: bubbly sonar pings,
wet atmospheric tails, blown-out distortion and stuttering chaos, all in one
instrument. VST3 + Standalone, works in FL Studio.

```
OSC (morph sine>tri>saw>square, fold, unison, sub, noise)
  <- FM operator          <- PING pitch envelope        <- BUBBLER random re-triggers
  -> FILTER (LP/BP/HP/NOTCH, env + LFO) -> AMP ADSR x bubble gate
  -> DRIVE -> CRUSH -> GLITCH -> SONAR DELAY -> ATMOS (shimmer reverb) -> out
```

---

## 1. Get the plugin (pick one)

### Option A: Build in the cloud (nothing to install)
1. Make a free GitHub account, create a new **private** repo and upload this whole folder
   (including the hidden `.github` folder, so drag the folder in, not just the files).
2. Open the repo's **Actions** tab. The "Build SonarWraith" job starts on its own (about 10 min).
3. When it's green, click the run and download **SonarWraith-Windows** at the bottom.
4. Unzip it and copy `VST3/SonarWraith.vst3` (the whole folder) to
   `C:\Program Files\Common Files\VST3\`

### Option B: Build on your PC
1. Install **Visual Studio 2022 Community** (free) and tick **"Desktop development with C++"**.
2. Install **Git for Windows** (JUCE gets downloaded with it the first time you build).
3. **Extract the zip first** (right-click → Extract All), then double-click **`build.bat`** in the extracted folder.
   A build window opens and stays open. It checks your setup, tells you exactly what's missing, and shows each step. The first build takes about 5–15 minutes.
4. A window opens on `build\SonarWraith_artefacts\Release\VST3`. Copy `SonarWraith.vst3` to
   `C:\Program Files\Common Files\VST3\`

### Load it in FL Studio
Go to **Options → Manage plugins → Find installed plugins**, then tick the star next to
SonarWraith. Add it from the Channel Rack (**+ → SonarWraith**). There's also a standalone
`.exe` in the `Standalone` folder for jamming without a DAW.

---

## 2. What makes it different

| Module | What it does | Why it matters for this sound |
|---|---|---|
| **PING** | Pitch envelope that starts the note `Ping` semitones away and snaps to pitch over `Ping Time` | The core *sonar blip*. +12 to +36 st with 15–60 ms gives the classic "bloop". Negative values give laser zaps. |
| **BUBBLER** | Keeps re-firing the ping while you hold a note, at `Rate` Hz with `Chance` probability, scattering pitch by up to `Scatter` semitones **locked to minor pentatonic**, gated by `Gate` | Hold one note and you get endless random bubbling melodies that stay dark and in key. |
| **FM + Fold** | Phase-mod operator (`FM Ratio`, `FM Amt`) + sine wavefolder | Glassy bells, metallic pings, and gritty harmonics. Non-integer ratios like 3.5 or 7 sound haunted. |
| **GLITCH** | Synced to your DAW's tempo. At every grid step it may grab the last slice and stutter it: forwards or reversed, at normal, double or half speed | Instant stutter edits with no manual chopping. |
| **SONAR DELAY** | Tempo-synced ping-pong delay with warbling tape modulation, filtered feedback and a soft-saturated loop (feedback above ~95% self-oscillates) | Pings that bounce and smear into the distance. |
| **ATMOS** | 8-line FDN reverb with diffusion, modulation and **shimmer** (octave-up pitch shifting in the loop) | Huge, haunted, angelic or underwater spaces. |
| **DRIVE** | TANH, HARD, FOLD, RECTIFY (octave-up fuzz) and BLOWN (biased, blown-speaker crunch) | From warm to completely destroyed. |
| **CRUSH** | Bit depth + sample-rate reduction | Lo-fi digital grit, aliasing sparkle. |
| **MUTATE** | Randomizes the whole patch, but only within dark/glitchy/bubbly ranges | Hit it until something weird and good comes out. |

All the timing is synced to the host tempo: LFO sync, glitch grid and delay time.

---

## 3. Factory presets
- **Sonar Ping**: clean sine blip with ping-pong echoes. Play sparse 8ths.
- **Bubble Machine**: *hold* notes and let the Bubbler play the melody.
- **Wraith Pad**: 4-voice saw unison, drift LFO, shimmer reverb, light glitching.
- **Glitch Bell**: FM bell with bitcrush, 1/32 stutters and reversed and warped repeats.
- **Blown Sonar Bass**: mono/glide sine + sub with a +36 st ping into BLOWN drive. 808-style slides.
- **Crushed Blips**: downward square zaps, 5-bit crush.
- **Underwater Radar**: high-resonance filter pings, sample & hold filter, long warbly delay.
- **Haunted Arp Chaos**: everything on at once. Hold a chord.
- **Rectified Ghost Lead**: legato glide lead with octave-up rectifier fuzz.

## 4. Sound-design recipes
- **Classic sonar blip:** Wave 0 (sine) · Ping +24 · Ping Time 30–50 ms · Sustain 0 · Decay 400 ms · Delay 1/8D, FB 50% · Atmos 30%.
- **Water-drop bubbles:** Ping +14 · Bubbler Rate 8–12 Hz · Scatter 12 · Gate 80–100% · Filter LP with Reso 0.6 and Env Amt +0.4.
- **Radar ring:** Filter LP, Reso 0.85+, Cutoff ~900 Hz, Env Amt +0.5. The filter itself rings like sonar.
- **Glitched atmosphere:** any pad → Glitch 1/16, Chance 20–40%, Reverse 40% → Atmos Size 0.9, Shimmer 0.4.
- **Destroyed:** Drive FOLD or BLOWN at 0.6+, then Crush 6 bits / 6 kHz at 50% mix to keep some body.
- Use **Width** above 100% on pads and keep it at 100% or below on basses.

---

## 5. Project layout
```
Source/ParamDefs.h      parameter table (single source of truth)
Source/SonarEngine.h    all DSP, no JUCE dependency
Source/Presets.h        factory presets
Source/PluginProcessor  JUCE wrapper, MIDI, tempo sync, programs, MUTATE
Source/PluginEditor     custom UI (look & feel, panels, oscilloscope)
Tests/render_demo.cpp   offline renderer: g++ -O2 -std=c++17 -ISource Tests/render_demo.cpp
Tests/plugin_test.cpp   headless plugin test + GUI snapshot (cmake -DSW_BUILD_TESTS=ON)
```
To add a parameter, add one line to `SW_PARAMS` in `ParamDefs.h`, use it in the engine,
and add it to a `Panel` in `PluginEditor.cpp`.

Company name and plugin codes are set in `CMakeLists.txt` (`COMPANY_NAME`, `PLUGIN_CODE`).
Built with JUCE 8 (AGPLv3 / JUCE licence). If you ever sell the plugin, check JUCE's licence terms first.
