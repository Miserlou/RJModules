/*
Pluck is an integrated VCA + "ADSR". Hacked up from Fundamental.
*/

#include "RJModules.hpp"

struct Pluck : Module {
    enum ParamIds {
        LEVEL_PARAM,

        ATTACK_PARAM,
        DECAY_PARAM,
        SUSTAIN_PARAM,
        RELEASE_PARAM,
        EXP_PARAM,

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
    dsp::SchmittTrigger trigger;

    Pluck() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(Pluck::LEVEL_PARAM, 0.0, 1.0, 1.0, "");
        configParam(Pluck::RELEASE_PARAM, 0.2, 0.4f, 0.50f, "");
        configParam(Pluck::EXP_PARAM, 0.0001f, .2f, 4.0f, "");
    }

    void step() override {

        /* ADSR */
        float attack = 0.005f;
        float decay = 10.0f;
        float sustain = 10.0f;
        float release = clamp(params[RELEASE_PARAM].value + inputs[RELEASE_INPUT].value / 10.0f, 0.0f, 1.0f);

        // Gate and trigger
        bool gated = inputs[GATE_INPUT].value >= 1.0f;
        if (trigger.process(inputs[TRIG_INPUT].value))
            decaying = false;

        const float base = 20000.0f;
        const float maxTime = 20.0f;
        if (gated) {
            if (decaying) {
                // Decay
                if (decay < 1e-4) {
                    env = sustain;
                }
                else {
                    env += powf(base, 1 - decay) / maxTime * (sustain - env) * APP->engine->getSampleTime();
                }
            }
            else {
                // Attack
                // Skip ahead if attack is all the way down (infinitely fast)
                if (attack < 1e-4) {
                    env = 1.0f;
                }
                else {
                    env += powf(base, 1 - attack) / maxTime * (1.01f - env) * APP->engine->getSampleTime();
                }
                if (env >= 1.0f) {
                    env = 1.0f;
                    decaying = true;
                }
            }
        }
        else {
            // Release
            if (release < 1e-4) {
                env = 0.0f;
            }
            else {
                env += powf(base, 1 - release) / maxTime * (0.0f - env) * APP->engine->getSampleTime();
            }
            decaying = false;
        }

        bool sustaining = isNear(env, sustain, 1e-3);
        bool resting = isNear(env, 0.0f, 1e-3);
        float env_output = 10.0f * env;

        /* VCA */
        float cv = fmaxf(env_output / 10.f, 0.f);
        float exp_val =  clamp(params[EXP_PARAM].value + inputs[EXP_PARAM].value / 10.0f, 0.0f, 1.0f);
        cv = powf(cv, exp_val);
        lastCv = cv;
        outputs[OUT_OUTPUT].value = inputs[IN_INPUT].value * cv;

    }
};

struct PluckVUKnob : SliderKnob {
    Pluck *module = NULL;

    PluckVUKnob() {
        box.size = mm2px(Vec(10, 20));
    }

    void draw(const DrawArgs &args) override {
        float amplitude = module ? module->lastCv : 1.f;

        nvgBeginPath(args.vg);
        nvgRoundedRect(args.vg, 0, 0, box.size.x, box.size.y, 2.0);
        nvgFillColor(args.vg, nvgRGB(0, 0, 0));
        nvgFill(args.vg);

        const int segs = 25;
        const Vec margin = Vec(3, 3);
        Rect r = box.zeroPos().grow(margin.neg());

        for (int i = 0; i < segs; i++) {
            float value = getParamQuantity() ? getParamQuantity()->getValue() : 1.f;
            float segValue = clamp(value * segs - (segs - i - 1), 0.f, 1.f);
            float segAmplitude = clamp(amplitude * segs - (segs - i - 1), 0.f, 1.f);
            nvgBeginPath(args.vg);
            nvgRect(args.vg, r.pos.x, r.pos.y + r.size.y / segs * i + 0.5,
                r.size.x, r.size.y / segs - 1.0);
            if (segValue > 0.f) {
                nvgFillColor(args.vg, color::alpha(nvgRGBf(0.33, 0.33, 0.33), segValue));
                nvgFill(args.vg);
            }
            if (segAmplitude > 0.f) {
                nvgFillColor(args.vg, color::alpha(SCHEME_GREEN, segAmplitude));
                nvgFill(args.vg);
            }
        }
    }
};

struct PluckWidget : ModuleWidget {
    PluckWidget(Pluck *module) {
		setModule(module);
        setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Pluck.svg")));

        addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

        addParam(createParam<PluckVUKnob>(mm2px(Vec(2.62103, 12.31692)), module, Pluck::LEVEL_PARAM));

        addParam(createParam<RoundSmallBlackKnob>(mm2px(Vec(3.5, 38.9593)), module, Pluck::RELEASE_PARAM));
        addInput(createInput<PJ301MPort>(mm2px(Vec(3.51398, 48.74977)), module, Pluck::RELEASE_INPUT));

        addParam(createParam<RoundSmallBlackKnob>(mm2px(Vec(3.5, 61.9593)), module, Pluck::EXP_PARAM));
        addInput(createInput<PJ301MPort>(mm2px(Vec(3.51398, 71.74977)), module, Pluck::EXP_INPUT));

        addInput(createInput<PJ301MPort>(mm2px(Vec(3.51398, 84.74977)), module, Pluck::GATE_INPUT));
        addInput(createInput<PJ301MPort>(mm2px(Vec(3.51398, 97.74977)), module, Pluck::IN_INPUT));
        addOutput(createOutput<PJ301MPort>(mm2px(Vec(3.51398, 108.64454)), module, Pluck::OUT_OUTPUT));
    }
};


Model *modelPluck = createModel<Pluck, PluckWidget>("Pluck");
