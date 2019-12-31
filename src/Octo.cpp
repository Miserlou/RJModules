#include "RJModules.hpp"
#include "dsp/digital.hpp"
#include "plugin.hpp"
#include <iostream>
#include <cmath>
#include <sstream>
#include <iomanip>

using simd::float_4;

// DISPLAY
struct OctoRoundLargeBlackKnob : RoundLargeBlackKnob
{
    OctoRoundLargeBlackKnob()
    {
        setSVG(SVG::load(assetPlugin(pluginInstance, "res/KTFRoundLargeBlackKnob.svg")));
    }
};

struct OctoRoundSmallBlackKnob : RoundSmallBlackKnob
{
    OctoRoundSmallBlackKnob()
    {
        setSVG(SVG::load(assetPlugin(pluginInstance, "res/KTFRoundSmallBlackKnob.svg")));
    }
};

// LFO
template <typename T>
struct OctoLowFrequencyOscillator {
    T phase = 0.f;
    T pw = 0.5f;
    T freq = 1.f;
    bool invert = false;
    bool bipolar = false;
    T resetState = T::mask();

    void setPitch(T pitch) {
        pitch = simd::fmin(pitch, 10.f);
        freq = dsp::approxExp2_taylor5(pitch + 30) / 1073741824;
    }
    void setPulseWidth(T pw) {
        const T pwMin = 0.01f;
        this->pw = clamp(pw, pwMin, 1.f - pwMin);
    }
    void setReset(T reset) {
        reset = simd::rescale(reset, 0.1f, 2.f, 0.f, 1.f);
        T on = (reset >= 1.f);
        T off = (reset <= 0.f);
        T triggered = ~resetState & on;
        resetState = simd::ifelse(off, 0.f, resetState);
        resetState = simd::ifelse(on, T::mask(), resetState);
        phase = simd::ifelse(triggered, 0.f, phase);
    }
    void step(float dt) {
        T deltaPhase = simd::fmin(freq * dt, 0.5f);
        phase += deltaPhase;
        phase -= (phase >= 1.f) & 1.f;
    }
    T sin() {
        T p = phase;
        if (!bipolar)
            p -= 0.25f;
        T v = simd::sin(2 * M_PI * p);
        if (invert)
            v *= -1.f;
        if (!bipolar)
            v += 1.f;
        return v;
    }
    T tri() {
        T p = phase;
        // if (bipolar)
        //     p += 0.25f;
        T v = 4.f * simd::fabs(p - simd::round(p)) - 1.f;
        // if (invert)
        //     v *= -1.f;
        if (!bipolar)
            v += 1.f;
        return v;
    }
    T saw() {
        T p = phase;
        if (!bipolar)
            p -= 0.5f;
        T v = 2.f * (p - simd::round(p));
        if (invert)
            v *= -1.f;
        if (!bipolar)
            v += 1.f;
        return v;
    }
    T sqr() {
        T v = simd::ifelse(phase < pw, 1.f, -1.f);
        if (invert)
            v *= -1.f;
        if (!bipolar)
            v += 1.f;
        return v;
    }
    T light() {
        return simd::sin(2 * T(M_PI) * phase);
    }
};

// MODULE
struct Octo: Module {
    enum ParamIds {
        SPEED_PARAM,
        SPEED_ATTEN_PARAM,
        RESET_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        SPEED_CV,
        NUM_INPUTS
    };
    enum OutputIds {
        ENUMS(CH_OUTPUT, 8),
        NUM_OUTPUTS
    };
    enum LightIds {
        ENUMS(CH_LIGHT, 8),
        ENUMS(CH_NEG_LIGHT, 8),
        NUM_LIGHTS
    };

    OctoLowFrequencyOscillator<float_4> oscillators[8];
    dsp::ClockDivider lightDivider;
    float multiples[8];

    Octo() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(Octo::SPEED_PARAM, -10.0, 10.0, 0.0, "");
        configParam(Octo::SPEED_ATTEN_PARAM, 0.0, 1.0, 0.5, "");
        lightDivider.setDivision(128);

