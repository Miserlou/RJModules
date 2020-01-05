/*
Slackback!
*/

#include "RJModules.hpp"
#include "common.hpp"
#include "Chorus.h"

#include <iostream>
#include <cmath>
#include <sstream>
#include <iomanip>
#include <unistd.h>
#include <mutex>

using namespace std;
#define HISTORY_SIZE (1<<21)

struct RJChorusRoundSmallBlackKnob : RoundSmallBlackKnob
{
    RJChorusRoundSmallBlackKnob()
    {
        setSVG(SVG::load(assetPlugin(pluginInstance, "res/KTFRoundSmallBlackKnob.svg")));
    }
};

struct RJChorus : Module {
    enum ParamIds {
        DELAY_PARAM,
        FREQ_PARAM,
        DEPTH_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        IN_INPUT,
        DELAY_CV,
        FREQ_CV,
        DEPTH_CV,
        NUM_INPUTS
    };
    enum OutputIds {
        OUT_OUTPUT,
        OUT_OUTPUT_2,
        NUM_OUTPUTS
    };
    enum LightIds {
        NUM_LIGHTS
    };

    int lastDelay = 50;
    stk::Chorus   chorus;

    RJChorus() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

        configParam(RJChorus::DELAY_PARAM, 1, 6000, 50, "Delay Time ms");
        configParam(RJChorus::FREQ_PARAM, 0.0, 25.0, 2.0, "Frequency");
        configParam(RJChorus::DEPTH_PARAM, 0.00001, 0.99999, 0.99999, "Depth");
        chorus = stk::Chorus(50);

    }

    void process(const ProcessArgs &args) override {

        float input = inputs[IN_INPUT].value;
        int delay = params[DELAY_PARAM].value;

        if(delay != lastDelay){
            chorus = stk::Chorus(delay);
            lastDelay = delay;
        }

        chorus.setModFrequency( params[FREQ_PARAM].value );
        chorus.setModDepth( params[DEPTH_PARAM].value );
        float processed = chorus.tick( input );
        outputs[OUT_OUTPUT].value = processed;

    }
};

struct RJChorusWidget : ModuleWidget {
    RJChorusWidget(RJChorus *module) {
		setModule(module);
        setPanel(SVG::load(assetPlugin(pluginInstance, "res/Chorus.svg")));

        int ONE = -4;
        addParam(createParam<RJChorusRoundSmallBlackKnob>(mm2px(Vec(3.5, 38.9593 + ONE)), module, RJChorus::DELAY_PARAM));
        addInput(createPort<PJ301MPort>(mm2px(Vec(3.51398, 48.74977 + ONE)), PortWidget::INPUT, module, RJChorus::DELAY_CV));
        addParam(createParam<RJChorusRoundSmallBlackKnob>(mm2px(Vec(3.51398, 62.3 + ONE)), module, RJChorus::FREQ_PARAM));
        addInput(createPort<PJ301MPort>(mm2px(Vec(3.51398, 73.3 + ONE)), PortWidget::INPUT, module, RJChorus::FREQ_CV));

        int TWO = 45;
        addParam(createParam<RJChorusRoundSmallBlackKnob>(mm2px(Vec(3.5, 38.9593 + TWO)), module, RJChorus::DEPTH_PARAM));
        addInput(createPort<PJ301MPort>(mm2px(Vec(3.51398, 48.74977 + TWO)), PortWidget::INPUT, module, RJChorus::DEPTH_CV));
        addInput(createPort<PJ301MPort>(mm2px(Vec(3.51398, 62.3 + TWO)), PortWidget::INPUT, module, RJChorus::IN_INPUT));
        addOutput(createPort<PJ301MPort>(mm2px(Vec(3.51398, 73.3 + TWO)), PortWidget::OUTPUT, module, RJChorus::OUT_OUTPUT));


    }
};


Model *modelRJChorus = createModel<RJChorus, RJChorusWidget>("RJChorus");
