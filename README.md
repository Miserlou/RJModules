# RJModules

Various DIY modules I've made for [VCV Rack](https://github.com/VCVRack/Rack). So far, mostly simple utilites, hopefully some more interesting ones soon!

## Contents

### BitCrusher
<img src='https://i.imgur.com/tjKYMUn.png' width="50%" />

It's a bit crusher! Accepts control voltage, and sets a (voltage controlled) minimum bit depth for fine tuning.

### Integers
<img src='https://i.imgur.com/NRQjpmZ.png' width="50%" />
It generates three (voltage controlled) integers from -12 to +12!

### Floats
<img src='https://i.imgur.com/spQgKmr.png' width="50%" />
It generates three (voltage controlled) floats from -12 to +12!

## Future Plans

None of them actually exist yet, but I'm hoping this will project eventually contain:

  * FFTTuner - FFT / Tuner
  * VCMono - Combine two signals into one, modulated by VC
  * VCSplitter - Split one signal into two, modulated by VC
  * BPM - Dial in a pulse to a specific beats per minute. Also VC-able and "reset"-able.
  * Sidechain - Lower the volume of A based on B.
  * Autopanner - Given a signal, oscillate into two output channels
  * VCDryWet
  * Button - It's literally just a fucking button. You hit it, it sends a reset signal.
  * DubEcho - Two delays and a spring.

## Building

First, clone and [make Rack for yourself](https://github.com/VCVRack/Rack#building).

Then, clone this repo into the `plugins` directory and run `make` from this directory.

## License

(c) Rich Jones 2017, BSD.