        multiples[0] = 1;
        multiples[1] = .75f;
        multiples[2] = .501f;
        multiples[3] = .24f;
        multiples[4] = .12f;
        multiples[5] = .04f;
        multiples[6] = .005f;
        multiples[7] = .0001f;

    }

    void process(const ProcessArgs& args) override {

        float pitch = params[SPEED_PARAM].value;
        for (int c = 0; c < 8; c++) {
            if(outputs[CH_OUTPUT + c].isConnected()){
                auto* oscillator = &oscillators[c];
                oscillator->setPitch(pitch * multiples[c]);
                oscillator->step(args.sampleTime);
                outputs[CH_OUTPUT + c].setVoltageSimd(oscillator->tri(), 0);
            }
        }

        if (lightDivider.process()) {
            for (int c = 0; c < 8; c++) {
                float lightValue = oscillators[c].light().s[0];
                lights[CH_LIGHT + c].setSmoothBrightness(lightValue, args.sampleTime * lightDivider.getDivision());
                lights[CH_NEG_LIGHT + c].setSmoothBrightness(-lightValue, args.sampleTime * lightDivider.getDivision());
            }
        }

    }
};

struct OctoWidget: ModuleWidget {
    OctoWidget(Octo *module);
};

OctoWidget::OctoWidget(Octo *module) {
    setModule(module);
    box.size = Vec(6*10, 380);

    {
        SVGPanel *panel = new SVGPanel();
        panel->box.size = box.size;
        panel->setBackground(SVG::load(assetPlugin(pluginInstance, "res/Octo.svg")));
        addChild(panel);
    }

    addParam(createParam<OctoRoundLargeBlackKnob>(Vec(12, 55), module, Octo::SPEED_PARAM));
    addParam(createParam<OctoRoundSmallBlackKnob>(Vec(5, 100), module, Octo::SPEED_ATTEN_PARAM));
    addInput(createPort<PJ301MPort>(Vec(32, 99), PortWidget::INPUT, module, Octo::SPEED_CV));

    int SPACE = 30;
    int BASE = 136;
    int LEFT = 18;
    addOutput(createPort<PJ301MPort>(Vec(LEFT, BASE), PortWidget::OUTPUT, module, Octo::CH_OUTPUT));
    addOutput(createPort<PJ301MPort>(Vec(LEFT, BASE + SPACE * 1), PortWidget::OUTPUT, module, Octo::CH_OUTPUT + 1));
    addOutput(createPort<PJ301MPort>(Vec(LEFT, BASE + SPACE * 2), PortWidget::OUTPUT, module, Octo::CH_OUTPUT + 2));
    addOutput(createPort<PJ301MPort>(Vec(LEFT, BASE + SPACE * 3), PortWidget::OUTPUT, module, Octo::CH_OUTPUT + 3));
    addOutput(createPort<PJ301MPort>(Vec(LEFT, BASE + SPACE * 4), PortWidget::OUTPUT, module, Octo::CH_OUTPUT + 4));
    addOutput(createPort<PJ301MPort>(Vec(LEFT, BASE + SPACE * 5), PortWidget::OUTPUT, module, Octo::CH_OUTPUT + 5));
    addOutput(createPort<PJ301MPort>(Vec(LEFT, BASE + SPACE * 6), PortWidget::OUTPUT, module, Octo::CH_OUTPUT + 6));
    addOutput(createPort<PJ301MPort>(Vec(LEFT, BASE + SPACE * 7), PortWidget::OUTPUT, module, Octo::CH_OUTPUT + 7));

    BASE = BASE + 8;
    int RIGHT = 46;
    LEFT = 5;
    addChild(createLight<MediumLight<GreenRedLight>>(Vec(RIGHT, BASE            ), module, Octo::CH_LIGHT + 0));
    addChild(createLight<MediumLight<GreenRedLight>>(Vec(LEFT,  BASE + SPACE * 1), module, Octo::CH_LIGHT + 1));
    addChild(createLight<MediumLight<GreenRedLight>>(Vec(RIGHT, BASE + SPACE * 2), module, Octo::CH_LIGHT + 2));
    addChild(createLight<MediumLight<GreenRedLight>>(Vec(LEFT,  BASE + SPACE * 3), module, Octo::CH_LIGHT + 3));
    addChild(createLight<MediumLight<GreenRedLight>>(Vec(RIGHT, BASE + SPACE * 4), module, Octo::CH_LIGHT + 4));
    addChild(createLight<MediumLight<GreenRedLight>>(Vec(LEFT,  BASE + SPACE * 5), module, Octo::CH_LIGHT + 5));
    addChild(createLight<MediumLight<GreenRedLight>>(Vec(RIGHT, BASE + SPACE * 6), module, Octo::CH_LIGHT + 6));
    addChild(createLight<MediumLight<GreenRedLight>>(Vec(LEFT,  BASE + SPACE * 7), module, Octo::CH_LIGHT + 7));

}

Model *modelOcto = createModel<Octo, OctoWidget>("Octo");
