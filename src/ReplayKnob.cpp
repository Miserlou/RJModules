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
        REPLAY_PARAM,
        START_PARAM,
        END_PARAM,

        BIG_PARAM_2,
        REC_PARAM_2,
        REPLAY_PARAM_2,
        START_PARAM_2,
        END_PARAM_2,

        NUM_PARAMS
    };
    enum InputIds {
        BIG_CV_INPUT,
        REC_CV_INPUT,
        REPLAY_CV_INPUT,

        BIG_CV_INPUT_2,
        REC_CV_INPUT_2,
        REPLAY_CV_INPUT_2,

        NUM_INPUTS
    };
    enum OutputIds {
        OUT_OUTPUT,
        OUT_OUTPUT_2,
        NUM_OUTPUTS
    };
    enum LightIds {
        REC_LIGHT,
        REPLAY_LIGHT,
        REC_LIGHT_2,
        REPLAY_LIGHT_2,
        NUM_LIGHTS
    };

    // Knob One
    SchmittTrigger recTrigger;
    SchmittTrigger recTriggerCV;
    SchmittTrigger replayTrigger;
    SchmittTrigger replayTriggerCV;

    std::vector<float> replayVector;
    float param;
    int tapeHead = 0;

    bool isRecording = false;
    bool hasRecorded = false;
    float replayLight = 0.0;

    // Knob Two
    SchmittTrigger recTrigger_2;
    SchmittTrigger recTriggerCV_2;
    SchmittTrigger replayTrigger_2;
    SchmittTrigger replayTriggerCV_2;

    std::vector<float> replayVector_2;
    float param_2;
    int tapeHead_2 = 0;

    bool isRecording_2 = false;
    bool hasRecorded_2 = false;
    float replayLight_2 = 0.0;

    const float lightLambda = 0.075;

    ReplayKnob() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);}

    void step() override;
};

struct LilLEDButton : SVGSwitch, MomentarySwitch {
        LilLEDButton() {
                addFrame(SVG::load(assetPlugin(pluginInstance, "res/LilLEDButton.svg")));
        }
};


