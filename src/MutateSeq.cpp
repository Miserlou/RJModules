#include "RJModules.hpp"

#define NUM_CHANNELS 8

struct MutateSnapKnob : RoundSmallBlackKnob
{
    MutateSnapKnob()
    {
        minAngle = -0.83 * M_PI;
        maxAngle = 0.83 * M_PI;
        snap = true;
    }
};

struct MutateSeq : Module {
    enum ParamIds {
        ENUMS(LOCK_PARAM, 8),
        ENUMS(OCT_PARAM, 8),
        ENUMS(SEMI_PARAM, 8),
        STEPS_PARAM,
        NUM_PARAMS
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
        ENUMS(LOCK_LIGHT, 8),
        NUM_LIGHTS
    };

    // Lock
    bool lock_state[8];
    dsp::BooleanTrigger lockTrigger[8];

    // Seq
    int index = 0;
    float phase = 0.f;
    int currentPosition;
    SchmittTrigger clockTrigger;

    float notes[12] = {0,   0.08, 0.17, 0.25, 0.33, 0.42,
                     0.5, 0.58, 0.67, 0.75, 0.83, 0.92};
    int octaves[7] = {-1, 0, 1, 2, 3, 4, 5};

    MutateSeq() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

        configParam(MutateSeq::LOCK_PARAM + 0, 0.0, 1.0, 0.0, string::f("Ch %d lock", 0));
        configParam(MutateSeq::LOCK_PARAM + 1, 0.0, 1.0, 0.0, string::f("Ch %d lock", 1));
        configParam(MutateSeq::LOCK_PARAM + 2, 0.0, 1.0, 0.0, string::f("Ch %d lock", 2));
        configParam(MutateSeq::LOCK_PARAM + 3, 0.0, 1.0, 0.0, string::f("Ch %d lock", 3));
        configParam(MutateSeq::LOCK_PARAM + 4, 0.0, 1.0, 0.0, string::f("Ch %d lock", 4));
        configParam(MutateSeq::LOCK_PARAM + 5, 0.0, 1.0, 0.0, string::f("Ch %d lock", 5));
        configParam(MutateSeq::LOCK_PARAM + 6, 0.0, 1.0, 0.0, string::f("Ch %d lock", 6));
        configParam(MutateSeq::LOCK_PARAM + 7, 0.0, 1.0, 0.0, string::f("Ch %d lock", 7));

        configParam(MutateSeq::OCT_PARAM + 0, 0.0, 7.0, 0.0, string::f("Ch %d octave", 0));
        configParam(MutateSeq::OCT_PARAM + 1, 0.0, 7.0, 0.0, string::f("Ch %d octave", 1));
        configParam(MutateSeq::OCT_PARAM + 2, 0.0, 7.0, 0.0, string::f("Ch %d octave", 2));
        configParam(MutateSeq::OCT_PARAM + 3, 0.0, 7.0, 0.0, string::f("Ch %d octave", 3));
        configParam(MutateSeq::OCT_PARAM + 4, 0.0, 7.0, 0.0, string::f("Ch %d octave", 4));
        configParam(MutateSeq::OCT_PARAM + 5, 0.0, 7.0, 0.0, string::f("Ch %d octave", 5));
        configParam(MutateSeq::OCT_PARAM + 6, 0.0, 7.0, 0.0, string::f("Ch %d octave", 6));
        configParam(MutateSeq::OCT_PARAM + 7, 0.0, 7.0, 0.0, string::f("Ch %d octave", 7));

        configParam(MutateSeq::SEMI_PARAM + 0, 0.0, 12.0, 0.0, string::f("Ch %d semi", 0));
        configParam(MutateSeq::SEMI_PARAM + 1, 0.0, 12.0, 0.0, string::f("Ch %d semi", 1));
        configParam(MutateSeq::SEMI_PARAM + 2, 0.0, 12.0, 0.0, string::f("Ch %d semi", 2));
        configParam(MutateSeq::SEMI_PARAM + 3, 0.0, 12.0, 0.0, string::f("Ch %d semi", 3));
        configParam(MutateSeq::SEMI_PARAM + 4, 0.0, 12.0, 0.0, string::f("Ch %d semi", 4));
        configParam(MutateSeq::SEMI_PARAM + 5, 0.0, 12.0, 0.0, string::f("Ch %d semi", 5));
        configParam(MutateSeq::SEMI_PARAM + 6, 0.0, 12.0, 0.0, string::f("Ch %d semi", 6));
        configParam(MutateSeq::SEMI_PARAM + 7, 0.0, 12.0, 0.0, string::f("Ch %d semi", 7));

