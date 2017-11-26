<img src="https://i.imgur.com/YbMssHR.png" width="100%">

# RJModules
Various DIY modules made by Rich Jones for use with [VCV Rack](https://github.com/VCVRack/Rack). So far, mostly simple utilities and effects, hopefully some more interesting ones soon!

<!-- START doctoc generated TOC please keep comment here to allow auto update -->
<!-- DON'T EDIT THIS SECTION, INSTEAD RE-RUN doctoc TO UPDATE -->


- [Contents](#contents)
  - [Supersaw](#supersaw)
  - [Bit Crusher](#bit-crusher)
  - [Integers](#integers)
  - [Floats](#floats)
  - [Randoms](#randoms)
  - [Button](#button)
  - [Splitter](#splitter)
  - [Splitters](#splitters)
  - [Panner](#panner)
  - [Filter Delay](#filter-delay)
  - [Left Right Mixer](#left-right-mixer)
- [Future Plans](#future-plans)
- [Building](#building)
- [Related Projects](#related-projects)
- [License](#license)

<!-- END doctoc generated TOC please keep comment here to allow auto update -->

## Contents

### Supersaw
<img src='https://i.imgur.com/ACIUKI0.png' width="25%" />
It's a supersaw! Frequency, detune and mix are all voltage controlled, and there's switches for phase, inversion and 2/3 OSC. There's also a reset button.

### Bit Crusher
<img src='https://i.imgur.com/tjKYMUn.png' width="50%" />

It's a bit crusher! Accepts control voltage, and sets a (voltage controlled) minimum bit depth for fine tuning.

### Integers
<img src='https://i.imgur.com/NRQjpmZ.png' width="25%" />
It generates three (voltage controlled) integers from -12 to +12!

### Floats
<img src='https://i.imgur.com/spQgKmr.png' width="25%" />
It generates three (voltage controlled) floats from -12 to +12!

### Randoms
<img src='https://i.imgur.com/CuM471K.png' width="50%" />

Generates three random values. The range of the values can be controlled via CV, but will default to (-12, +12) if CV values are empty/equal.

### Button
<img src='https://i.imgur.com/msNcs07.png' width="25%" />
It's literally just a big ass button with six outputs. You hit it, it sends a +12 reset signal.

### Splitter
<img src="https://i.imgur.com/bvJKVEn.png" width="25%" />
It's a 1 to 9-way splitter! You've got a signal - now send it everywhere!

### Splitters
<img src="https://i.imgur.com/NVWwfnZ.png" width="25%" />
If splitting one signal to many isn't your fancy, Splitters gives you a 5:10 splitter instead! Handy!

### Panner
<img src="https://i.imgur.com/4z5li8u.png" width="25%" />
Panner is a voltage controlled panner. Without CV, it pans a mono signal into left and right channels based on the value of the knob. Combine with an LFO to build an autopanner!

### Filter Delay
<img src="https://i.imgur.com/9CPtg6R.png" width="25%" />
A modification of the basic delay that filters each feedbacking pass. Kind of reggaeish, good for pads too.

### Left Right Mixer
<img src="https://i.imgur.com/UOidGVr.png" width="25%" />
A simple 12-to-2 mixer for mixing multiple stereo signals. With an additional overall voume knob.

## Future Plans

None of them actually exist yet, but I'm hoping this will project eventually contain:

  * FFTTuner - FFT / Tuner
  * VCMono - Combine two signals into "one", through two identical outputs, modulated by VC
  * VCWidener - Use the Haas effect to widen two inputs
  * BPM - Dial in a pulse to a specific beats per minute. Also VC-able and "reset"-able.
  * Sidechain - Lower the volume of A based on B.
  * VCDryWet
  * DubEcho - Two delays and a spring.
  * Ping Pong Delay (or maybe just a delay with seperate dry/wet outs that can feed to the panner?)
  * Vocoder, maybe?
  * Dedicated 808/kick circuit

## Building

First, clone and [make Rack for yourself](https://github.com/VCVRack/Rack#building).

Then, clone this repo into the `plugins` directory and run `make` from this directory.

## Related Projects

  * [Autopan](https://github.com/Miserlou/Autopan)

## License

(c) Rich Jones 2017, BSD.