void ReplayKnob::step() {

    /*
    *
    * Knob One
    *
    */

    // If we've never recorded anything, or we're recording right now, pass the knob value through.
    if (inputs[BIG_CV_INPUT].active){
        param = inputs[BIG_CV_INPUT].value;
    } else {
        param = params[BIG_PARAM].value;
    }
    outputs[OUT_OUTPUT].value = param;

    // Flip the recording state
    if (recTrigger.process(params[REC_PARAM].value) || recTriggerCV.process(inputs[REC_CV_INPUT].value)){

        // Clear vector
        if(!isRecording and hasRecorded){
            replayVector.clear();
            tapeHead = 0;
        }

        isRecording = !isRecording;

        if(!hasRecorded and !isRecording){
            hasRecorded = true;
        }
    }

    if(isRecording){
        replayVector.push_back(param);
    }
    else if (hasRecorded){
        // Get start and end values
        float startParam = params[START_PARAM].value;
        float endParam = params[END_PARAM].value;
        float vectorSize = replayVector.size();
        int startPos = (int) (startParam * vectorSize);
        int endPos = (int) (endParam * vectorSize);
        if(startPos >= endPos){
            startPos = endPos;
        }

        // Are we replaying?
        if (replayTrigger.process(params[REPLAY_PARAM].value) || replayTriggerCV.process(inputs[REPLAY_CV_INPUT].value)){
            tapeHead = startPos;
            replayLight = 1.0;
        }
        replayLight -= replayLight / lightLambda / engineGetSampleRate();

        // Loop around
        if (tapeHead >= endPos || tapeHead >= vectorSize){
            tapeHead = startPos;
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
    lights[REPLAY_LIGHT].value = replayLight;

    /*
    *
    * Knob Two
    *
    */

    // If we've never recorded anything, or we're recording right now, pass the knob value through.
    if (inputs[BIG_CV_INPUT_2].active){
        param_2 = inputs[BIG_CV_INPUT_2].value;
    } else {
        param_2 = params[BIG_PARAM_2].value;
    }
    outputs[OUT_OUTPUT_2].value = param_2;

    // Flip the recording state
    if (recTrigger_2.process(params[REC_PARAM_2].value) || recTriggerCV_2.process(inputs[REC_CV_INPUT_2].value)){

        // Clear vector
        if(!isRecording_2 and hasRecorded_2){
            replayVector_2.clear();
            tapeHead_2 = 0;
        }

        isRecording_2 = !isRecording_2;

        if(!hasRecorded_2 and !isRecording_2){
            hasRecorded_2 = true;
        }
    }

    if(isRecording_2){
        replayVector_2.push_back(param_2);
    }
    else if (hasRecorded_2){
        // Get start and end values
        float startParam_2 = params[START_PARAM_2].value;
        float endParam_2 = params[END_PARAM_2].value;
        float vectorSize_2 = replayVector_2.size();
        int startPos_2 = (int) (startParam_2 * vectorSize_2);
        int endPos_2 = (int) (endParam_2 * vectorSize_2);
        if(startPos_2 >= endPos_2){
            startPos_2 = endPos_2;
        }

        // Are we replaying?
        if (replayTrigger_2.process(params[REPLAY_PARAM_2].value) || replayTriggerCV_2.process(inputs[REPLAY_CV_INPUT_2].value)){
            tapeHead_2 = startPos_2;
            replayLight_2 = 1.0;
        }
        replayLight_2 -= replayLight_2 / lightLambda / engineGetSampleRate();

        // Loop around
        if (tapeHead_2 >= endPos_2 || tapeHead_2 >= vectorSize_2){
            tapeHead_2 = startPos_2;
        }
        outputs[OUT_OUTPUT_2].value = replayVector_2.at(tapeHead_2);
        tapeHead_2++;
    }

    // Lights
    if(isRecording_2){
        lights[REC_LIGHT_2].value = 10.0;
    } else{
        lights[REC_LIGHT_2].value = -10.0;
    }
    lights[REPLAY_LIGHT_2].value = replayLight_2;

}


struct ReplayKnobWidget: ModuleWidget {
    ReplayKnobWidget(ReplayKnob *module);
};

ReplayKnobWidget::ReplayKnobWidget(ReplayKnob *module) {
		setModule(module);

    float buttonx = 20;
    float buttony = 114;
    float offset = 160;

    box.size = Vec(15*10, 380);

    {
        SVGPanel *panel = new SVGPanel();
        panel->box.size = box.size;
        panel->setBackground(SVG::load(assetPlugin(pluginInstance, "res/ReplayKnob.svg")));
        addChild(panel);
    }

    addChild(createWidget<ScrewSilver>(Vec(15, 0)));
    addChild(createWidget<ScrewSilver>(Vec(box.size.x-30, 0)));
    addChild(createWidget<ScrewSilver>(Vec(15, 365)));
    addChild(createWidget<ScrewSilver>(Vec(box.size.x-30, 365)));

    // Knob One
    addParam(createParam<RoundHugeBlackKnob>(Vec(47, 61), module, ReplayKnob::BIG_PARAM, -5.0, 5.0, 0.0));
    addInput(createPort<PJ301MPort>(Vec(17, 50), PortWidget::INPUT, module, ReplayKnob::BIG_CV_INPUT));
    addInput(createPort<PJ301MPort>(Vec(17, 80), PortWidget::INPUT, module, ReplayKnob::REC_CV_INPUT));

    addParam(createParam<LilLEDButton>(Vec(buttonx, buttony), module, ReplayKnob::REC_PARAM, 0.0, 1.0, 0.0));
    addChild(createLight<MediumLight<RedLight>>(Vec(buttonx+4.4, buttony+4.4), module, ReplayKnob::REC_LIGHT));

    addInput(createPort<PJ301MPort>(Vec(110, 50), PortWidget::INPUT, module, ReplayKnob::REPLAY_CV_INPUT));
    addParam(createParam<LilLEDButton>(Vec(114, 85), module, ReplayKnob::REPLAY_PARAM, 0.0, 1.0, 0.0));
    addChild(createLight<MediumLight<GreenLight>>(Vec(114+4.4, 85+4.4), module, ReplayKnob::REPLAY_LIGHT));

    addParam(createParam<RoundBlackKnob>(Vec(17, 140), module, ReplayKnob::START_PARAM, 0.0, 1.0, 0.0));
    addParam(createParam<RoundBlackKnob>(Vec(58, 140), module, ReplayKnob::END_PARAM, 0.0, 1.0, 1.0));
    addOutput(createPort<PJ301MPort>(Vec(110, 142), PortWidget::OUTPUT, module, ReplayKnob::OUT_OUTPUT));

    // Knob Two
    addParam(createParam<RoundHugeBlackKnob>(Vec(47, 61 + offset), module, ReplayKnob::BIG_PARAM_2, 0.0, 10.0, 5.0));
    addInput(createPort<PJ301MPort>(Vec(17, 50 + offset), PortWidget::INPUT, module, ReplayKnob::BIG_CV_INPUT_2));
    addInput(createPort<PJ301MPort>(Vec(17, 80 + offset), PortWidget::INPUT, module, ReplayKnob::REC_CV_INPUT_2));

    addParam(createParam<LilLEDButton>(Vec(buttonx, buttony + offset), module, ReplayKnob::REC_PARAM_2, 0.0, 1.0, 0.0));
    addChild(createLight<MediumLight<RedLight>>(Vec(buttonx+4.4, buttony+4.4+ offset), module, ReplayKnob::REC_LIGHT_2));

    addInput(createPort<PJ301MPort>(Vec(110, 50 + offset), PortWidget::INPUT, module, ReplayKnob::REPLAY_CV_INPUT_2));
    addParam(createParam<LilLEDButton>(Vec(114, 85 + offset), module, ReplayKnob::REPLAY_PARAM_2, 0.0, 1.0, 0.0));
    addChild(createLight<MediumLight<GreenLight>>(Vec(114+4.4, 85+4.4 + offset), module, ReplayKnob::REPLAY_LIGHT_2));

    addParam(createParam<RoundBlackKnob>(Vec(17, 140 + offset), module, ReplayKnob::START_PARAM_2, 0.0, 1.0, 0.0));
    addParam(createParam<RoundBlackKnob>(Vec(58, 140 + offset), module, ReplayKnob::END_PARAM_2, 0.0, 1.0, 1.0));
    addOutput(createPort<PJ301MPort>(Vec(110, 142 + offset), PortWidget::OUTPUT, module, ReplayKnob::OUT_OUTPUT_2));

}
Model *modelReplayKnob = createModel<ReplayKnob, ReplayKnobWidget>("RJModules", "ReplayKnob", "[LIVE] ReplayKnob", DELAY_TAG);
