#include "RJModules.hpp"
#include "dsp/digital.hpp"
#include "plugin.hpp"
#include <iostream>
#include <cmath>
#include <sstream>
#include <iomanip>
#include <random>

using simd::float_4;

// DISPLAY
struct GaussianRoundLargeBlackKnob : RoundLargeBlackKnob
{
    GaussianRoundLargeBlackKnob()
    {
        setSVG(APP->window->loadSvg(asset::plugin(pluginInstance, "res/KTFRoundLargeBlackKnob.svg")));
    }
};

struct GaussianRoundSmallBlackKnob : RoundSmallBlackKnob
{
    GaussianRoundSmallBlackKnob()
    {
        setSVG(APP->window->loadSvg(asset::plugin(pluginInstance, "res/KTFRoundSmallBlackKnob.svg")));
    }
};

template <typename BASE>
struct LittleLight : BASE {
        LittleLight() {
                this->box.size = mm2px(Vec(34, 34));
        }
};

// MODULE
struct Gaussian: Module {
    enum ParamIds {
        MU_PARAM,
        SIGMA_PARAM,
        BUTTON_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        TRIGGER_INPUT,
        NUM_INPUTS
    };
    enum OutputIds {
        ENUMS(CH_OUTPUT, 9),
        NUM_OUTPUTS
    };
    enum LightIds {
        ENUMS(CH_LIGHT, 36),
        BUTTON_LIGHT,
        NUM_LIGHTS
    };

    dsp::ClockDivider lightDivider;
    float multiples[8];
    rack::simd::Vector<float, 4> samples[8];
    bool hold = false;

    rack::simd::Vector<float, 4> outs[8];
    int frame_counter = 0;
    bool force_update = false;
    int COUNTER_MAX = 5000;
    int wave_mode_index = 0;

    float last_mu = -1;
    float last_sigma = -1;
    int hist[9] = {0,0,0,0,0,0,0,0,0};
    float genny = 0.0;
    std::random_device rd{};
    std::mt19937 gen{rd()};
    dsp::SchmittTrigger inTrigger;
    dsp::SchmittTrigger btnTrigger;
    int lightValue = 0.0;

    int result = -1;
    bool triggered = false;
    const float lightLambda = 0.075;

    Gaussian() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(Gaussian::MU_PARAM, 0.0f, 10.0f, 5.0f, "Mu");
        configParam(Gaussian::SIGMA_PARAM, .5f, 6.0f, 1.5f, "Sigma");
        configParam(Gaussian::BUTTON_PARAM, 0.0f, 1.0f, 0.0f, "Button");
        lightDivider.setDivision(2048);
    }

    void process(const ProcessArgs& args) override {

        frame_counter++;
        if (frame_counter >= COUNTER_MAX){
            frame_counter = 0;
            force_update = true;
        }

        float mu = params[MU_PARAM].value;
        float sigma = params[SIGMA_PARAM].value;

        std::normal_distribution<double> d{mu, sigma};

        // Distribution visualizer
        if(mu != last_mu || sigma != last_sigma){
            for (int i=0; i<9; i++) {
                hist[i] = 0;
            }
            for(int n=0; n<100; ++n) {
                hist[std::lrint(d(gen)) - 1]++;
            }
            for (int i=0; i<9; i++) {
                lights[9 + CH_LIGHT + i].setBrightness(hist[i]/10.0f);
            }
        }

        // Meat
        if (btnTrigger.process(params[BUTTON_PARAM].value) || inTrigger.process(inputs[TRIGGER_INPUT].value)) {

            bool good = false;
            int result;

            while(!good){
                result = std::round(d(gen)) - 1;
                if(result < 0 || result > 9){
                    good = false;
                } else{
                    good = true;
                }
            }

            for (int c = 0; c < 9; c++) {
                if(c == result){
                    lightValue = 1.0;
                    outputs[CH_OUTPUT + c].value = 5.0;
                } else{
                    lightValue = 0.0;
                    outputs[CH_OUTPUT + c].value = 0.0;
                }
                lights[CH_LIGHT + c].setBrightness(lightValue);
            }
        }

        if(force_update == true){
            force_update = false;
        }
        last_sigma = sigma;
        last_mu = mu;

    }
};

