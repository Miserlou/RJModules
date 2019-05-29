/*
A C I D

aka BADACID

2x Wave + Envelope/VCA + FM Filter + Acid

Elements pinched from Lindenberg + dBiZ

*/

#include "RJModules.hpp"
#include "dsp/digital.hpp"

struct Acid : Module {
    enum ParamIds {

        // Wave
        WAVE_1_PARAM,
        WAVE_2_PARAM,
        WAVE_MIX_PARAM,

        // Env
        ENV_REL_PARAM,
        ENV_SHAPE_PARAM,
        ENV_AMT_PARAM,

        // Filter
        FILTER_CUT_PARAM,
        FILTER_FM_1_PARAM,
        FILTER_FM_2_PARAM,
        FILTER_Q_PARAM,
        FILTER_DRIVE_PARAM,

        // Pluck
        PLUCK_REL_PARAM,
        PLUCK_EXP_PARAM,

        NUM_PARAMS
    };
    enum InputIds {
        VOCT_INPUT,
        GATE_INPUT,

        ATTACK_INPUT,
        DECAY_INPUT,
        SUSTAIN_INPUT,
        RELEASE_INPUT,

        TRIG_INPUT,
        EXP_INPUT,

        NUM_INPUTS
    };
    enum OutputIds {
        OUT_OUTPUT,
        NUM_OUTPUTS
    };
    enum LightIds {
        ATTACK_LIGHT,
        DECAY_LIGHT,
        SUSTAIN_LIGHT,
        RELEASE_LIGHT,
        NUM_LIGHTS
};

    float lastCv = 0.f;
    bool decaying = false;
    float env = 0.0f;
    SchmittTrigger trigger;

    Acid() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {}

    void step() override {

        // /* ADSR */
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

struct AcidWidget : ModuleWidget {
    AcidWidget(Acid *module) : ModuleWidget(module) {
        setPanel(SVG::load(assetPlugin(plugin, "res/Acid.svg")));

        int BOTTOM_OFFSET = 50;
        int LEFT_BUFFER = 2;
        int RIGHT_BUFFER = 50;

        /*
            Left Side
        */

        // Wave
        addParam(ParamWidget::create<RoundLargeBlackKnob>(mm2px(Vec(5 + LEFT_BUFFER, 20)), module, Acid::WAVE_1_PARAM, 0.2, 0.4f, 0.50f));
        addParam(ParamWidget::create<RoundLargeBlackKnob>(mm2px(Vec(30 + LEFT_BUFFER, 20)), module, Acid::WAVE_2_PARAM, 0.2, 0.4f, 0.50f));
        addParam(ParamWidget::create<RoundLargeBlackKnob>(mm2px(Vec(17.5 + LEFT_BUFFER, 35)), module, Acid::WAVE_MIX_PARAM, 0.2, 0.4f, 0.50f));

        // Envelope
        addParam(ParamWidget::create<RoundLargeBlackKnob>(mm2px(Vec(5 + LEFT_BUFFER, 20 + BOTTOM_OFFSET)), module, Acid::ENV_REL_PARAM, 0.2, 0.4f, 0.50f));
        addParam(ParamWidget::create<RoundLargeBlackKnob>(mm2px(Vec(30 + LEFT_BUFFER, 20 + BOTTOM_OFFSET)), module, Acid::ENV_AMT_PARAM, 0.2, 0.4f, 0.50f));
        addParam(ParamWidget::create<RoundLargeBlackKnob>(mm2px(Vec(17.5 + LEFT_BUFFER, 35 + BOTTOM_OFFSET)), module, Acid::ENV_SHAPE_PARAM, 0.2, 0.4f, 0.50f));

        /*
            Right Side
        */
        // Filter
        addParam(ParamWidget::create<RoundLargeBlackKnob>(mm2px(Vec(17.5 + LEFT_BUFFER + RIGHT_BUFFER, 20)), module, Acid::FILTER_CUT_PARAM, 0.2, 0.4f, 0.50f));
        addParam(ParamWidget::create<RoundLargeBlackKnob>(mm2px(Vec(5 + LEFT_BUFFER + RIGHT_BUFFER, 35)), module, Acid::FILTER_FM_1_PARAM, 0.2, 0.4f, 0.50f));
        addParam(ParamWidget::create<RoundLargeBlackKnob>(mm2px(Vec(30 + LEFT_BUFFER + RIGHT_BUFFER, 35)), module, Acid::FILTER_Q_PARAM, 0.2, 0.4f, 0.50f));
        addParam(ParamWidget::create<RoundLargeBlackKnob>(mm2px(Vec(5 + LEFT_BUFFER + RIGHT_BUFFER, 50)), module, Acid::FILTER_FM_2_PARAM, 0.2, 0.4f, 0.50f));
        addParam(ParamWidget::create<RoundLargeBlackKnob>(mm2px(Vec(30 + LEFT_BUFFER + RIGHT_BUFFER, 50)), module, Acid::FILTER_DRIVE_PARAM, 0.2, 0.4f, 0.50f));

        // Pluck
        addParam(ParamWidget::create<RoundLargeBlackKnob>(mm2px(Vec(5 + LEFT_BUFFER + RIGHT_BUFFER, 35 + BOTTOM_OFFSET)), module, Acid::PLUCK_REL_PARAM, 0.2, 0.4f, 0.50f));
        addParam(ParamWidget::create<RoundLargeBlackKnob>(mm2px(Vec(30 + LEFT_BUFFER + RIGHT_BUFFER, 35 + BOTTOM_OFFSET)), module, Acid::PLUCK_EXP_PARAM, 0.2, 0.4f, 0.50f));

        /*
            Bottom
        */
        addInput(Port::create<PJ301MPort>(mm2px(Vec(5 + LEFT_BUFFER, 115)), Port::INPUT, module, Acid::VOCT_INPUT));
        addInput(Port::create<PJ301MPort>(mm2px(Vec(20 + LEFT_BUFFER, 115)), Port::INPUT, module, Acid::GATE_INPUT));
        addOutput(Port::create<PJ301MPort>(mm2px(Vec(35 + LEFT_BUFFER + RIGHT_BUFFER, 115)), Port::OUTPUT, module, Acid::OUT_OUTPUT));
    }
};


Model *modelAcid = Model::create<Acid, AcidWidget>("RJModules", "Acid", "[VCO] Acid", AMPLIFIER_TAG);
