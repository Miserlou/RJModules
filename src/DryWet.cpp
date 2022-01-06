#include "RJModules.hpp"

#include <iostream>
#include <cmath>
#include <sstream>
#include <iomanip>
#include <vector>

/* UI */
struct DryWetBigSwitchLEDButton : SVGSwitch {
        DryWetBigSwitchLEDButton() {
                addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/SwitchLEDButton.svg")));
                momentary = true;
        }
};

template <typename BASE>
struct DryWetBigOlLight : BASE {
        DryWetBigOlLight() {
                this->box.size = mm2px(Vec(16.0, 16.0));
        }
};

/* The Thing */

struct DryWet : Module {
    enum ParamIds {

        BIG_PARAM,
        BIG_PARAM_2,

        NUM_PARAMS
    };
    enum InputIds {
        BIG_CV_INPUT,
        IN_INPUT,
        IN2_INPUT,

        BIG_CV_INPUT_2,
        IN_INPUT_2,
        IN2_INPUT_2,

        NUM_INPUTS
    };
    enum OutputIds {
        OUT_OUTPUT,
        OUT_OUTPUT_2,
        NUM_OUTPUTS
    };
    enum LightIds {
        IN_LIGHT,
        IN2_LIGHT,

        IN_LIGHT_2,
        IN2_LIGHT_2,
        NUM_LIGHTS
    };

    // Knob One
    dsp::SchmittTrigger resetTrigger;
    dsp::SchmittTrigger resetTrigger2;
    dsp::SchmittTrigger recTriggerCV;
    dsp::SchmittTrigger replayTrigger;
    dsp::SchmittTrigger replayTriggerCV;
    bool on = false;

    std::vector<float> replayVector;
    float param;
    int tapeHead = 0;

    bool isRecording = false;
    bool hasRecorded = false;
    float replayLight = 0.0;
    float last_press = 999999;
    float last_press2 = 999999;

    // Knob Two
    dsp::SchmittTrigger switchOne;

    std::vector<float> replayVector_2;
    float param_2;
    int tapeHead_2 = 0;

    bool switchOneActive = true;
    bool switchTwoActive = true;
    float replayLight_2 = 0.0;

    const float lightLambda = 0.075;

    DryWet() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(DryWet::BIG_PARAM, 0.0, 1.0, 0.5, "");
        configParam(DryWet::BIG_PARAM_2, 0.0, 1.0, 0.5, "");

    }

    void step() override;
};

struct LilLEDButton : SVGSwitch {
        LilLEDButton() {
                addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/LilLEDButton.svg")));
                momentary = true;
        }
};


void DryWet::step() {

    /*
    *
    * Button One
    *
    */

    float mix_amount = params[BIG_PARAM].value * clamp(inputs[BIG_CV_INPUT].normalize(10.0f) / 10.0f, 0.0f, 1.0f);
    outputs[OUT_OUTPUT].value = (inputs[IN2_INPUT].value * mix_amount) + (((1 - mix_amount)) * inputs[IN_INPUT].value);
    lights[IN2_LIGHT].value = mix_amount;
    lights[IN_LIGHT].value = 1 - mix_amount;

    /*
    *
    * Button Two
    *
    */

    float mix_amount2 = params[BIG_PARAM_2].value * clamp(inputs[BIG_CV_INPUT_2].normalize(10.0f) / 10.0f, 0.0f, 1.0f);
    outputs[OUT_OUTPUT_2].value = (inputs[IN2_INPUT_2].value * mix_amount2) + (((1 - mix_amount2)) * inputs[IN_INPUT_2].value);
    lights[IN2_LIGHT_2].value = mix_amount2;
    lights[IN_LIGHT_2].value = 1 - mix_amount2;
}


struct DryWetWidget: ModuleWidget {
    DryWetWidget(DryWet *module);
};

DryWetWidget::DryWetWidget(DryWet *module) {
        setModule(module);

    float buttonx = 20;
    float buttony = 114;
    float offset = 160;

    box.size = Vec(15*10, 380);

    {
        SVGPanel *panel = new SVGPanel();
        panel->box.size = box.size;
        panel->setBackground(APP->window->loadSvg(asset::plugin(pluginInstance, "res/DryWet.svg")));
        addChild(panel);
    }

    addChild(createWidget<ScrewSilver>(Vec(15, 0)));
    addChild(createWidget<ScrewSilver>(Vec(box.size.x-30, 0)));
    addChild(createWidget<ScrewSilver>(Vec(15, 365)));
    addChild(createWidget<ScrewSilver>(Vec(box.size.x-30, 365)));

    // Knob One
    addParam(createParam<RoundHugeBlackKnob>(Vec(47, 61), module, DryWet::BIG_PARAM));
    // addParam(createParam<DryWetBigSwitchLEDButton>(Vec(47, 61), module, DryWet::BIG_PARAM));
    // addChild(createLight<DryWetBigOlLight<GreenLight>>(Vec(53, 67), module, DryWet::RESET_LIGHT));
    addInput(createInput<PJ301MPort>(Vec(17, 50), module, DryWet::BIG_CV_INPUT));
    addInput(createInput<PJ301MPort>(Vec(18, 142), module, DryWet::IN_INPUT));
    addInput(createInput<PJ301MPort>(Vec(64, 142), module, DryWet::IN2_INPUT));
    addOutput(createOutput<PJ301MPort>(Vec(110, 142), module, DryWet::OUT_OUTPUT));
    addChild(createLight<MediumLight<GreenLight>>(Vec(26, 178), module, DryWet::IN_LIGHT));
    addChild(createLight<MediumLight<GreenLight>>(Vec(72, 178), module, DryWet::IN2_LIGHT));

    // Knob Two
    addParam(createParam<RoundHugeBlackKnob>(Vec(47, 61 + offset), module, DryWet::BIG_PARAM_2));
    // addParam(createParam<DryWetBigSwitchLEDButton>(Vec(47, 61 + offset), module, DryWet::BIG_PARAM_2));
    // addChild(createLight<DryWetBigOlLight<GreenLight>>(Vec(53, 67 + offset), module, DryWet::RESET_LIGHT_2));
    addInput(createInput<PJ301MPort>(Vec(17, 50 + offset), module, DryWet::BIG_CV_INPUT_2));
    addInput(createInput<PJ301MPort>(Vec(18, 142 + offset), module, DryWet::IN_INPUT_2));
    addInput(createInput<PJ301MPort>(Vec(64, 142 + offset), module, DryWet::IN2_INPUT_2));
    addOutput(createOutput<PJ301MPort>(Vec(110, 142 + offset), module, DryWet::OUT_OUTPUT_2));
    addChild(createLight<MediumLight<GreenLight>>(Vec(26, 178 + offset), module, DryWet::IN_LIGHT_2));
    addChild(createLight<MediumLight<GreenLight>>(Vec(72, 178 + offset), module, DryWet::IN2_LIGHT_2));

}
Model *modelDryWet = createModel<DryWet, DryWetWidget>("DryWet");
