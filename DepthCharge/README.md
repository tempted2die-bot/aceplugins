# DEPTHCHARGE: 808 quake + glitch / atmos / sonar synth (VST3)

Everything SonarWraith does (bubbly sonar blips, glitches, atmospheric FX), plus a
dedicated **808 engine** for wide, clipped trap 808s and wobbling, growling,
dubstep-style 808s. VST3 + Standalone, works in FL Studio.

```
                 ┌─ 808 QUAKE (below the split) ─────────────────────────────────────────┐
 MIDI ─ split ──>│ core (sine>tri>square, L/R phase spread) <- FM growl                  │
                 │ pitch <- note + SLIDE + PUNCH drop + wobble                            │
                 │ -> fold -> DIRT (clip/tube/fold/fuzz/shred) -> tone -> KNOCK           │
                 │ -> GROWL formants -> wobble filter -> + clean sub -> amp               │
                 ├─ SONAR SYNTH (above the split) ───────────────────────────────────────┤
                 │ morph osc + FM + PING + BUBBLER -> filter -> amp                       │
                 └───────────────────────────────────────────────────────────────────────┘
   -> DRIVE -> CRUSH -> GLITCH (stutter / GAP / TAPE STOP) -> SONAR DELAY -> ATMOS -> out
```

---

## 1. Get the plugin

### Option A: Build in the cloud (nothing to install)
1. Create a new **private** GitHub repo and upload everything in this folder, including `.github`.
   It's fine if the files end up inside a subfolder; the build finds them.
