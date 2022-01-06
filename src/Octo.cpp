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
        setSVG(APP->window->loadSvg(asset::plugin(pluginInstance, "res/KTFRoundLargeBlackKnob.svg")));
    }
};

struct OctoRoundSmallBlackKnob : RoundSmallBlackKnob
{
    OctoRoundSmallBlackKnob()
    {
        setSVG(APP->window->loadSvg(asset::plugin(pluginInstance, "res/KTFRoundSmallBlackKnob.svg")));
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
        freq = dsp::approxExp2_taylor5(pitch + 30) / 60073741824;
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
        // if (bipolar)
        //     p += 0.25f;
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
        if (invert)
            v *= -1.f;
        if (!bipolar)
            v += 1.f;
        return v;
    }
    T saw() {
        T p = phase;
        // if (bipolar)
        //     p += 0.25f;
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
        ENUMS(CH_LIGHT, 16),
        NUM_LIGHTS
    };

    OctoLowFrequencyOscillator<float_4> oscillators[8];
    dsp::ClockDivider lightDivider;
    float multiples[8];
    rack::simd::Vector<float, 4> samples[8];
    bool hold = false;

    rack::simd::Vector<float, 4> outs[8];
    int frame_counter = 0;
    bool force_update = false;
    int COUNTER_MAX = 5000;
    int wave_mode_index = 0;

    Octo() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(Octo::SPEED_PARAM, 0.0, 10.0, 5.0, "Speed");
        configParam(Octo::SPEED_ATTEN_PARAM, -1.0, 1.0, 1.0, "Attenuvertion");
        lightDivider.setDivision(2048);

        multiples[0] = 1;
        multiples[1] = .85f;
        oscillators[1].invert = true;
        multiples[2] = .701f;
        multiples[3] = .54f;
        oscillators[3].invert = true;
        multiples[4] = .42f;
        multiples[5] = .3333f;
        oscillators[5].invert = true;
        multiples[6] = .1f;
        multiples[7] = .004f;
        oscillators[7].invert = true;

    }

    void process(const ProcessArgs& args) override {

        frame_counter++;
        if (frame_counter >= COUNTER_MAX){
            frame_counter = 0;
            force_update = true;
        }

        float pitch = params[SPEED_PARAM].value;
        float cv_voltage = inputs[SPEED_CV].value;
        float cv_amt = params[SPEED_ATTEN_PARAM].getValue();
        bool hold_now = false;

        cv_amt = cv_amt * cv_voltage;
        if (cv_amt <= -5.0){
            hold_now = true;
        } else {
            hold = false;
        }

        bool isOutputConnected = false;
        for (int c = 0; c < 8; c++) {
            isOutputConnected = outputs[CH_OUTPUT + c].isConnected();
            if(isOutputConnected || force_update){
                auto* oscillator = &oscillators[c];
                oscillator->setPitch(clamp( ((pitch + cv_voltage) * multiples[c]), 0.0f, 20.0f));
                if(isOutputConnected){
                    oscillator->step(args.sampleTime);
                } else{
                    oscillator->step(args.sampleTime * COUNTER_MAX);
                }
                if(hold != true){
                    switch(wave_mode_index){
                    case 0:
                        outs[c] = oscillator->tri();
                        break;
                    case 1:
                        outs[c] = oscillator->sqr();
                        break;
                    case 2:
                        outs[c] = oscillator->saw();
                        break;
                    case 3:
                        outs[c] = oscillator->sin();
                        break;
                    }
                    samples[c] = outs[c];
                } else{
                    outs[c] = samples[c];
                }
                outputs[CH_OUTPUT + c].setVoltageSimd(outs[c] * 2.5, 0);
            }
        }

        if (lightDivider.process() || (force_update == true)) {
            for (int c = 0; c < 8; c++) {
                float lightValue = outs[c][0];
                if(force_update == false){
                    lights[CH_LIGHT + c].setSmoothBrightness(lightValue / 2.5, args.sampleTime * lightDivider.getDivision());
                } else{
                    lights[CH_LIGHT + c].setBrightness(lightValue / 2.5);
                }
            }
        }

        if(force_update == true){
            force_update = false;
        }
        if(hold_now == true){
            hold = true;
        }

    }
};

