# RJModules

Various DIY modules for [VCV Rack](https://github.com/VCVRack/Rack).

None of them actually exist yet, but I'm hoping this will eventually contain:

  * FFTTuner - FFT / Tuner
  * VCMono - Combine two signals into one, modulated by VC
  * VCSplitter - Split one signal into two, modulated by VC
  * VCBitCrush - Bit Crusher
  * BPM - Dial in a pulse to a specific beats per minute. Also VC-able and "reset"-able.
  * Sidechain - Lower the volume of A based on B.
  * Autopanner - Given a signal, oscillate into two output channels
  * VCDryWet
  * Button - It's literally just a fucking button. You hit it, it sends a reset signal.
  * Integers - Just some integers!
  * Floats - Just some floats!
  * DubEcho - Two delays and a spring.

## Building

First, clone and [make Rack for yourself](https://github.com/VCVRack/Rack#building).

Then, clone this repo into the `plugins` directory and run `make` from this directory.
