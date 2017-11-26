<img src="https://i.imgur.com/1D1ffjO.png" width="50%">

# RJModules
Various DIY modules made by Rich Jones for use with [VCV Rack](https://github.com/VCVRack/Rack). So far, mostly simple utilites, hopefully some more interesting ones soon!

<!-- START doctoc generated TOC please keep comment here to allow auto update -->
<!-- DON'T EDIT THIS SECTION, INSTEAD RE-RUN doctoc TO UPDATE -->

- [RJModules](#rjmodules)
  - [Contents](#contents)
    - [Bit Crusher](#bit-crusher)
    - [Integers](#integers)
    - [Floats](#floats)
    - [Randoms](#randoms)
    - [Button](#button)
  - [Future Plans](#future-plans)
  - [Building](#building)
  - [License](#license)

<!-- END doctoc generated TOC please keep comment here to allow auto update -->

## Contents

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

## Future Plans

None of them actually exist yet, but I'm hoping this will project eventually contain:

  * FFTTuner - FFT / Tuner
  * VCMono - Combine two signals into one, modulated by VC
  * VCSplitter - Split one signal into two, modulated by VC
  * BPM - Dial in a pulse to a specific beats per minute. Also VC-able and "reset"-able.
  * Sidechain - Lower the volume of A based on B.
  * Autopanner - Given a signal, oscillate into two output channels
  * VCDryWet
  * DubEcho - Two delays and a spring.
  * Filter Delay
  * Vocoder, maybe?

## Building

First, clone and [make Rack for yourself](https://github.com/VCVRack/Rack#building).

Then, clone this repo into the `plugins` directory and run `make` from this directory.

## Related Projects

  * [Autopan](https://github.com/Miserlou/Autopan)

## License

(c) Rich Jones 2017, BSD.
