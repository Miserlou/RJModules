#include "RJModules.hpp"
#include <sndfile.h>

#define NUM_CHANNELS 1

struct SnapKnob : RoundSmallBlackKnob
{
    SnapKnob()
    {
        minAngle = -0.83 * M_PI;
        maxAngle = 0.83 * M_PI;
        snap = true;
    }
};

struct Soundpipe : Module {
    enum ParamIds {
        MUTE_PARAM,
        NUM_PARAMS = MUTE_PARAM + NUM_CHANNELS
    };
    enum InputIds {
        IN_INPUT,
        NUM_INPUTS = IN_INPUT + NUM_CHANNELS
    };
    enum OutputIds {
        OUT_OUTPUT,
        NUM_OUTPUTS = OUT_OUTPUT + NUM_CHANNELS
    };
    enum LightIds {
        MUTE_LIGHT,
        NUM_LIGHTS = MUTE_LIGHT + NUM_CHANNELS
    };

    bool state[NUM_CHANNELS];
    SchmittTrigger muteTrigger[NUM_CHANNELS];

    Soundpipe() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

    }
    void step() override;

    json_t *dataToJson() override {
        json_t *rootJ = json_object();
        // states
        json_t *statesJ = json_array();
        for (int i = 0; i < NUM_CHANNELS; i++) {
            json_t *stateJ = json_boolean(state[i]);
            json_array_append_new(statesJ, stateJ);
        }
        json_object_set_new(rootJ, "states", statesJ);
        return rootJ;
    }
    void dataFromJson(json_t *rootJ) override {
        // states
        json_t *statesJ = json_object_get(rootJ, "states");
        if (statesJ) {
            for (int i = 0; i < NUM_CHANNELS; i++) {
                json_t *stateJ = json_array_get(statesJ, i);
                if (stateJ)
                    state[i] = json_boolean_value(stateJ);
            }
        }
    }
};

void Soundpipe::step() {
    for (int i = 0; i < NUM_CHANNELS; i++) {
        float in = inputs[IN_INPUT + i].value;
        outputs[OUT_OUTPUT + i].value = in + round(params[MUTE_PARAM + i].value);
    }
}


template <typename BASE>
struct MuteLight : BASE {
    MuteLight() {
        this->box.size = mm2px(Vec(6.0, 6.0));
    }
};

struct SoundpipeWidget: ModuleWidget {
    SoundpipeWidget(Soundpipe *module);
};

SoundpipeWidget::SoundpipeWidget(Soundpipe *module) {
        setModule(module);
    setPanel(SVG::load(assetPlugin(pluginInstance, "res/Soundpipe.svg")));

    addChild(createWidget<ScrewSilver>(Vec(15, 0)));
    addChild(createWidget<ScrewSilver>(Vec(box.size.x - 30, 0)));
    addChild(createWidget<ScrewSilver>(Vec(15, 365)));
    addChild(createWidget<ScrewSilver>(Vec(box.size.x - 30, 365)));

    addParam(createParam<SnapKnob>(mm2px(Vec(16.57, 17.165)), module, Soundpipe::MUTE_PARAM + 0));
    addInput(createPort<PJ301MPort>(mm2px(Vec(4.214, 17.81)), PortWidget::INPUT, module, Soundpipe::IN_INPUT + 0));
    addOutput(createPort<PJ301MPort>(mm2px(Vec(28.214, 17.81)), PortWidget::OUTPUT, module, Soundpipe::OUT_OUTPUT + 0));

}

Model *modelSoundpipe = createModel<Soundpipe, SoundpipeWidget>("Soundpipe");
