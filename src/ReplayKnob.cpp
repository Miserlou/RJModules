#include "RJModules.hpp"

#include "dsp/samplerate.hpp"
#include "dsp/ringbuffer.hpp"
#include "dsp/filter.hpp"
#include "dsp/digital.hpp"

#include <iostream>
#include <cmath>
#include <sstream>
#include <iomanip>
#include <vector>

#define HISTORY_SIZE (1<<21)

struct ReplayKnob : Module {
    enum ParamIds {
        BIG_PARAM,
        REC_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        REC_CV_INPUT,
        NUM_INPUTS
    };
    enum OutputIds {
        OUT_OUTPUT,
        NUM_OUTPUTS
    };
    enum LightIds {
        REC_LIGHT,
        NUM_LIGHTS
    };

    SchmittTrigger recTrigger;
    DoubleRingBuffer<float, HISTORY_SIZE> historyBuffer;
    SampleRateConverter<1> src;

    std::vector<float> replayVector;
    int tapeHead = 0;

    bool isRecording = false;
    bool hasRecorded = false;

    ReplayKnob() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {}

    void step() override;
};

struct LilLEDButton : SVGSwitch, MomentarySwitch {
        LilLEDButton() {
                addFrame(SVG::load(assetPlugin(plugin, "res/LilLEDButton.svg")));
        }
};


void ReplayKnob::step() {

    float param = params[BIG_PARAM].value;

    // If we've never recorded anything, or we're recording right now, pass the knob value through.
    outputs[OUT_OUTPUT].value = param;

    // Flip the recording state
    if (recTrigger.process(params[REC_PARAM].value)){

        fprintf( stderr, "I'm recording!" );

        // Clear vector
        if(!isRecording and hasRecorded){
            replayVector.clear();
        }

        isRecording = !isRecording;

        if(!hasRecorded and !isRecording){
            hasRecorded = true;
        }

    }

    if(isRecording){
        replayVector.push_back(param);
        fprintf( stderr, "I'm recording!" );
    }
    else if (hasRecorded){
        fprintf( stderr, "I've recorded!" );

        if (tapeHead >= replayVector.size()){
            tapeHead = 0;
        }
        outputs[OUT_OUTPUT].value = replayVector.at(tapeHead);
        tapeHead++;

    }

    // Lights
    if(isRecording){
        lights[REC_LIGHT].value = 10.0;
    } else{
        lights[REC_LIGHT].value = -10.0;
    }

}


struct ReplayKnobWidget: ModuleWidget {
    ReplayKnobWidget(ReplayKnob *module);
};

ReplayKnobWidget::ReplayKnobWidget(ReplayKnob *module) : ModuleWidget(module) {
    box.size = Vec(15*10, 380);

    {
        SVGPanel *panel = new SVGPanel();
        panel->box.size = box.size;
        panel->setBackground(SVG::load(assetPlugin(plugin, "res/ReplayKnob.svg")));
        addChild(panel);
    }

    addChild(Widget::create<ScrewSilver>(Vec(15, 0)));
    addChild(Widget::create<ScrewSilver>(Vec(box.size.x-30, 0)));
    addChild(Widget::create<ScrewSilver>(Vec(15, 365)));
    addChild(Widget::create<ScrewSilver>(Vec(box.size.x-30, 365)));

    addParam(ParamWidget::create<RoundHugeBlackKnob>(Vec(47, 61), module, ReplayKnob::BIG_PARAM, -12.0, 12.0, 0.0));
    addInput(Port::create<PJ301MPort>(Vec(17, 60), Port::INPUT, module, ReplayKnob::REC_CV_INPUT));
    addParam(ParamWidget::create<LilLEDButton>(Vec(18, 110), module, ReplayKnob::REC_PARAM, 0.0, 1.0, 0.0));
    addChild(ModuleLightWidget::create<MediumLight<GreenLight>>(Vec(22.4, 114.4), module, ReplayKnob::REC_LIGHT));

    // addInput(Port::create<PJ301MPort>(Vec(22, 65), Port::INPUT, module, ReplayKnob::TIME_INPUT));
    addOutput(Port::create<PJ301MPort>(Vec(110, 136), Port::OUTPUT, module, ReplayKnob::OUT_OUTPUT));
}
Model *modelReplayKnob = Model::create<ReplayKnob, ReplayKnobWidget>("RJModules", "ReplayKnob", "[LIVE] ReplayKnob", DELAY_TAG);
