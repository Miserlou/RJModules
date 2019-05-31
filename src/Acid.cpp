/*
A C I D

aka BADACID

2x Wave + Envelope/VCA + FM Filter + Acid

Elements pinched from Lindenberg + dBiZ

*/

#include "RJModules.hpp"

#include <iostream>

#include "Oscillator.hpp"
#include "dsp/digital.hpp"

#include <math.h>

using dsp::DSPBLOscillator;
#define pi 3.14159265359


/*
    UI
*/
struct RoundLargeBlackSnapKnob : RoundLargeBlackKnob
{
    RoundLargeBlackSnapKnob()
    {
        minAngle = -0.83 * M_PI;
        maxAngle = 0.83 * M_PI;
        snap = true;
    }
};

/*
    Static Functions and Structions
*/
static float shapeDelta(float delta, float tau, float shape) {
    float lin = sgn(delta) * 10.0 / tau;
    if (shape < 0.0) {
        float log = sgn(delta) * 40.0 / tau / (fabsf(delta) + 1.0);
        return crossfade(lin, log, -shape * 0.95);
    }
    else {
        float exp = M_E * delta / tau;
        return crossfade(lin, exp, shape * 0.90);
    }
}

struct MultiFilter
{
    float q;
    float freq;
    float smpRate;
    float hp = 0.0f,bp = 0.0f,lp = 0.0f,mem1 = 0.0f,mem2 = 0.0f;

    void setParams(float freq, float q, float smpRate) {
        this->freq = freq;
        this->q=q;
        this->smpRate=smpRate;
    }

    void calcOutput(float sample)
    {
        float g = tan(pi*freq/smpRate);
        float R = 1.0f/(2.0f*q);
        hp = (sample - (2.0f*R + g)*mem1 - mem2)/(1.0f + 2.0f*R*g + g*g);
        bp = g*hp + mem1;
        lp = g*bp +  mem2;
        mem1 = g*hp + bp;
        mem2 = g*bp + lp;
    }

};