struct GaussianWidget: ModuleWidget {
  GaussianWidget(Gaussian *module) {
        setModule(module);
        box.size = Vec(6*10, 380);

        {
            SVGPanel *panel = new SVGPanel();
            panel->box.size = box.size;
            panel->setBackground(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Gaussian.svg")));
            addChild(panel);
        }

        addParam(createParam<GaussianRoundSmallBlackKnob>(Vec(5, 35), module, Gaussian::MU_PARAM));
        addParam(createParam<GaussianRoundSmallBlackKnob>(Vec(33, 35), module, Gaussian::SIGMA_PARAM));

        addParam(createParam<LEDButton>(Vec(8, 76), module, Gaussian::BUTTON_PARAM));
        addChild(createLight<MediumLight<GreenLight>>(Vec(12.4, 80.4), module, Gaussian::BUTTON_LIGHT));
        addInput(createInput<PJ301MPort>(Vec(32, 75 - 1), module, Gaussian::TRIGGER_INPUT));

        int SPACE = 30;
        int BASE = 106;
        int LEFT = 18;
        addOutput(createOutput<PJ301MPort>(Vec(LEFT, BASE), module, Gaussian::CH_OUTPUT));
        addOutput(createOutput<PJ301MPort>(Vec(LEFT, BASE + SPACE * 1), module, Gaussian::CH_OUTPUT + 1));
        addOutput(createOutput<PJ301MPort>(Vec(LEFT, BASE + SPACE * 2), module, Gaussian::CH_OUTPUT + 2));
        addOutput(createOutput<PJ301MPort>(Vec(LEFT, BASE + SPACE * 3), module, Gaussian::CH_OUTPUT + 3));
        addOutput(createOutput<PJ301MPort>(Vec(LEFT, BASE + SPACE * 4), module, Gaussian::CH_OUTPUT + 4));
        addOutput(createOutput<PJ301MPort>(Vec(LEFT, BASE + SPACE * 5), module, Gaussian::CH_OUTPUT + 5));
        addOutput(createOutput<PJ301MPort>(Vec(LEFT, BASE + SPACE * 6), module, Gaussian::CH_OUTPUT + 6));
        addOutput(createOutput<PJ301MPort>(Vec(LEFT, BASE + SPACE * 7), module, Gaussian::CH_OUTPUT + 7));
        addOutput(createOutput<PJ301MPort>(Vec(LEFT, BASE + SPACE * 8), module, Gaussian::CH_OUTPUT + 8));

        BASE = BASE + 8;
        int RIGHT = 46;
        LEFT = 5;
        addChild(createLight<MediumLight<WhiteLight>>(Vec(RIGHT, BASE            ), module, Gaussian::CH_LIGHT + 0));
        addChild(createLight<MediumLight<WhiteLight>>(Vec(RIGHT,  BASE + SPACE * 1), module, Gaussian::CH_LIGHT + 1));
        addChild(createLight<MediumLight<WhiteLight>>(Vec(RIGHT, BASE + SPACE * 2), module, Gaussian::CH_LIGHT + 2));
        addChild(createLight<MediumLight<WhiteLight>>(Vec(RIGHT,  BASE + SPACE * 3), module, Gaussian::CH_LIGHT + 3));
        addChild(createLight<MediumLight<WhiteLight>>(Vec(RIGHT, BASE + SPACE * 4), module, Gaussian::CH_LIGHT + 4));
        addChild(createLight<MediumLight<WhiteLight>>(Vec(RIGHT,  BASE + SPACE * 5), module, Gaussian::CH_LIGHT + 5));
        addChild(createLight<MediumLight<WhiteLight>>(Vec(RIGHT, BASE + SPACE * 6), module, Gaussian::CH_LIGHT + 6));
        addChild(createLight<MediumLight<WhiteLight>>(Vec(RIGHT,  BASE + SPACE * 7), module, Gaussian::CH_LIGHT + 7));
        addChild(createLight<MediumLight<WhiteLight>>(Vec(RIGHT,  BASE + SPACE * 8), module, Gaussian::CH_LIGHT + 8));

        addChild(createLight<MediumLight<RedLight>>(Vec(LEFT, BASE            ), module, Gaussian::CH_LIGHT + 9));
        addChild(createLight<MediumLight<RedLight>>(Vec(LEFT,  BASE + SPACE * 1), module, Gaussian::CH_LIGHT + 10));
        addChild(createLight<MediumLight<RedLight>>(Vec(LEFT, BASE + SPACE * 2), module, Gaussian::CH_LIGHT + 11));
        addChild(createLight<MediumLight<RedLight>>(Vec(LEFT,  BASE + SPACE * 3), module, Gaussian::CH_LIGHT + 12));
        addChild(createLight<MediumLight<RedLight>>(Vec(LEFT, BASE + SPACE * 4), module, Gaussian::CH_LIGHT + 13));
        addChild(createLight<MediumLight<RedLight>>(Vec(LEFT,  BASE + SPACE * 5), module, Gaussian::CH_LIGHT + 14));
        addChild(createLight<MediumLight<RedLight>>(Vec(LEFT, BASE + SPACE * 6), module, Gaussian::CH_LIGHT + 15));
        addChild(createLight<MediumLight<RedLight>>(Vec(LEFT,  BASE + SPACE * 7), module, Gaussian::CH_LIGHT + 16));
        addChild(createLight<MediumLight<RedLight>>(Vec(LEFT,  BASE + SPACE * 8), module, Gaussian::CH_LIGHT + 17));

    }

    json_t *toJson() {
        json_t *rootJ = ModuleWidget::toJson();
        Gaussian *module = dynamic_cast<Gaussian *>(this->module);
        json_object_set_new(rootJ, "wave", json_real(module->wave_mode_index));
        return rootJ;
    }

    void fromJson(json_t *rootJ) {
        ModuleWidget::fromJson(rootJ);
        json_t *waveJ = json_object_get(rootJ, "wave");
        Gaussian *module = dynamic_cast<Gaussian *>(this->module);
        if (waveJ)
            module->wave_mode_index = json_number_value(waveJ);
    }

};

Model *modelGaussian = createModel<Gaussian, GaussianWidget>("Gaussian");
