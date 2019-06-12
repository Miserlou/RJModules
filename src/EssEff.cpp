#include "RJModules.hpp"
#include "dsp/digital.hpp"

#include "common.hpp"
#include <iostream>
#include <cmath>
#include <sstream>
#include <iomanip>

#define TSF_IMPLEMENTATION
#include "tsf.h"

using namespace std;

/*
Display
*/

static tsf* tee_ess_eff = tsf_load_filename("soundfonts/Wii_Grand_Piano.sf2");

/*
Widget
*/

struct EssEff : Module {
    enum ParamIds {
        OFFSET_PARAM,
        INVERT_PARAM,
        FREQ_PARAM,
        FM1_PARAM,
        FM2_PARAM,
        PW_PARAM,
        PWM_PARAM,
        NUM_PARAMS,
        CH1_PARAM,
        CH2_PARAM,
    };
    enum InputIds {
        VOCT_INPUT,
        GATE_INPUT,
        NUM_INPUTS
    };
    enum OutputIds {
        MAIN_OUTPUT,
        NUM_OUTPUTS
    };
    enum LightIds {
        PHASE_POS_LIGHT,
        PHASE_NEG_LIGHT,
        NUM_LIGHTS
    };

    bool output_set = false;
    float frame[1000000];
    int head = -1;

    EssEff() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {}
    void step() override;

};

void EssEff::step() {

    if(!output_set){

        tsf_set_output(tee_ess_eff, TSF_MONO, 44100, 0.0); //sample rate
        output_set = true;

    } else{

        if (inputs[GATE_INPUT].value >= 1.0f) {


            int note;
            if (inputs[VOCT_INPUT].active){
                note = (int) std::round(inputs[VOCT_INPUT].value * 12.f + 60.f);
                note = clamp(note, 0, 127);
            } else{
                note = 60;
            }
            tsf_note_on(tee_ess_eff, 0, note, 1.0f);
            head = -1;
        }

        if ( head < 0 || head>=1000000 ){
            head = 0;
            tsf_render_float(tee_ess_eff, frame, 1000000, 0);
        } else{
            head++;
        }

        outputs[MAIN_OUTPUT].value  = frame[head];
    }
}


struct EssEffWidget: ModuleWidget {
    EssEffWidget(EssEff *module);
};

EssEffWidget::EssEffWidget(EssEff *module) : ModuleWidget(module) {
    box.size = Vec(15*10, 380);

    {
        SVGPanel *panel = new SVGPanel();
        panel->box.size = box.size;
        panel->setBackground(SVG::load(assetPlugin(plugin, "res/RangeLFO.svg")));
        addChild(panel);
    }

    addChild(Widget::create<ScrewSilver>(Vec(15, 0)));
    addChild(Widget::create<ScrewSilver>(Vec(box.size.x-30, 0)));
    addChild(Widget::create<ScrewSilver>(Vec(15, 365)));
    addChild(Widget::create<ScrewSilver>(Vec(box.size.x-30, 365)));

    // Inputs and Knobs
    addInput(Port::create<PJ301MPort>(Vec(11, 100), Port::INPUT, module, EssEff::VOCT_INPUT));
    addInput(Port::create<PJ301MPort>(Vec(11, 200), Port::INPUT, module, EssEff::GATE_INPUT));

    addOutput(Port::create<PJ301MPort>(Vec(11, 320), Port::OUTPUT, module, EssEff::MAIN_OUTPUT));
}

Model *modelEssEff = Model::create<EssEff, EssEffWidget>("RJModules", "EssEff", "[GEN] EssEff", LFO_TAG);
