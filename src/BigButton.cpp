#include "RJModules.hpp"
#include "dsp/digital.hpp"
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

    BigButton() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {}
    void step() override;
};

struct BigLEDButton : SVGSwitch, MomentarySwitch {
        BigLEDButton() {
                addFrame(SVG::load(assetPlugin(plugin, "res/BigLEDButton.svg")));
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
    SchmittTrigger resetTrigger;

    // Reset
    if (params[RESET_PARAM].value > 0) {
        resetLight = 1.0;
        output = 12.0;
    }

    resetLight -= resetLight / lightLambda / engineGetSampleRate();

    outputs[CH1_OUTPUT].value = output;
    outputs[CH2_OUTPUT].value = output;
    outputs[CH3_OUTPUT].value = output;
    outputs[CH4_OUTPUT].value = output;
    outputs[CH5_OUTPUT].value = output;
    outputs[CH6_OUTPUT].value = output;
    lights[RESET_LIGHT].value = resetLight;

}


ButtonWidget::ButtonWidget() {
    BigButton *module = new BigButton();
    setModule(module);
    box.size = Vec(15*10, 380);

    {
        SVGPanel *panel = new SVGPanel();
        panel->box.size = box.size;
        panel->setBackground(SVG::load(assetPlugin(plugin, "res/Button.svg")));
        addChild(panel);
    }

    addChild(createScrew<ScrewSilver>(Vec(15, 0)));
    addChild(createScrew<ScrewSilver>(Vec(box.size.x-30, 0)));
    addChild(createScrew<ScrewSilver>(Vec(15, 365)));
    addChild(createScrew<ScrewSilver>(Vec(box.size.x-30, 365)));

    addOutput(createOutput<PJ301MPort>(Vec(24, 223), module, BigButton::CH1_OUTPUT));
    addOutput(createOutput<PJ301MPort>(Vec(65, 223), module, BigButton::CH2_OUTPUT));
    addOutput(createOutput<PJ301MPort>(Vec(105, 223), module, BigButton::CH3_OUTPUT));

    addOutput(createOutput<PJ301MPort>(Vec(24, 274), module, BigButton::CH4_OUTPUT));
    addOutput(createOutput<PJ301MPort>(Vec(65, 274), module, BigButton::CH5_OUTPUT));
    addOutput(createOutput<PJ301MPort>(Vec(106, 274), module, BigButton::CH6_OUTPUT));

    addParam(createParam<BigLEDButton>(Vec(15, 60), module, BigButton::RESET_PARAM, 0.0, 1.0, 0.0));
    addChild(createLight<GiantLight<GreenLight>>(Vec(25, 70), module, BigButton::RESET_LIGHT));

}
