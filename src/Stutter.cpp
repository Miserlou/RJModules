#include <iostream>
#include <stdlib.h>
#include <random>
#include <cmath>

#include "RJModules.hpp"
#include "VAStateVariableFilter.h"

#define HISTORY_SIZE (1<<21)

struct Stutter : Module {
    enum ParamIds {
        TIME_PARAM,
        MIX_PARAM,
        ONOFF_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        CH1_INPUT,
        TIME_CV_INPUT,
        MIX_CV_INPUT,
        ONOFF_INPUT,
        NUM_INPUTS
    };
    enum OutputIds {
        CH1_OUTPUT,
        CH2_OUTPUT,
        NUM_OUTPUTS
    };
    enum LightIds {
        RESET_LIGHT,
        NUM_LIGHTS
    };

    DoubleRingBuffer<float, HISTORY_SIZE> historyBuffer;
    DoubleRingBuffer<float, 16> outBuffer;
    SampleRateConverter<1> src;

    bool on = false;
    float last_press = 999999;

    SchmittTrigger resetTrigger;

    float bufferedSamples[36000] = {0.0};
    int tapeHead = 0;

    Stutter() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);}
    void step() override;
};

struct BigSwitchLEDButton : SVGSwitch {
        BigSwitchLEDButton() {
                addFrame(SVG::load(assetPlugin(pluginInstance, "res/SwitchLEDButton.svg")));
                momentary = true;
        }
};

template <typename BASE>
struct BigOlLight : BASE {
        BigOlLight() {
                this->box.size = mm2px(Vec(16.0, 16.0));
        }
};

void Stutter::step(){

  float in = inputs[CH1_INPUT].value;
  float on_off = params[ONOFF_PARAM].value;
  int time = params[TIME_PARAM].value * clamp(inputs[TIME_CV_INPUT].normalize(10.0f) / 10.0f, 0.0f, 1.0f); ;
  float wet = in;

  // issa hack
  if (last_press > 6000){
    if(resetTrigger.process(inputs[ONOFF_INPUT].value)){
      // tapeHead=0;
      on = !on;
      last_press = 0;
    }
    if(resetTrigger.process(on_off)){
      // tapeHead=0;
      on = !on;
      last_press = 0;
    }
  }

  // issa hack
  if (time == 0){
    time = 143;
  }

  if(!on){
      bufferedSamples[tapeHead] = in;
  }

  if(on){
    wet = bufferedSamples[tapeHead];
    if(tapeHead >= time){
      tapeHead = -1;
    }
  } else{
    if(tapeHead >= (36000-1)){
      tapeHead = -1;
    }
  }

  tapeHead++;
  last_press++;

  //mix
  float mix_percent = params[MIX_PARAM].value * clamp(inputs[MIX_CV_INPUT].normalize(10.0f) / 10.0f, 0.0f, 1.0f);
  float mixed = ((wet * mix_percent)) + (in * (1-mix_percent));

  outputs[CH1_OUTPUT].value = mixed;
  if(on){
    lights[RESET_LIGHT].value = 1.0;
  } else{
    lights[RESET_LIGHT].value = 0.0;
  }

}

struct StutterWidget: ModuleWidget {
    StutterWidget(Stutter *module);
};

StutterWidget::StutterWidget(Stutter *module) {
		setModule(module);
    box.size = Vec(15*10, 380);

    {
        SVGPanel *panel = new SVGPanel();
        panel->box.size = box.size;
        panel->setBackground(SVG::load(assetPlugin(pluginInstance, "res/Stutter.svg")));
        addChild(panel);
    }

    addChild(createWidget<ScrewSilver>(Vec(15, 0)));
    addChild(createWidget<ScrewSilver>(Vec(box.size.x-30, 0)));
    addChild(createWidget<ScrewSilver>(Vec(15, 365)));
    addChild(createWidget<ScrewSilver>(Vec(box.size.x-30, 365)));

    //addParam(createParam<RoundHugeBlackKnob>(Vec(47, 61), module, Stutter::ONOFF_PARAM, 0.0, 1.0, 1.0));
    addParam(createParam<BigSwitchLEDButton>(Vec(47, 61), module, Stutter::ONOFF_PARAM, 0.0, 1.0, 0.0));
    addChild(createLight<BigOlLight<GreenLight>>(Vec(53, 67), module, Stutter::RESET_LIGHT));

    addParam(createParam<RoundHugeBlackKnob>(Vec(47, 143), module, Stutter::TIME_PARAM, 0, 36000, 4000));
    addParam(createParam<RoundHugeBlackKnob>(Vec(47, 228), module, Stutter::MIX_PARAM, 0.0, 1.0, 1.0));

    addInput(createPort<PJ301MPort>(Vec(22, 100), PortWidget::INPUT, module, Stutter::ONOFF_INPUT));
    addInput(createPort<PJ301MPort>(Vec(22, 190), PortWidget::INPUT, module, Stutter::TIME_CV_INPUT));
    addInput(createPort<PJ301MPort>(Vec(22, 270), PortWidget::INPUT, module, Stutter::MIX_CV_INPUT));

    addInput(createPort<PJ301MPort>(Vec(22, 315), PortWidget::INPUT, module, Stutter::CH1_INPUT));
    addOutput(createPort<PJ301MPort>(Vec(100, 315), PortWidget::OUTPUT, module, Stutter::CH1_OUTPUT));
}
Model *modelStutter = createModel<Stutter, StutterWidget>("Stutter");
