#include <iostream>
#include <stdlib.h>
#include <random>
#include <cmath>

#include "dsp/digital.hpp"
#include "RJModules.hpp"

/*
    Voss pink noise algorithm from: http://www.firstpr.com.au/dsp/pink-noise/#Voss
*/

class PinkNumber
{

// goes from 0 to 118;
private:
  int max_key;
  int key;
  unsigned int white_values[5];
  unsigned int range;
public:
  PinkNumber(unsigned int range = 128)
    {
      max_key = 0x1f; // Five bits set
      this->range = range;
      key = 0;
      for (int i = 0; i < 5; i++){
        white_values[i] = rand() % (range/5);
      }
    }
  float GetNextValue()
    {
      int last_key = key;
      unsigned int sum;

      key++;
      if (key > max_key){
        key = 0;
      }
      // Exclusive-Or previous value with current value. This gives
      // a list of bits that have changed.
      int diff = last_key ^ key;
      sum = 0;
      for (int i = 0; i < 5; i++)
 {
   // If bit changed get new random number for corresponding
   // white_value
   if (diff & (1 << i))
     white_values[i] = rand() % (range/5);
   sum += white_values[i];
 }
      return 1.0 * (float)sum;
    }
};

struct Noise : Module {
    enum ParamIds {
        COLOR_PARAM,
        LPF_PARAM,
        HPF_PARAM,
        VOL_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        COLOR_CV_INPUT,
        LPF_CV_INPUT,
        HPF_CV_INPUT,
        VOL_CV_INPUT,
        NUM_INPUTS
    };
    enum OutputIds {
        NOISE_OUTPUT,
        NUM_OUTPUTS
    };
    enum LightIds {
        NUM_LIGHTS
    };

    PinkNumber pink = PinkNumber();

    float low = 99;
    float high = 0;
    float next;
    float mapped_pink = 0.0;
    float white = 0.0;
    float mixed = 0.0;
    float mix_value = 1.0;
    std::random_device rd; // obtain a random number from hardware

    Noise() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {}
    void step() override;
};

void Noise::step(){

    //std::cout << pink.GetNextValue() << '\n';
    // new_value = ((old_value - old_min) / (old_max - old_min)) * (new_max - new_min) + new_min

    // ( (old_value - old_min) / (old_max - old_min) ) * (new_max - new_min) + new_min
    next = pink.GetNextValue();

    // std::cout << next << '\n';

    // works ish
    //mapped_pink = (next - 1) / 118 * 12;

    // ( (old_value - old_min) / (old_max - old_min) ) * (new_max - new_min) + new_min

    // -12/12 mapper
    //mapped_pink = (next - 0) / (118 ) * 24 - 12;

    // -5/5 mapper
    // ( (old_value - old_min) / (old_max - old_min) ) * (new_max - new_min) + new_min
    mapped_pink = (next - 0) / (118 ) * 10 - 5;

    std::mt19937 eng(rd()); // seed the generator
    std::uniform_real_distribution<> distr1(-5, 5); // define the range
    white =  distr1(eng);

    // float mapped_pink = 0 + ((118 - 0) / (24)) * (next - 0);
    // std::cout << mapped_pink << '\n';

    // std::cout << '\n';
    // if (mapped_pink<low){
    //     low = mapped_pink;
    // }
    // if (mapped_pink>high){
    //     high = mapped_pink;
    // }
    // std::cout << "high: " << high << "\n";
    // std::cout << "low: " << low << "\n";
    // std::cout << "mapped: " << mapped_pink << "\n";

    mix_value = params[COLOR_PARAM].value * clampf(inputs[COLOR_CV_INPUT].normalize(10.0) / 10.0, 0.0, 1.0);
    mixed = ( (mapped_pink * mix_value) + (white * (1.0 - mix_value)) )/ 2;

    // if you don't map to whatever, it just sounds like weird kinda cool crackles
    //outputs[NOISE_OUTPUT].value = pink.GetNextValue();
    outputs[NOISE_OUTPUT].value = mixed;

}

NoiseWidget::NoiseWidget() {
    Noise *module = new Noise();
    setModule(module);
    box.size = Vec(15*10, 380);

    {
        SVGPanel *panel = new SVGPanel();
        panel->box.size = box.size;
        panel->setBackground(SVG::load(assetPlugin(plugin, "res/Noise.svg")));
        addChild(panel);
    }

    addChild(createScrew<ScrewSilver>(Vec(15, 0)));
    addChild(createScrew<ScrewSilver>(Vec(box.size.x-30, 0)));
    addChild(createScrew<ScrewSilver>(Vec(15, 365)));
    addChild(createScrew<ScrewSilver>(Vec(box.size.x-30, 365)));

    addParam(createParam<RoundHugeBlackKnob>(Vec(47, 61), module, Noise::COLOR_PARAM, 0.0, 1.0, 1.0));
    addParam(createParam<RoundHugeBlackKnob>(Vec(47, 143), module, Noise::LPF_PARAM, 0.0, 1.0, 0.1));
    addParam(createParam<RoundHugeBlackKnob>(Vec(47, 228), module, Noise::HPF_PARAM, 0.0, 1.0, 1.0));

    addInput(createInput<PJ301MPort>(Vec(22, 100), module, Noise::COLOR_CV_INPUT));
    addInput(createInput<PJ301MPort>(Vec(22, 190), module, Noise::LPF_CV_INPUT));
    addInput(createInput<PJ301MPort>(Vec(22, 270), module, Noise::HPF_CV_INPUT));
    addInput(createInput<PJ301MPort>(Vec(22, 310), module, Noise::VOL_CV_INPUT));

    addOutput(createOutput<PJ301MPort>(Vec(100, 310), module, Noise::NOISE_OUTPUT));
}