#include "RJModules.hpp"
#include <iostream>
#include <cmath>

struct Panner: Module {
    enum ParamIds {
        CH1_PARAM,
        CH2_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        CH1_INPUT,
        CH1_CV_INPUT,
        NUM_INPUTS
    };
    enum OutputIds {
        CH1_OUTPUT,
        CH2_OUTPUT,
        NUM_OUTPUTS
    };

    Panner() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS) {}
    void step() override;
};


void Panner::step() {
    float ch1 = inputs[CH1_INPUT].value;

    float combined_input = params[CH1_PARAM].value * clampf(inputs[CH1_CV_INPUT].normalize(10.0) / 10.0, 0.0, 1.0);

    float left_percent = combined_input;
    float right_percent = 1 - combined_input;

    outputs[CH2_OUTPUT].value = ch1 * left_percent;
    outputs[CH1_OUTPUT].value = ch1 * right_percent;
}


PannerWidget::PannerWidget() {
    Panner *module = new Panner();
    setModule(module);
    box.size = Vec(15*10, 380);

    {
        SVGPanel *panel = new SVGPanel();
        panel->box.size = box.size;
        panel->setBackground(SVG::load(assetPlugin(plugin, "res/Panner.svg")));
        addChild(panel);
    }

    addChild(createScrew<ScrewSilver>(Vec(15, 0)));
    addChild(createScrew<ScrewSilver>(Vec(box.size.x-30, 0)));
    addChild(createScrew<ScrewSilver>(Vec(15, 365)));
    addChild(createScrew<ScrewSilver>(Vec(box.size.x-30, 365)));

    addParam(createParam<RoundBlackKnob>(Vec(57, 139), module, Panner::CH1_PARAM, 0.0, 1.0, 0.0));

    addInput(createInput<PJ301MPort>(Vec(22, 129), module, Panner::CH1_INPUT));
    addInput(createInput<PJ301MPort>(Vec(22, 160), module, Panner::CH1_CV_INPUT));

    addOutput(createOutput<PJ301MPort>(Vec(110, 125), module, Panner::CH1_OUTPUT));
    addOutput(createOutput<PJ301MPort>(Vec(110, 175), module, Panner::CH2_OUTPUT));
}