        configParam(MutateSeq::STEPS_PARAM, 1.0f, 8.0f, 8.0f, "");
    }

    void setIndex(int index) {
        int numSteps = (int) clamp(roundf(params[STEPS_PARAM].value), 1.0f, 8.0f);
        phase = 0.f;
        this->index = index;
        if (this->index >= numSteps)
            this->index = 0;
    }

    // void step() override;

    json_t *dataToJson() override {
        json_t *rootJ = json_object();
        // states
        json_t *statesJ = json_array();
        for (int i = 0; i < NUM_CHANNELS; i++) {
            json_t *stateJ = json_boolean(lock_state[i]);
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
                    lock_state[i] = json_boolean_value(stateJ);
            }
        }
    }

    void process(const ProcessArgs &args) override {
        const float zero[16] = {};
        float out[16] = {};
        int channels = 1;

        bool gateIn = false;
        if (inputs[IN_INPUT].active) {
            if (clockTrigger.process(inputs[IN_INPUT].value)) {
                setIndex(index + 1);
            }
            gateIn = clockTrigger.isHigh();
        }

        // Iterate rows
        for (int i = 0; i < 8; i++) {
            // Process trigger
            if (lockTrigger[i].process(params[LOCK_PARAM + i].getValue() > 0.f))
                lock_state[i] ^= true;

            // Set light
            lights[LOCK_LIGHT + i].setBrightness(lock_state[i] ? 0.6f : 0.f);
            lights[LOCK_LIGHT + i].setBrightness((index == i) ? 1.0f :lights[LOCK_LIGHT + i].getBrightness());
        }


        float oct_param = params[OCT_PARAM + index].value;
        float note_param = params[SEMI_PARAM + index].value;
        outputs[OUT_OUTPUT].value = octaves[(int)oct_param] + notes[(int)note_param];

    }
};

template <typename BASE>
struct MuteLight : BASE {
    MuteLight() {
        this->box.size = mm2px(Vec(6.0, 6.0));
    }
};

struct MutateSeqWidget: ModuleWidget {
    MutateSeqWidget(MutateSeq *module);
};

