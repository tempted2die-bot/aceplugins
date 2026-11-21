# WRAITHCORE

**Sonar synth + 808 QUAKE + Snap-Heap-style FX rack.** WRAITHCORE merges SonarWraith and DepthCharge
into one instrument with a modular effects rack, a mod matrix, a 108-preset browser and an
industrial/glitch interface. It's built for dark, glitchy, underground beats.

VST3 + Standalone · Windows / macOS / Linux · JUCE 8

---

## What's inside

### SYNTH page: the sonar layer
- **OSC A**: wave morphs sine → triangle → saw → square, with fold, up to 4 unison voices, sub and noise.
- **OSC B**: own wave, octave, semitone and fine controls. It mixes in as **ADD**, **RING** (ring mod with OSC A) or **SYNC** (hard sync to OSC A).
- **FM + PING**: FM operator plus a pitch drop at the start of each note, which makes the sonar blip.
- **BUBBLER**: keeps re-firing the ping while a note is held, scattering pitch in minor pentatonic so it stays dark and in key.
- **FILTER**: LP12, LP24, BAND, HIGH, NOTCH, **COMB** or **ACID** (driven 4-pole), with drive, envelope and key tracking.
- **AMP**, **VOICE LFO** and **VOICE** (poly / mono / legato, glide).

### 808 QUAKE page
- **Punch**: pitch drop fitted to a real clipped 808 (+17 st over about 100 ms).
- **Spread**: L/R phase offset for wide 808s.
- **Clean Sub**: a clean sine under the distortion so the low end stays solid.
- **Knock**: boosts harmonics 2–4 and follows the note's pitch, so the 808 cuts through on small speakers.
- **Dirt**: CLIP, TUBE, FOLD, FUZZ or SHRED distortion.
- **FM Growl** and **GROWL** (vowel formants) for talking, dubstep-style 808s.
- **WOBBLE**: synced to your tempo, with 7 shapes including YOI. It can move FILTER, GROWL, FM, FOLD, PITCH or ALL.
- **Slide**: legato 808 glides between overlapping notes.
- **Sub Mono**: keeps the lows centered for club and car systems.
- **Split**: by default, keys below **C4 in FL** play the 808 and keys from C4 up play the synth. Turn Split to OFF to play both layers on every key.

### FX RACK page (like Kilohearts Snap Heap)
**2 lanes × 5 slots.** Pick any module in any slot, switch slots on and off, set each slot's mix, and
move modules with the `<` `>` buttons. Mod-matrix routings stay attached to a module when you move it.

Lane routing:
- **SERIAL A>B**
- **PARALLEL A+B**
- **SPLIT**: lows go to lane A and highs to lane B, with a crossover you can set. Use it to distort an 808's top without touching its sub.

Every lane has its own level and on switch, and the whole rack has a mix control.

26 modules:

| Group | Modules |
|---|---|
| DRIVE | **Distort** (6 modes, 2× oversampled), **Clipper** (2× OS), **Bitcrush** (jittered rate), **Blown Spkr** (torn cone + buzzing rattle), **Subharm** (octave-divider sub generator) |
| TONE | **Filter** (incl. ± comb), **Formant** (talking vowels + synced LFO), **EQ**, **Resonator** (tuned metal combs playing chords) |
| DYNAMICS | **OTT** (3-band upward + downward smash), **Pump** (synced sidechain duck), **Trancegate** (16 patterns) |
| SPACE | **Delay** (ping-pong + tape warble), **Reverb** (8-line FDN + shimmer), **Reverse** (backwards delay), **Chorus**, **Phaser** |
| GLITCH | **Datamosh** (bit-flips, stuck buffers, shuffled memory), **Stutter**, **Tapestop** (stops, dropouts, DJ spinbacks), **Grains** (granular cloud with random freezes), **Freq Shift** (Hilbert barber-pole shifter), **Ring Mod**, **Pitch** |
| CHARACTER | **Lo-Fi Tape** (wow, flutter, age, dropouts), **Stereo** (width, Haas, mono-below, pan) |

### MOD MATRIX page
- **4 macros**: BRIGHT, DIRT, SPACE and CHAOS. Every factory preset comes with these routings set, so they're useful straight away.
- **2 LFOs**: synced or free, 7 shapes.
- **Envelope follower.**
- **8 matrix rows**: sources are the macros, LFOs, envelope follower, mod wheel, velocity and a per-note random value. They can target any FX slot knob or mix, or 23 synth, 808 and rack parameters. Each row has an LED showing its live value.

### Presets
There are 108 factory presets in 9 folders: 808 + SYNTH splits, 808, BASS, LEAD, PLUCK, KEYS + BELLS,
PADS + ATMOS, ARP + SEQ, and FX + NOISE. Many are modelled on the reference beats:
- **Dungeon Sub**: a low G#0-range harmonic stack.
- **Cringe Stack**: a dense harmonic series.
- **Itchy Wide**, **Sleep Deprived** and **Multi-Dim**: wide 808s with dropouts.
- **Roten Grit**: broadband crushed grit.
- **Raw DVD**: punchy, glitched 808s.
- **2-Step Dive**: an 808 pitch dive.

Every preset is **loudness-matched**, peaks at or below 0 dBFS, and was checked offline for silence and glitches.

The **browser** has folder buttons and search. **SAVE** writes user presets to
`Documents/WRAITHCORE/User Presets`, and they show up in the USER folder. The `<` `>` buttons in the header
step through whatever list the browser is showing. **MUTATE** randomizes the synth, the 808 and the rack within
musical ranges.

### Pro details
- Sample-accurate MIDI, with modulation updated every 32 samples.
- Click-free FX slot changes: modules are created off the audio thread and handed over lock-free.
- Oversampled distortion.
- Optional 0 dBFS safety clipper.
- UI scaling from 70% to 130%.
- Full host automation: every knob, including all rack slots.
- About 9% of one CPU core with a worst-case patch (10 heavy modules, a 4-voice unison chord and an 808).

---

## Build

**GitHub (no installs).** Push this repo, then download **WraithCore-Windows** from the Actions tab.

**Windows PC.** Install Visual Studio 2022 Community (with "Desktop development with C++") and Git,
extract the folder, and double-click `build.bat`.

**Install.** Copy `WRAITHCORE.vst3` to `C:\Program Files\Common Files\VST3\`, then in FL Studio go to
Options → Manage plugins → Find installed plugins.

## Source layout
```
Source/DSP/ParamDefs.h     parameter table (base params + generated rack / mod / matrix params)
Source/DSP/FxCatalog.h     module names, knob labels, value mapping + readouts (shared by DSP and UI)
Source/DSP/FxModules.h     the 26 rack modules
Source/DSP/FxRack.h        lanes, routing, lock-free module hand-over
Source/DSP/SynthVoice.h    sonar synth voice      Source/DSP/Quake808.h   808 voice
Source/DSP/Engine.h        layers, key split, modulation, master
Source/DSP/Presets.h       preset library (+ PresetTrims.h, generated loudness trims)
Source/UI/*                industrial look & feel, pages, rack, matrix, browser
Tests/render_demo.cpp      offline render of every preset (+ --calibrate to regenerate loudness trims)
Tests/plugin_test.cpp      headless plugin test + GUI snapshots (cmake -DWC_BUILD_TESTS=ON)
```
Fonts: Anton and Anonymous Pro, both under the SIL Open Font License (see `Resources/`).
Built with JUCE 8 (AGPLv3 / JUCE licence). If you ever sell the plugin, check JUCE's licence terms first.
