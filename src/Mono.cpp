#include <iostream>
#include "RJModules.hpp"

struct Mono : Module {
    enum ParamIds {
        MONO_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        CH1_INPUT,
        CH2_INPUT,
        MONO_CV_INPUT,
        NUM_INPUTS
    };
    enum OutputIds {
        CH1_OUTPUT,
        CH2_OUTPUT,
        NUM_OUTPUTS
    };
    enum LightIds {
        NUM_LIGHTS
    };

    Mono() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS); configParam(Mono::MONO_PARAM, 0.0, 1.0, 0.1, "");}
    void step() override;
};

void Mono::step() {

    float mono_amount = params[MONO_PARAM].value * clamp(inputs[MONO_CV_INPUT].normalize(10.0f) / 10.0f, 0.0f, 1.0f);
    float mono_value = (inputs[CH1_INPUT].value + inputs[CH2_INPUT].value) / 2;

    outputs[CH1_OUTPUT].value = (mono_value * mono_amount) + (((1 - mono_amount)) * inputs[CH1_INPUT].value);
    outputs[CH2_OUTPUT].value = (mono_value * mono_amount) + (((1 - mono_amount)) * inputs[CH2_INPUT].value);

}

struct MonoWidget: ModuleWidget {
    MonoWidget(Mono *module);
};

MonoWidget::MonoWidget(Mono *module) {
		setModule(module);
    box.size = Vec(15*10, 380);

    {
        SVGPanel *panel = new SVGPanel();
        panel->box.size = box.size;
        panel->setBackground(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Mono.svg")));
        addChild(panel);
    }

    addChild(createWidget<ScrewSilver>(Vec(15, 0)));
    addChild(createWidget<ScrewSilver>(Vec(box.size.x-30, 0)));
    addChild(createWidget<ScrewSilver>(Vec(15, 365)));
    addChild(createWidget<ScrewSilver>(Vec(box.size.x-30, 365)));

    addInput(createInput<PJ301MPort>(Vec(22, 85), module, Mono::CH1_INPUT));
    addInput(createInput<PJ301MPort>(Vec(104, 85), module, Mono::CH2_INPUT));

    addParam(createParam<RoundHugeBlackKnob>(Vec(47, 143), module, Mono::MONO_PARAM));

    addInput(createInput<PJ301MPort>(Vec(22, 190), module, Mono::MONO_CV_INPUT));

    addOutput(createOutput<PJ301MPort>(Vec(22, 255), module, Mono::CH1_OUTPUT));
    addOutput(createOutput<PJ301MPort>(Vec(104, 255), module, Mono::CH2_OUTPUT));
}
Model *modelMono = createModel<Mono, MonoWidget>("Mono");
