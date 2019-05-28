/*
Pluck is an integrated VCA + "ADSR". Hacked up from Fundamental.
*/

#include "RJModules.hpp"
#include "dsp/digital.hpp"

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
    SchmittTrigger trigger;

    Pluck() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {}

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
                    env += powf(base, 1 - decay) / maxTime * (sustain - env) * engineGetSampleTime();
                }
            }
            else {
                // Attack
                // Skip ahead if attack is all the way down (infinitely fast)
                if (attack < 1e-4) {
                    env = 1.0f;
                }
                else {
                    env += powf(base, 1 - attack) / maxTime * (1.01f - env) * engineGetSampleTime();
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
                env += powf(base, 1 - release) / maxTime * (0.0f - env) * engineGetSampleTime();
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


struct PluckVUKnob : Knob {
    PluckVUKnob() {
        box.size = mm2px(Vec(10, 20));
    }

    void draw(NVGcontext *vg) override {
        nvgBeginPath(vg);
        nvgRoundedRect(vg, 0, 0, box.size.x, box.size.y, 2.0);
        nvgFillColor(vg, nvgRGB(0, 0, 0));
        nvgFill(vg);

        Pluck *module = dynamic_cast<Pluck*>(this->module);

        const int segs = 25;
        const Vec margin = Vec(4, 4);
        Rect r = box.zeroPos().shrink(margin);

        for (int i = 0; i < segs; i++) {
            float segValue = clamp(value * segs - (segs - i - 1), 0.f, 1.f);
            float amplitude = value * module->lastCv;
            float segAmplitude = clamp(amplitude * segs - (segs - i - 1), 0.f, 1.f);
            nvgBeginPath(vg);
            nvgRect(vg, r.pos.x, r.pos.y + r.size.y / segs * i + 0.5,
                r.size.x, r.size.y / segs - 1.0);
            if (segValue > 0.f) {
                nvgFillColor(vg, colorAlpha(nvgRGBf(0.33, 0.33, 0.33), segValue));
                nvgFill(vg);
            }
            if (segAmplitude > 0.f) {
                nvgFillColor(vg, colorAlpha(COLOR_GREEN, segAmplitude));
                nvgFill(vg);
            }
        }
    }
};


struct PluckWidget : ModuleWidget {
    PluckWidget(Pluck *module) : ModuleWidget(module) {
        setPanel(SVG::load(assetPlugin(plugin, "res/Pluck.svg")));

        addChild(Widget::create<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
        addChild(Widget::create<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
        addChild(Widget::create<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
        addChild(Widget::create<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

        addParam(ParamWidget::create<PluckVUKnob>(mm2px(Vec(2.62103, 12.31692)), module, Pluck::LEVEL_PARAM, 0.0, 1.0, 1.0));

        addParam(ParamWidget::create<RoundSmallBlackKnob>(mm2px(Vec(3.5, 38.9593)), module, Pluck::RELEASE_PARAM, 0.2, 0.4f, 0.50f));
        addInput(Port::create<PJ301MPort>(mm2px(Vec(3.51398, 48.74977)), Port::INPUT, module, Pluck::RELEASE_INPUT));

        addParam(ParamWidget::create<RoundSmallBlackKnob>(mm2px(Vec(3.5, 61.9593)), module, Pluck::EXP_PARAM, 0.0001f, .2f, 4.0f));
        addInput(Port::create<PJ301MPort>(mm2px(Vec(3.51398, 71.74977)), Port::INPUT, module, Pluck::EXP_INPUT));

        addInput(Port::create<PJ301MPort>(mm2px(Vec(3.51398, 84.74977)), Port::INPUT, module, Pluck::GATE_INPUT));
        addInput(Port::create<PJ301MPort>(mm2px(Vec(3.51398, 97.74977)), Port::INPUT, module, Pluck::IN_INPUT));
        addOutput(Port::create<PJ301MPort>(mm2px(Vec(3.51398, 108.64454)), Port::OUTPUT, module, Pluck::OUT_OUTPUT));
    }
};


Model *modelPluck = Model::create<Pluck, PluckWidget>("RJModules", "Pluck", "[VCA] Pluck", AMPLIFIER_TAG);
