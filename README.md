# ShifuVirtMix

All-in-one VST3 virtual guitar mixing utility built with **JUCE** and **C++17**, designed for DI/amp-sim virtual guitars (such as Odin III, Shreddage 3, Argent, and Rogue).

## Features
- **Factory Presets**: Built-in presets tailored for Odin III, Shreddage 3, Argent, Rogue, and Lead/Harmony buses.
- **High-Pass & Low-Pass Filtering (`Low Cut`, `High Cut`)**: Trims sub-bass rumble and high-end cabinet fizz.
- **Guitar-Voiced EQ (`Low Tight`, `380Hz Cut`, `3.2k Bite`)**: Tightens low-end palm mutes, tames boxy low-mids, and shapes pick articulation.
- **Transient Shaping (`Pick Attack`)**: Fast/slow envelope follower to add or tame initial string/pick transient snap.
- **Harmonic Saturation (`Grit`)**: Asymmetric soft-clipping waveshaper with level compensation.
- **Stereo Control (`Width`)**: Mid/side width adjustment for stereo virtual guitar buses.
- **Dynamics & Loudness (`Glue Thresh`, `Auto Loudness`, `Target dB`, `Out Trim`)**: Bus compression paired with smoothed RMS loudness leveling.

## Building Locally
Requires **CMake 3.22+** and a C++17 compiler (Visual Studio on Windows, Xcode on macOS, or GCC/Clang on Linux).

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --target ShifuVirtMix_VST3
```

## GitHub Actions
Pushing to `main` or `master` runs `.github/workflows/build-vst3.yml` and produces a `ShifuVirtMix.vst3` build artifact.
