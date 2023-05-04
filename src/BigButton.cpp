#include "RJModules.hpp"
#include <iostream>
#include <cmath>

struct BigButton: Module {
    enum ParamIds {
        RESET_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        NUM_INPUTS
    };
    enum OutputIds {
        CH1_OUTPUT,
        CH2_OUTPUT,
        CH3_OUTPUT,
        CH4_OUTPUT,
        CH5_OUTPUT,
        CH6_OUTPUT,
        NUM_OUTPUTS
    };
    enum LightIds {
        RESET_LIGHT,
        NUM_LIGHTS
    };
    float resetLight = 0.0;

    BigButton() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(BigButton::RESET_PARAM, 0.0, 1.0, 0.0, "");
    }
    void step() override;
};

// struct BigLEDButton : SVGSwitch, MomentarySwitch {
struct BigLEDButton : SVGSwitch{
        BigLEDButton() {
                addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/BigLEDButton.svg")));
                momentary = true;
        }
};

template <typename BASE>
struct GiantLight : BASE {
        GiantLight() {
                this->box.size = mm2px(Vec(34, 34));
        }
};

void BigButton::step() {

    const float lightLambda = 0.075;
    float output = 0.0;
    dsp::SchmittTrigger resetTrigger;

    // Reset
    if (params[RESET_PARAM].value > 0) {
        resetLight = 1.0;
        output = 12.0;
    }

    resetLight -= resetLight / lightLambda / APP->engine->getSampleRate();

    outputs[CH1_OUTPUT].value = output;
    outputs[CH2_OUTPUT].value = output;
    outputs[CH3_OUTPUT].value = output;
    outputs[CH4_OUTPUT].value = output;
    outputs[CH5_OUTPUT].value = output;
    outputs[CH6_OUTPUT].value = output;
    lights[RESET_LIGHT].value = resetLight;

}

struct ButtonWidget: ModuleWidget {
    ButtonWidget(BigButton *module);
};

ButtonWidget::ButtonWidget(BigButton *module) {
		setModule(module);
    box.size = Vec(15*10, 380);

    {
        SVGPanel *panel = new SVGPanel();
        panel->box.size = box.size;
        panel->setBackground(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Button.svg")));
        addChild(panel);
    }

    addChild(createWidget<ScrewSilver>(Vec(15, 0)));
    addChild(createWidget<ScrewSilver>(Vec(box.size.x-30, 0)));
    addChild(createWidget<ScrewSilver>(Vec(15, 365)));
    addChild(createWidget<ScrewSilver>(Vec(box.size.x-30, 365)));

    addOutput(createOutput<PJ301MPort>(Vec(24, 223), module, BigButton::CH1_OUTPUT));
    addOutput(createOutput<PJ301MPort>(Vec(65, 223), module, BigButton::CH2_OUTPUT));
    addOutput(createOutput<PJ301MPort>(Vec(105, 223), module, BigButton::CH3_OUTPUT));

    addOutput(createOutput<PJ301MPort>(Vec(24, 274), module, BigButton::CH4_OUTPUT));
    addOutput(createOutput<PJ301MPort>(Vec(65, 274), module, BigButton::CH5_OUTPUT));
    addOutput(createOutput<PJ301MPort>(Vec(106, 274), module, BigButton::CH6_OUTPUT));

    addParam(createParam<BigLEDButton>(Vec(15, 60), module, BigButton::RESET_PARAM));
    addChild(createLight<GiantLight<GreenLight>>(Vec(25, 70), module, BigButton::RESET_LIGHT));

}

Model *modelButton = createModel<BigButton, ButtonWidget>("Button");