struct OctoWidget: ModuleWidget {
  OctoWidget(Octo *module) {
        setModule(module);
        box.size = Vec(6*10, 380);

        {
            SVGPanel *panel = new SVGPanel();
            panel->box.size = box.size;
            panel->setBackground(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Octo.svg")));
            addChild(panel);
        }

        addParam(createParam<OctoRoundLargeBlackKnob>(Vec(12, 55), module, Octo::SPEED_PARAM));
        addParam(createParam<OctoRoundSmallBlackKnob>(Vec(5, 100), module, Octo::SPEED_ATTEN_PARAM));
        addInput(createInput<PJ301MPort>(Vec(32, 99), module, Octo::SPEED_CV));

        int SPACE = 30;
        int BASE = 136;
        int LEFT = 18;
        addOutput(createOutput<PJ301MPort>(Vec(LEFT, BASE), module, Octo::CH_OUTPUT));
        addOutput(createOutput<PJ301MPort>(Vec(LEFT, BASE + SPACE * 1), module, Octo::CH_OUTPUT + 1));
        addOutput(createOutput<PJ301MPort>(Vec(LEFT, BASE + SPACE * 2), module, Octo::CH_OUTPUT + 2));
        addOutput(createOutput<PJ301MPort>(Vec(LEFT, BASE + SPACE * 3), module, Octo::CH_OUTPUT + 3));
        addOutput(createOutput<PJ301MPort>(Vec(LEFT, BASE + SPACE * 4), module, Octo::CH_OUTPUT + 4));
        addOutput(createOutput<PJ301MPort>(Vec(LEFT, BASE + SPACE * 5), module, Octo::CH_OUTPUT + 5));
        addOutput(createOutput<PJ301MPort>(Vec(LEFT, BASE + SPACE * 6), module, Octo::CH_OUTPUT + 6));
        addOutput(createOutput<PJ301MPort>(Vec(LEFT, BASE + SPACE * 7), module, Octo::CH_OUTPUT + 7));

        BASE = BASE + 8;
        int RIGHT = 46;
        LEFT = 5;
        addChild(createLight<MediumLight<WhiteLight>>(Vec(RIGHT, BASE            ), module, Octo::CH_LIGHT + 0));
        addChild(createLight<MediumLight<WhiteLight>>(Vec(LEFT,  BASE + SPACE * 1), module, Octo::CH_LIGHT + 1));
        addChild(createLight<MediumLight<WhiteLight>>(Vec(RIGHT, BASE + SPACE * 2), module, Octo::CH_LIGHT + 2));
        addChild(createLight<MediumLight<WhiteLight>>(Vec(LEFT,  BASE + SPACE * 3), module, Octo::CH_LIGHT + 3));
        addChild(createLight<MediumLight<WhiteLight>>(Vec(RIGHT, BASE + SPACE * 4), module, Octo::CH_LIGHT + 4));
        addChild(createLight<MediumLight<WhiteLight>>(Vec(LEFT,  BASE + SPACE * 5), module, Octo::CH_LIGHT + 5));
        addChild(createLight<MediumLight<WhiteLight>>(Vec(RIGHT, BASE + SPACE * 6), module, Octo::CH_LIGHT + 6));
        addChild(createLight<MediumLight<WhiteLight>>(Vec(LEFT,  BASE + SPACE * 7), module, Octo::CH_LIGHT + 7));
    }

    json_t *toJson() {
        json_t *rootJ = ModuleWidget::toJson();
        Octo *module = dynamic_cast<Octo *>(this->module);
        json_object_set_new(rootJ, "wave", json_real(module->wave_mode_index));
        return rootJ;
    }

    void fromJson(json_t *rootJ) {
        ModuleWidget::fromJson(rootJ);
        json_t *waveJ = json_object_get(rootJ, "wave");
        Octo *module = dynamic_cast<Octo *>(this->module);
        if (waveJ)
            module->wave_mode_index = json_number_value(waveJ);
    }

    void appendContextMenu(Menu *menu) override
    {
        Octo *module = dynamic_cast<Octo *>(this->module);

        struct WaveIndexItem : MenuItem
        {
            Octo *module;
            int index;
            void onAction(const event::Action &e) override
            {
                module->wave_mode_index = index;
            }
        };

        struct WaveItem : MenuItem
        {
            Octo *module;
            Menu *createChildMenu() override
            {
                Menu *menu = new Menu();
                const std::string feedbackLabels[] = {
                    "Triangle",
                    "Square",
                    "Saw",
                    "Sin"
                };
                for (int i = 0; i < (int)LENGTHOF(feedbackLabels); i++)
                {
                    WaveIndexItem *item = createMenuItem<WaveIndexItem>(feedbackLabels[i], CHECKMARK(module->wave_mode_index == i));
                    item->module = module;
                    item->index = i;
                    menu->addChild(item);
                }
                return menu;
            }
        };

        menu->addChild(new MenuEntry);

        WaveItem *waveItem = createMenuItem<WaveItem>("Wave Mode", ">");
        waveItem->module = module;
        menu->addChild(waveItem);

    }

};

Model *modelOcto = createModel<Octo, OctoWidget>("Octo");