2. Open the **Actions** tab and wait for "Build DepthCharge" to turn green (about 10 min).
3. Click the run and download **DepthCharge-Windows** at the bottom.
4. Copy `VST3/DepthCharge.vst3` (the whole folder) to `C:\Program Files\Common Files\VST3\`

### Option B: Build on your PC
1. Install **Visual Studio 2022 Community** with **"Desktop development with C++"**, plus **Git for Windows**.
2. **Extract the zip first**, then double-click `build.bat` in the extracted folder.
   The build window stays open and tells you exactly what's missing, if anything.
3. Copy `DepthCharge.vst3` into `C:\Program Files\Common Files\VST3\`

In FL Studio, go to **Options → Manage plugins → Find installed plugins**, then add DepthCharge
from the Channel Rack. It installs alongside SonarWraith; they're separate plugins.

---

## 2. Playing it

The **Split** is set to MIDI note 48 (**C4 in FL's piano roll**) by default:
- Keys **below C4** play the **808**. The Octave knob is -1, so FL's C3 sounds as a C1 808 at 32.7 Hz.
- Keys **at C4 and above** play the **sonar synth**.
- Turn Split all the way down (**OFF**) to play both layers on every key, or set Synth Level to 0 for
  808 only. The 808-only presets already do this.

**808 slides:** in LEGATO mode (the default), overlap two notes in the piano roll and the 808
slides between them with no new punch. The **Slide** knob sets how long the slide takes.

---

## 3. The 808 section

| Panel | Controls | Tips |
|---|---|---|
| **808 CORE** | Octave, Shape (sine → triangle → square), **Punch** (pitch drop in semitones), **Punch Time**, Click, **Spread** (L/R phase offset), **Clean Sub** | Your reference 808 drops +17 st over about 100 ms, and Spread at 60–90° gives its very wide stereo. |
| **808 DIRT** | Mode: **CLIP** (squared), **TUBE** (warm, even harmonics), **FOLD** (notchy), **FUZZ** (octave-up), **SHRED** (folded + crushed). Dirt, Fold, Asym, Tone, **Knock** | **Asym** adds even harmonics that make an 808 sound fat. **Knock** boosts harmonics 2–4 and follows the note's pitch, so the 808 comes through on phone speakers. **Clean Sub** adds a clean, centered sine under the distortion so the low end stays solid. |
| **FILTER + FM** | Cutoff, Reso, **FM Growl**, FM Ratio | FM at ratio 1 or 2 makes a gritty growl. Odd ratios like 3 sound like screeching metal. |
| **GROWL** | Vowel (U-O-A-E-I), Formant Q, Growl amount | A vowel filter. Sweep Vowel and the 808 sounds like it's talking: "yoy", "wow". |
| **WOBBLE** | Rate (synced to your tempo, 1/1 to 1/32, triplets, dotted), Shape (SINE, TRI, SAW DN, SAW UP, SQUARE, **YOI**, RANDOM), **Target** (FILTER, GROWL, FM, FOLD, PITCH, ALL), Depth, Retrig | This is the dubstep part. **ALL** wobbles the filter, growl and FM together. Automate **Rate** in FL to switch wobble speeds per bar. PITCH with very low depth makes rattles. |
| **808 AMP + SLIDE** | Decay, Sustain, Release, Mode (POLY/MONO/LEGATO), Slide | Sustain at 100% gives long held 808s. For a decaying 808, set Sustain to 0 and use Decay. |
| **LAYERS** | 808 Level, Synth Level, Split, **Sub Mono** | Spread makes the sub wide too. **Sub Mono** keeps the lows mono and leaves the width up top, which is safer for club and car systems. |

New FX for this plugin:
- **Glitch GAP**: hard dropouts where the whole beat cuts out.
- **Glitch TAPE STOP**: the sound slows down and grinds to a halt.
- **FX Low Cut** (Master): keeps the 808 lows out of the delay and reverb so the bass stays tight.
- **Bend Range** (Master): sets the pitch-wheel range. Set it to 12 for octave dives.

---

## 4. Presets
**808:**
- **Wide Clip 808** is fitted to your reference 808: same pitch drop, clipped tone and stereo width.
- **Wobble Wraith 808** is a dubstep wobble with all targets on 1/8.
- **Yoi Talker 808** uses the YOI shape on the growl, so it sounds like it's talking.
- **Depth Charge** has a huge punch and click, SHRED distortion, and random tape stops and gaps.
- **Rattle Snake 808** is a wide clip with a 1/32 pitch rattle.
- **Glide Bully 808** is a long-sliding tube 808 with Sub Mono on.
- **Screamer 808** has an FM screech on 1/8T.

**808 + synth (split at C4):** Sonar Over 808, Bubble Quake, Haunted Drop.

**Synth only:** Sonar Ping, Bubble Machine, Wraith Pad, Glitch Bell, Underwater Radar, Haunted Arp Chaos.

**MUTATE** randomizes the synth, the 808 and the FX, but keeps your split and level settings.

## 5. Recipes
- **Your reference 808:** start from Wide Clip 808, play FL C3, and hold it.
- **Dubstep wobble:** Wobble on, Target ALL, Shape SINE, Rate 1/8 → change to 1/16T for the fills.
  Use Dirt FUZZ at about 50% and Clean Sub at about 50%.
- **"Yoy" growl:** Target GROWL, Shape YOI, Rate 1/4, Formant Q 0.7, Dirt TUBE.
- **Drop cutouts:** Glitch on, Grid 1/8, Chance 0, Gap 25%, Tape Stop 30%.
- **Trap glide:** LEGATO mode, Slide 120–160 ms. Overlap the notes in the piano roll.

---

## Project layout
```
Source/ParamDefs.h      parameter table (single source of truth)
Source/DepthEngine.h    all DSP: synth voices, Quake808 voices, FX (no JUCE dependency)
Source/Presets.h        factory presets
Source/PluginProcessor  JUCE wrapper, MIDI + key split, tempo sync, programs, MUTATE
Source/PluginEditor     tabbed UI (808 QUAKE / SONAR SYNTH pages + FX row)
Tests/render_demo.cpp   offline renderer (g++ -O2 -std=c++17 -ISource Tests/render_demo.cpp)
Tests/plugin_test.cpp   headless plugin test + GUI snapshots (cmake -DSW_BUILD_TESTS=ON)
```
Built with JUCE 8 (AGPLv3 / JUCE licence). If you ever sell the plugin, check JUCE's licence terms first.
