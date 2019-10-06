#include "RJModules.hpp"

#include <iostream>
#include <cmath>
#include <sstream>
#include <iomanip>
#include <vector>

/* UI */
struct TriggerSwitchBigSwitchLEDButton : SVGSwitch {
        TriggerSwitchBigSwitchLEDButton() {
                addFrame(SVG::load(assetPlugin(pluginInstance, "res/SwitchLEDButton.svg")));
                momentary = true;
        }
};

template <typename BASE>
struct TriggerSwitchBigOlLight : BASE {
        TriggerSwitchBigOlLight() {
                this->box.size = mm2px(Vec(16.0, 16.0));
        }
};

/* The Thing */

struct TriggerSwitch : Module {
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
        RESET_LIGHT,
        IN_LIGHT,
        IN2_LIGHT,

        RESET_LIGHT_2,
        IN_LIGHT_2,
        IN2_LIGHT_2,
        NUM_LIGHTS
    };

    // Knob One
    SchmittTrigger resetTrigger;
    SchmittTrigger resetTrigger2;
    SchmittTrigger recTriggerCV;
    SchmittTrigger replayTrigger;
    SchmittTrigger replayTriggerCV;
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
    SchmittTrigger switchOne;

    std::vector<float> replayVector_2;
    float param_2;
    int tapeHead_2 = 0;

    bool switchOneActive = true;
    bool switchTwoActive = false;
    float replayLight_2 = 0.0;

    const float lightLambda = 0.075;

    TriggerSwitch() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(TriggerSwitch::BIG_PARAM, -5.0, 5.0, 0.0, "");
        configParam(TriggerSwitch::BIG_PARAM_2, 0.0, 10.0, 5.0, "");

    }

    void step() override;
};

struct LilLEDButton : SVGSwitch {
        LilLEDButton() {
                addFrame(SVG::load(assetPlugin(pluginInstance, "res/LilLEDButton.svg")));
                momentary = true;
        }
};


void TriggerSwitch::step() {

    /*
    *
    * Button One
    *
    */

    float on_off = params[BIG_PARAM].value;
    // issa hack
    if (last_press > 10000 || inputs[BIG_CV_INPUT].value != 0){
        if(resetTrigger.process(inputs[BIG_CV_INPUT].value)){
          switchOneActive = !switchOneActive;
          last_press = 0;
        }
        else{
            lights[RESET_LIGHT].value = 0.0;
        }
        if(resetTrigger.process(on_off)){
          switchOneActive = !switchOneActive;
          last_press = 0;
        } else{
            lights[RESET_LIGHT].value = 0.0;
        }
    }

    if(switchOneActive){
        outputs[OUT_OUTPUT].value = inputs[IN_INPUT].value;
        lights[RESET_LIGHT].value = 0.0;
        lights[IN_LIGHT].value = 1.0;
        lights[IN2_LIGHT].value = 0.0;
    } else {
        outputs[OUT_OUTPUT].value = inputs[IN2_INPUT].value;
        lights[RESET_LIGHT].value = 1.0;
        lights[IN_LIGHT].value = 0.0;
        lights[IN2_LIGHT].value = 1.0;
    }
    last_press++;

    /*
    *
    * Button One
    *
    */

    float on_off2 = params[BIG_PARAM_2].value;
    // issa hack
    if (last_press2 > 10000 || inputs[BIG_CV_INPUT_2].value != 0){
        if(resetTrigger2.process(inputs[BIG_CV_INPUT_2].value)){
          switchTwoActive = !switchTwoActive;
          last_press2 = 0;
        }
        else{
            lights[RESET_LIGHT_2].value = 0.0;
        }
        if(resetTrigger2.process(on_off2)){
          switchTwoActive = !switchTwoActive;
          last_press2 = 0;
        } else{
            lights[RESET_LIGHT_2].value = 0.0;
        }
    }

    if(switchTwoActive){
        outputs[OUT_OUTPUT_2].value = inputs[IN_INPUT_2].value;
        lights[RESET_LIGHT_2].value = 0.0;
        lights[IN_LIGHT_2].value = 1.0;
        lights[IN2_LIGHT_2].value = 0.0;
    } else {
        outputs[OUT_OUTPUT_2].value = inputs[IN2_INPUT_2].value;
        lights[RESET_LIGHT_2].value = 1.0;
        lights[IN_LIGHT_2].value = 0.0;
        lights[IN2_LIGHT_2].value = 1.0;
    }
    last_press2++;

}