MutateSeqWidget::MutateSeqWidget(MutateSeq *module) {
    setModule(module);
    setPanel(SVG::load(assetPlugin(pluginInstance, "res/Octaves.svg")));

    addChild(createWidget<ScrewSilver>(Vec(15, 0)));
    addChild(createWidget<ScrewSilver>(Vec(box.size.x - 30, 0)));
    addChild(createWidget<ScrewSilver>(Vec(15, 365)));
    addChild(createWidget<ScrewSilver>(Vec(box.size.x - 30, 365)));

    addParam(createParam<LEDBezel>(mm2px(Vec(4.214, 17.81)), module, MutateSeq::LOCK_PARAM + 0));
    addParam(createParam<LEDBezel>(mm2px(Vec(4.214, 27.809)), module, MutateSeq::LOCK_PARAM + 1));
    addParam(createParam<LEDBezel>(mm2px(Vec(4.214, 37.809)), module, MutateSeq::LOCK_PARAM + 2));
    addParam(createParam<LEDBezel>(mm2px(Vec(4.214, 47.81)), module, MutateSeq::LOCK_PARAM + 3));
    addParam(createParam<LEDBezel>(mm2px(Vec(4.214, 57.81)), module, MutateSeq::LOCK_PARAM + 4));
    addParam(createParam<LEDBezel>(mm2px(Vec(4.214, 67.809)), module, MutateSeq::LOCK_PARAM + 5));
    addParam(createParam<LEDBezel>(mm2px(Vec(4.214, 77.81)), module, MutateSeq::LOCK_PARAM + 6));
    addParam(createParam<LEDBezel>(mm2px(Vec(4.214, 87.81)), module, MutateSeq::LOCK_PARAM + 7));

    addChild(createLight<MuteLight<GreenLight>>(mm2px(Vec(4.214 + 0.75, 17.81 + 0.75)), module, MutateSeq::LOCK_LIGHT + 0));
    addChild(createLight<MuteLight<GreenLight>>(mm2px(Vec(4.214 + 0.75, 27.809 + 0.75)), module, MutateSeq::LOCK_LIGHT + 1));
    addChild(createLight<MuteLight<GreenLight>>(mm2px(Vec(4.214 + 0.75, 37.809 + 0.75)), module, MutateSeq::LOCK_LIGHT + 2));
    addChild(createLight<MuteLight<GreenLight>>(mm2px(Vec(4.214 + 0.75, 47.81 + 0.75)), module, MutateSeq::LOCK_LIGHT + 3));
    addChild(createLight<MuteLight<GreenLight>>(mm2px(Vec(4.214 + 0.75, 57.81 + 0.75)), module, MutateSeq::LOCK_LIGHT + 4));
    addChild(createLight<MuteLight<GreenLight>>(mm2px(Vec(4.214 + 0.75, 67.809 + 0.75)), module, MutateSeq::LOCK_LIGHT + 5));
    addChild(createLight<MuteLight<GreenLight>>(mm2px(Vec(4.214 + 0.75, 77.81 + 0.75)), module, MutateSeq::LOCK_LIGHT + 6));
    addChild(createLight<MuteLight<GreenLight>>(mm2px(Vec(4.214 + 0.75, 87.81 + 0.75)), module, MutateSeq::LOCK_LIGHT + 7));

    addParam(createParam<MutateSnapKnob>(mm2px(Vec(16.57, 17.165)), module, MutateSeq::OCT_PARAM + 0));
    addParam(createParam<MutateSnapKnob>(mm2px(Vec(16.57, 27.164)), module, MutateSeq::OCT_PARAM + 1));
    addParam(createParam<MutateSnapKnob>(mm2px(Vec(16.57, 37.164)), module, MutateSeq::OCT_PARAM + 2));
    addParam(createParam<MutateSnapKnob>(mm2px(Vec(16.57, 47.165)), module, MutateSeq::OCT_PARAM + 3));
    addParam(createParam<MutateSnapKnob>(mm2px(Vec(16.57, 57.164)), module, MutateSeq::OCT_PARAM + 4));
    addParam(createParam<MutateSnapKnob>(mm2px(Vec(16.57, 67.165)), module, MutateSeq::OCT_PARAM + 5));
    addParam(createParam<MutateSnapKnob>(mm2px(Vec(16.57, 77.164)), module, MutateSeq::OCT_PARAM + 6));
    addParam(createParam<MutateSnapKnob>(mm2px(Vec(16.57, 87.164)), module, MutateSeq::OCT_PARAM + 7));

    addParam(createParam<MutateSnapKnob>(mm2px(Vec(28.214, 17.165)),  module, MutateSeq::SEMI_PARAM + 0));
    addParam(createParam<MutateSnapKnob>(mm2px(Vec(28.214, 27.164)),  module, MutateSeq::SEMI_PARAM + 1));
    addParam(createParam<MutateSnapKnob>(mm2px(Vec(28.214, 37.164)),  module, MutateSeq::SEMI_PARAM + 2));
    addParam(createParam<MutateSnapKnob>(mm2px(Vec(28.214, 47.165)),  module, MutateSeq::SEMI_PARAM + 3));
    addParam(createParam<MutateSnapKnob>(mm2px(Vec(28.214, 57.164)),  module, MutateSeq::SEMI_PARAM + 4));
    addParam(createParam<MutateSnapKnob>(mm2px(Vec(28.214, 67.165)),  module, MutateSeq::SEMI_PARAM + 5));
    addParam(createParam<MutateSnapKnob>(mm2px(Vec(28.214, 77.164)),  module, MutateSeq::SEMI_PARAM + 6));
    addParam(createParam<MutateSnapKnob>(mm2px(Vec(28.214, 87.164)),  module, MutateSeq::SEMI_PARAM + 7));

    // Mutate Params

    // Ins/Outs
    addInput(createPort<PJ301MPort>(mm2px(Vec(4.214, 117.809)), PortWidget::INPUT, module, MutateSeq::IN_INPUT));
    addParam(createParam<MutateSnapKnob>(mm2px(Vec(16.57, 117.809)), module, MutateSeq::STEPS_PARAM));
    addOutput(createPort<PJ301MPort>(mm2px(Vec(28.214, 117.809)), PortWidget::OUTPUT, module, MutateSeq::OUT_OUTPUT));


}

Model *modelMutateSeq = createModel<MutateSeq, MutateSeqWidget>("MutateSeq");
