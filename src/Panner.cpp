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

    Panner() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS); configParam(Panner::CH1_PARAM, 0.0, 1.0, 0.0, "");}
    void step() override;
};


void Panner::step() {
    float ch1 = inputs[CH1_INPUT].value;

    float combined_input = params[CH1_PARAM].value * clamp(inputs[CH1_CV_INPUT].normalize(10.0f) / 10.0f, 0.0f, 1.0f);

    float left_percent = combined_input;
    float right_percent = 1 - combined_input;

    outputs[CH2_OUTPUT].value = ch1 * left_percent;
    outputs[CH1_OUTPUT].value = ch1 * right_percent;
}


struct PannerWidget: ModuleWidget {
    PannerWidget(Panner *module);
};

PannerWidget::PannerWidget(Panner *module) {
		setModule(module);
    box.size = Vec(15*10, 380);

    {
        SVGPanel *panel = new SVGPanel();
        panel->box.size = box.size;
        panel->setBackground(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Panner.svg")));
        addChild(panel);
    }

    addChild(createWidget<ScrewSilver>(Vec(15, 0)));
    addChild(createWidget<ScrewSilver>(Vec(box.size.x-30, 0)));
    addChild(createWidget<ScrewSilver>(Vec(15, 365)));
    addChild(createWidget<ScrewSilver>(Vec(box.size.x-30, 365)));

    addParam(createParam<RoundBlackKnob>(Vec(57, 139), module, Panner::CH1_PARAM));

    addInput(createInput<PJ301MPort>(Vec(22, 129), module, Panner::CH1_INPUT));
    addInput(createInput<PJ301MPort>(Vec(22, 160), module, Panner::CH1_CV_INPUT));

    addOutput(createOutput<PJ301MPort>(Vec(110, 125), module, Panner::CH1_OUTPUT));
    addOutput(createOutput<PJ301MPort>(Vec(110, 175), module, Panner::CH2_OUTPUT));
}

Model *modelPanner = createModel<Panner, PannerWidget>("Panner");