struct TriggerSwitchWidget: ModuleWidget {
    TriggerSwitchWidget(TriggerSwitch *module);
};

TriggerSwitchWidget::TriggerSwitchWidget(TriggerSwitch *module) {
        setModule(module);

    float buttonx = 20;
    float buttony = 114;
    float offset = 160;

    box.size = Vec(15*10, 380);

    {
        SVGPanel *panel = new SVGPanel();
        panel->box.size = box.size;
        panel->setBackground(SVG::load(assetPlugin(pluginInstance, "res/TriggerSwitch.svg")));
        addChild(panel);
    }

    addChild(createWidget<ScrewSilver>(Vec(15, 0)));
    addChild(createWidget<ScrewSilver>(Vec(box.size.x-30, 0)));
    addChild(createWidget<ScrewSilver>(Vec(15, 365)));
    addChild(createWidget<ScrewSilver>(Vec(box.size.x-30, 365)));

    // Knob One
    //addParam(createParam<RoundHugeBlackKnob>(Vec(47, 61), module, TriggerSwitch::BIG_PARAM));
    addParam(createParam<TriggerSwitchBigSwitchLEDButton>(Vec(47, 61), module, TriggerSwitch::BIG_PARAM));
    addChild(createLight<TriggerSwitchBigOlLight<GreenLight>>(Vec(53, 67), module, TriggerSwitch::RESET_LIGHT));
    addInput(createPort<PJ301MPort>(Vec(17, 50), PortWidget::INPUT, module, TriggerSwitch::BIG_CV_INPUT));
    addInput(createPort<PJ301MPort>(Vec(18, 142), PortWidget::INPUT, module, TriggerSwitch::IN_INPUT));
    addInput(createPort<PJ301MPort>(Vec(64, 142), PortWidget::INPUT, module, TriggerSwitch::IN2_INPUT));
    addOutput(createPort<PJ301MPort>(Vec(110, 142), PortWidget::OUTPUT, module, TriggerSwitch::OUT_OUTPUT));
    addChild(createLight<MediumLight<GreenLight>>(Vec(26, 178), module, TriggerSwitch::IN_LIGHT));
    addChild(createLight<MediumLight<GreenLight>>(Vec(72, 178), module, TriggerSwitch::IN2_LIGHT));

    // Knob Two
    addParam(createParam<TriggerSwitchBigSwitchLEDButton>(Vec(47, 61 + offset), module, TriggerSwitch::BIG_PARAM_2));
    addChild(createLight<TriggerSwitchBigOlLight<GreenLight>>(Vec(53, 67 + offset), module, TriggerSwitch::RESET_LIGHT_2));
    addInput(createPort<PJ301MPort>(Vec(17, 50 + offset), PortWidget::INPUT, module, TriggerSwitch::BIG_CV_INPUT_2));
    addInput(createPort<PJ301MPort>(Vec(18, 142 + offset), PortWidget::INPUT, module, TriggerSwitch::IN_INPUT_2));
    addInput(createPort<PJ301MPort>(Vec(64, 142 + offset), PortWidget::INPUT, module, TriggerSwitch::IN2_INPUT_2));
    addOutput(createPort<PJ301MPort>(Vec(110, 142 + offset), PortWidget::OUTPUT, module, TriggerSwitch::OUT_OUTPUT_2));
    addChild(createLight<MediumLight<GreenLight>>(Vec(26, 178 + offset), module, TriggerSwitch::IN_LIGHT_2));
    addChild(createLight<MediumLight<GreenLight>>(Vec(72, 178 + offset), module, TriggerSwitch::IN2_LIGHT_2));

}
Model *modelTriggerSwitch = createModel<TriggerSwitch, TriggerSwitchWidget>("TriggerSwitch");