/*
    Modules
*/
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
        ENV_OUTPUT,
        NUM_OUTPUTS
    };
    enum LightIds {
        ATTACK_LIGHT,
        DECAY_LIGHT,
        SUSTAIN_LIGHT,
        RELEASE_LIGHT,
        NUM_LIGHTS
};

    // Wave
    DSPBLOscillator *osc1 = new DSPBLOscillator(engineGetSampleRate());
    DSPBLOscillator *osc2 = new DSPBLOscillator(engineGetSampleRate());

    // Env
    SchmittTrigger env_trigger;
    float env_out = 0.0f;
    float env_gate = 0.0f;
    float env_in = 0.0f;

    // VCA
    float vca_out;
    float vca_last;

    // Filter
    MultiFilter filter;

    // Pluck
    float lastCv = 0.f;
    bool decaying = false;
    float env = 0.0f;

    Acid() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {}

    void step() override {

        /*
            Inputs
        */
        float voct = inputs[VOCT_INPUT].value;

        /*
            Wave
        */

        // OSC1
        float wave1 = params[WAVE_1_PARAM].value;
        osc1->setInputs(voct, 0.0, 0.0, 0.f, -2.f);
        osc1->process();
        float osc1_out;
        switch(int(wave1)){

            // Sin
            case 0:
                osc1_out = osc1->getSineWave();
                break;
            // Saw
            case 1:
                osc1_out = osc1->getSawWave();
                break;
            // Pulse
            case 2:
                osc1_out = osc1->getPulseWave();
                break;
            // Tri
            case 3:
                osc1_out = osc1->getTriWave();
                break;
            // Noise
            case 4:
                osc1_out = osc1->getNoise();
                break;
        }

        // OSC2
        float wave2 = params[WAVE_2_PARAM].value;
        osc2->setInputs(voct, 0.0, 0.0, 0.f, -2.f);
        osc2->process();
        float osc2_out;
        switch(int(wave2)){

            // Sin
            case 0:
                osc2_out = osc2->getSineWave();
                break;
            // Saw
            case 1:
                osc2_out = osc2->getSawWave();
                break;
            // Pulse
            case 2:
                osc2_out = osc2->getPulseWave();
                break;
            // Tri
            case 3:
                osc2_out = osc2->getTriWave();
                break;
            // Noise
            case 4:
                osc2_out = osc2->getNoise();
                break;
        }

        // Mix
        float wave_mixed = ((1 - params[WAVE_MIX_PARAM].value) * osc1_out) + ((params[WAVE_MIX_PARAM].value) * osc2_out);

        /*
            Envelope
        */
        float shape = params[ENV_SHAPE_PARAM].value;
        float minTime = 1e-2;
        env_in = 0.0f;

        // Trigger
        // if (env_trigger.process(inputs[TRIG_INPUT].value)) {
        if (inputs[TRIG_INPUT].value >= 1.0f) {
            env_gate = true;
            // std::cout << "TRIGGING\n";
        // } else {
        //     env_gate = false;
        //     // std::cout << "NO TRIGGING \n";
        // }
        }
        if (env_gate) {
            env_in = 10.0;
        } else{
            // env_in = 0.0;
        }

        bool rising = false;
        bool falling = false;
        float delta = env_in - env_out;
        if (delta > 0) {
            // std::cout << "RISING!\n" << delta;
            // Rise
            // float riseCv = params[RISE_PARAM].value + inputs[RISE_INPUT].value / 10.0;
            // float riseCv = params[RISE_PARAM].value;
            float riseCv = 0.0;
            //riseCv = clamp(riseCv, 0.0, 1.0);
            // riseCv = 1.0;
            float rise = minTime * powf(2.0, riseCv * 10.0);
            env_out += shapeDelta(delta, rise, shape) / engineGetSampleRate();
            rising = (env_in - env_out > 1e-3);
            if (!rising) {
                env_gate = false;
            }
        }
        else if (delta < 0) {
            //std::cout << "Falling!\n";
            // Fall
            //float fallCv = params[FALL_PARAM].value + inputs[FALL_INPUT].value / 10.0;
            float fallCv = params[ENV_REL_PARAM].value + 0.0 / 10.0;
            fallCv = clamp(fallCv, 0.0, 1.0);
            float fall = minTime * powf(2.0, fallCv * 10.0);
            env_out += shapeDelta(delta, fall, shape) / engineGetSampleRate();
            falling = (env_in - env_out < -1e-3);
        }
        else {
            env_gate = false;
        }

        if (!rising && !falling) {
            env_out = env_in;
        }

        outputs[ENV_OUTPUT].value = env_out;

        /*
            VCA
            via https://github.com/VCVRack/AudibleInstruments/blob/dd25b1785c2e67f19824fad97527c97c5d779685/src/Veils.cpp
        */
        float vca_out = wave_mixed * env_out * params[ENV_AMT_PARAM].value;

        /*
            Filter
        */

        // Stage 1
        //float cutoff = pow(2.0f, rescale(clamp(params[FILTER_CUT_PARAM].value + quadraticBipolar(params[FILTER_FM_2_PARAM].value) * 0.1f * inputs[CUTOFF_INPUT2].value + quadraticBipolar(params[FILTER_FM_PARAM].value) * 0.1f * inputs[CUTOFF_INPUT].value / 5.0f, 0.0f , 1.0f), 0.0f, 1.0f, 4.5f, 13.0f));
        float cutoff = pow(2.0f, rescale(clamp(params[FILTER_CUT_PARAM].value + quadraticBipolar(params[FILTER_FM_2_PARAM].value) * 0.1f * vca_out + quadraticBipolar(params[FILTER_FM_1_PARAM].value) * 0.1f * vca_out / 5.0f, 0.0f , 1.0f), 0.0f, 1.0f, 4.5f, 13.0f));

        //float q = 10.0f * clamp(params[FILTER_Q_PARAM].value + inputs[Q_INPUT].value / 5.0f, 0.1f, 1.0f);
        float q = 10.0f * clamp(params[FILTER_Q_PARAM].value / 5.0f, 0.1f, 1.0f);
        filter.setParams(cutoff, q, engineGetSampleRate());
        float in = vca_out / 5.0f;

        // Stage 2
        in = clamp(in, -5.0f, 5.0f) * 0.2f;
        float a_shape = params[FILTER_DRIVE_PARAM].value;
        a_shape = clamp(a_shape, -5.0f, 5.0f) * 0.2f;
        a_shape *= 0.99f;
        const float a_shapeB = (1.0 - a_shape) / (1.0 + a_shape);
        const float a_shapeA = (4.0 * a_shape) / ((1.0 - a_shape) * (1.0 + a_shape));
        float a_outputd = in * (a_shapeA + a_shapeB);
        a_outputd = a_outputd / ((std::abs(in) * a_shapeA) + a_shapeB);
        filter.calcOutput(a_outputd);
        float filter_out = filter.lp * 3.0f;

        /*
            Pluck
        */

        /*
            Outputs
        */

        outputs[OUT_OUTPUT].value = filter_out;

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
        addParam(ParamWidget::create<RoundLargeBlackSnapKnob>(mm2px(Vec(5 + LEFT_BUFFER, 20)), module, Acid::WAVE_1_PARAM, 0.0, 4.0, 0.0));
        addParam(ParamWidget::create<RoundLargeBlackSnapKnob>(mm2px(Vec(30 + LEFT_BUFFER, 20)), module, Acid::WAVE_2_PARAM, 0.0, 4.0, 0.0));
        addParam(ParamWidget::create<RoundLargeBlackKnob>(mm2px(Vec(17.5 + LEFT_BUFFER, 35)), module, Acid::WAVE_MIX_PARAM, 0.0, 1.0f, 0.0f));

        // Envelope
        addParam(ParamWidget::create<RoundLargeBlackKnob>(mm2px(Vec(5 + LEFT_BUFFER, 20 + BOTTOM_OFFSET)), module, Acid::ENV_REL_PARAM, 0.0, 1.0, 0.0));
        addParam(ParamWidget::create<RoundLargeBlackKnob>(mm2px(Vec(30 + LEFT_BUFFER, 20 + BOTTOM_OFFSET)), module, Acid::ENV_AMT_PARAM, 0.0, 1.0, 1.0));
        addParam(ParamWidget::create<RoundLargeBlackKnob>(mm2px(Vec(17.5 + LEFT_BUFFER, 35 + BOTTOM_OFFSET)), module, Acid::ENV_SHAPE_PARAM, -1.0, 1.0, 0.0));

        /*
            Right Side
        */

        // Filter
        addParam(ParamWidget::create<RoundLargeBlackKnob>(mm2px(Vec(17.5 + LEFT_BUFFER + RIGHT_BUFFER, 20)), module, Acid::FILTER_CUT_PARAM, 0.0, 1.0f, 0.90f));
        addParam(ParamWidget::create<RoundLargeBlackKnob>(mm2px(Vec(5 + LEFT_BUFFER + RIGHT_BUFFER, 35)), module, Acid::FILTER_FM_1_PARAM, -1.0, 1.0f, 0.0f));
        addParam(ParamWidget::create<RoundLargeBlackKnob>(mm2px(Vec(30 + LEFT_BUFFER + RIGHT_BUFFER, 35)), module, Acid::FILTER_Q_PARAM, 0.1f, 1.5f, 0.3f));
        addParam(ParamWidget::create<RoundLargeBlackKnob>(mm2px(Vec(5 + LEFT_BUFFER + RIGHT_BUFFER, 50)), module, Acid::FILTER_FM_2_PARAM, -1.0, 1.0f, 0.0f));
        addParam(ParamWidget::create<RoundLargeBlackKnob>(mm2px(Vec(30 + LEFT_BUFFER + RIGHT_BUFFER, 50)), module, Acid::FILTER_DRIVE_PARAM, -5.0f, 5.0f, 5.0f));

        // Pluck
        addParam(ParamWidget::create<RoundLargeBlackKnob>(mm2px(Vec(5 + LEFT_BUFFER + RIGHT_BUFFER, 35 + BOTTOM_OFFSET)), module, Acid::PLUCK_REL_PARAM, 0.2, 0.4f, 0.50f));
        addParam(ParamWidget::create<RoundLargeBlackKnob>(mm2px(Vec(30 + LEFT_BUFFER + RIGHT_BUFFER, 35 + BOTTOM_OFFSET)), module, Acid::PLUCK_EXP_PARAM, 0.2, 0.4f, 0.50f));

        /*
            Bottom
        */
        addInput(Port::create<PJ301MPort>(mm2px(Vec(5 + LEFT_BUFFER, 115)), Port::INPUT, module, Acid::VOCT_INPUT));
        addInput(Port::create<PJ301MPort>(mm2px(Vec(20 + LEFT_BUFFER, 115)), Port::INPUT, module, Acid::TRIG_INPUT));
        addOutput(Port::create<PJ301MPort>(mm2px(Vec(35 + LEFT_BUFFER + RIGHT_BUFFER - 15, 115)), Port::OUTPUT, module, Acid::ENV_OUTPUT));
        addOutput(Port::create<PJ301MPort>(mm2px(Vec(35 + LEFT_BUFFER + RIGHT_BUFFER, 115)), Port::OUTPUT, module, Acid::OUT_OUTPUT));
    }
};


Model *modelAcid = Model::create<Acid, AcidWidget>("RJModules", "Acid", "[VCO] Acid", AMPLIFIER_TAG);
