/*

*/

#include "RJModules.hpp"

#include "common.hpp"
#include <iostream>
#include <cmath>
#include <sstream>
#include <iomanip>

struct BrickwallRoundSmallBlackKnob : RoundSmallBlackKnob
{
    BrickwallRoundSmallBlackKnob()
    {
        setSVG(SVG::load(assetPlugin(pluginInstance, "res/KTFRoundSmallBlackKnob.svg")));
    }
};

struct Brickwall : Module {
    enum ParamIds {
        LEVEL_PARAM,

        ATTACK_PARAM,
        DECAY_PARAM,
        SUSTAIN_PARAM,
        RELEASE_PARAM,
        EXP_PARAM,

        DEPTH_PARAM,
        PREAMP_PARAM,
        POSTAMP_PARAM,

        NUM_PARAMS
    };
    enum InputIds {
        CV_INPUT,
        IN_INPUT,

        ATTACK_INPUT,
        DECAY_INPUT,
        SUSTAIN_INPUT,
        RELEASE_INPUT,
        GATE_INPUT,
        TRIG_INPUT,
        EXP_INPUT,
        DEPTH_INPUT,

        NUM_INPUTS
    };
    enum OutputIds {
        OUT_OUTPUT,
        NUM_OUTPUTS
    };
    enum LightIds {
        // ATTACK_LIGHT,
        // DECAY_LIGHT,
        // SUSTAIN_LIGHT,
        // RELEASE_LIGHT,
        ACTIVE_LIGHT,
        NUM_LIGHTS
};

    float lastCv = 0.f;
    bool decaying = false;
    float env = 0.0f;
    SchmittTrigger trigger;

    Brickwall() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(Brickwall::DEPTH_PARAM, 0.0, 12.0, 10.0, "");
        configParam(Brickwall::RELEASE_PARAM, 0.2, 0.4f, 0.50f, "");
        configParam(Brickwall::EXP_PARAM, 0.0001f, .2f, 4.0f, "");
        configParam(Brickwall::PREAMP_PARAM, 1.0f, 2.5f, 1.0f, "");
        configParam(Brickwall::POSTAMP_PARAM, 1.0f, 2.5f, 1.0f, "");
    }

    void step() override {

        float input = inputs[IN_INPUT].value;
        input = input * params[PREAMP_PARAM].value;


        float calculated_depth = params[DEPTH_PARAM].value;
        if(inputs[DEPTH_INPUT].isConnected()){
            calculated_depth = clamp(params[DEPTH_PARAM].value + inputs[DEPTH_INPUT].value / 10.0f, 0.0f, 10.0f);
        }
        float output = inputs[IN_INPUT].value;
        float light_value = 0.0;

        if(input>calculated_depth){
            output = calculated_depth;
            light_value = -1.0;
        }
        if(input<calculated_depth*-1){
            output = calculated_depth*-1;
            light_value = -1.0;
        }

        output = output * params[POSTAMP_PARAM].value;

        // lights[ACTIVE_LIGHT].setBrightness(light_value);
        outputs[OUT_OUTPUT].value = output;

        /* ADSR */
        // float attack = 0.005f;
        // float decay = 10.0f;
        // float sustain = 10.0f;
        // float release = clamp(params[RELEASE_PARAM].value + inputs[RELEASE_INPUT].value / 10.0f, 0.0f, 1.0f);

        // // Gate and trigger
        // bool gated = inputs[GATE_INPUT].value >= 1.0f;
        // if (trigger.process(inputs[TRIG_INPUT].value))
        //     decaying = false;

        // const float base = 20000.0f;
        // const float maxTime = 20.0f;
        // if (gated) {
        //     if (decaying) {
        //         // Decay
        //         if (decay < 1e-4) {
        //             env = sustain;
        //         }
        //         else {
        //             env += powf(base, 1 - decay) / maxTime * (sustain - env) * engineGetSampleTime();
        //         }
        //     }
        //     else {
        //         // Attack
        //         // Skip ahead if attack is all the way down (infinitely fast)
        //         if (attack < 1e-4) {
        //             env = 1.0f;
        //         }
        //         else {
        //             env += powf(base, 1 - attack) / maxTime * (1.01f - env) * engineGetSampleTime();
        //         }
        //         if (env >= 1.0f) {
        //             env = 1.0f;
        //             decaying = true;
        //         }
        //     }
        // }
        // else {
        //     // Release
        //     if (release < 1e-4) {
        //         env = 0.0f;
        //     }
        //     else {
        //         env += powf(base, 1 - release) / maxTime * (0.0f - env) * engineGetSampleTime();
        //     }
        //     decaying = false;
        // }

        // bool sustaining = isNear(env, sustain, 1e-3);
        // bool resting = isNear(env, 0.0f, 1e-3);
        // float env_output = 10.0f * env;

        // /* VCA */
        // float cv = fmaxf(env_output / 10.f, 0.f);
        // float exp_val =  clamp(params[EXP_PARAM].value + inputs[EXP_PARAM].value / 10.0f, 0.0f, 1.0f);
        // cv = powf(cv, exp_val);
        // lastCv = cv;
        // outputs[OUT_OUTPUT].value = inputs[IN_INPUT].value * cv;

    }
};

struct BrickwallWidget : ModuleWidget {
    BrickwallWidget(Brickwall *module) {
		setModule(module);
        setPanel(SVG::load(assetPlugin(pluginInstance, "res/Brickwall.svg")));

        addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

        addParam(createParam<BrickwallRoundSmallBlackKnob>(mm2px(Vec(3.5, 38.9593)), module, Brickwall::DEPTH_PARAM));
        addInput(createPort<PJ301MPort>(mm2px(Vec(3.51398, 48.74977)), PortWidget::INPUT, module, Brickwall::DEPTH_INPUT));

        addParam(createParam<BrickwallRoundSmallBlackKnob>(mm2px(Vec(3.5, 65.9593)), module, Brickwall::PREAMP_PARAM));
        // addInput(createPort<PJ301MPort>(mm2px(Vec(3.51398, 71.74977)), PortWidget::INPUT, module, Brickwall::EXP_INPUT));

        addParam(createParam<BrickwallRoundSmallBlackKnob>(mm2px(Vec(3.5, 81.9593)), module, Brickwall::POSTAMP_PARAM));

        // addChild(createLight<SmallLight<GreenRedLight>>(Vec(3, 84), module, Brickwall::ACTIVE_LIGHT));
        // addInput(createPort<PJ301MPort>(mm2px(Vec(3.51398, 84.74977)), PortWidget::INPUT, module, Brickwall::GATE_INPUT));
        addInput(createPort<PJ301MPort>(mm2px(Vec(3.51398, 97.74977)), PortWidget::INPUT, module, Brickwall::IN_INPUT));
        addOutput(createPort<PJ301MPort>(mm2px(Vec(3.51398, 108.64454)), PortWidget::OUTPUT, module, Brickwall::OUT_OUTPUT));
    }
};


Model *modelBrickwall = createModel<Brickwall, BrickwallWidget>("Brickwall");
