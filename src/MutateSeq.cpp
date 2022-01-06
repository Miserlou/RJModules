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

struct MutateSnapKnobLg : RoundBlackKnob
{
    MutateSnapKnobLg()
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
        OCT_DEPTH,
        NOTE_DEPTH,
        MUTATE_EVERY,
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
    dsp::SchmittTrigger clockTrigger;

    bool init = false;
    float seq_notes[8] = {-99,-99,-99,-99,-99,-99,-99,-99};
    float seq_octaves[8] = {-99,-99,-99,-99,-99,-99,-99,-99};
    float last_notes[8] = {-99,-99,-99,-99,-99,-99,-99,-99};
    float last_octaves[8] = {-99,-99,-99,-99,-99,-99,-99,-99};

    int mut_counter = 0;

    // Static
    float notes[12] = {0,   0.08, 0.17, 0.25, 0.33, 0.42,
                     0.5, 0.58, 0.67, 0.75, 0.83, 0.92};
    int octaves[7] = {-2, -1, 0, 1, 2, 3, 4};

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

        configParam(MutateSeq::OCT_PARAM + 0, 0.0, 6.0, 1.0, string::f("Ch %d octave", 0));
        configParam(MutateSeq::OCT_PARAM + 1, 0.0, 6.0, 1.0, string::f("Ch %d octave", 1));
        configParam(MutateSeq::OCT_PARAM + 2, 0.0, 6.0, 1.0, string::f("Ch %d octave", 2));
        configParam(MutateSeq::OCT_PARAM + 3, 0.0, 6.0, 1.0, string::f("Ch %d octave", 3));
        configParam(MutateSeq::OCT_PARAM + 4, 0.0, 6.0, 1.0, string::f("Ch %d octave", 4));
        configParam(MutateSeq::OCT_PARAM + 5, 0.0, 6.0, 1.0, string::f("Ch %d octave", 5));
        configParam(MutateSeq::OCT_PARAM + 6, 0.0, 6.0, 1.0, string::f("Ch %d octave", 6));
        configParam(MutateSeq::OCT_PARAM + 7, 0.0, 6.0, 1.0, string::f("Ch %d octave", 7));

        configParam(MutateSeq::SEMI_PARAM + 0, 0.0, 11.0, 0.0, string::f("Ch %d semi", 0));
        configParam(MutateSeq::SEMI_PARAM + 1, 0.0, 11.0, 0.0, string::f("Ch %d semi", 1));
        configParam(MutateSeq::SEMI_PARAM + 2, 0.0, 11.0, 0.0, string::f("Ch %d semi", 2));
        configParam(MutateSeq::SEMI_PARAM + 3, 0.0, 11.0, 0.0, string::f("Ch %d semi", 3));
        configParam(MutateSeq::SEMI_PARAM + 4, 0.0, 11.0, 0.0, string::f("Ch %d semi", 4));
        configParam(MutateSeq::SEMI_PARAM + 5, 0.0, 11.0, 0.0, string::f("Ch %d semi", 5));
        configParam(MutateSeq::SEMI_PARAM + 6, 0.0, 11.0, 0.0, string::f("Ch %d semi", 6));
        configParam(MutateSeq::SEMI_PARAM + 7, 0.0, 11.0, 0.0, string::f("Ch %d semi", 7));

        configParam(MutateSeq::MUTATE_EVERY, 1.0, 128.0, 16.0, "Mutate Every");
        configParam(MutateSeq::OCT_DEPTH, 0.0, 6.0, 1.0, "Oct depth");
        configParam(MutateSeq::NOTE_DEPTH, 0.0, 11.0, 11.0, "Note depth");

        configParam(MutateSeq::STEPS_PARAM, 1.0f, 8.0f, 8.0f, "Length");
    }

    void setIndex(int index) {
        int numSteps = (int) clamp(roundf(params[STEPS_PARAM].value), 1.0f, 8.0f);
        phase = 0.f;
        this->index = index;
        if (this->index >= numSteps)
            this->index = 0;
    }

    // void step() override;
    // json_t *dataToJson() override {};
    // void dataFromJson(json_t *rootJ) override {};

    void process(const ProcessArgs &args) override {

        // Reset on first process
        if(!init){
            for (int i = 0; i < 8; i++) {
                seq_octaves[i] = params[OCT_PARAM + i].value;
                seq_notes[i] = params[SEMI_PARAM + i].value;

                last_octaves[i] = params[OCT_PARAM + i].value;
                last_notes[i] = params[SEMI_PARAM + i].value;

                lock_state[i] = false;
            }
            init = true;
        }

        bool gateIn = false;
        if (inputs[IN_INPUT].active) {
            if (clockTrigger.process(inputs[IN_INPUT].value)) {
                setIndex(index + 1);

                // MUTATION
                mut_counter++;
                if (mut_counter >= params[MUTATE_EVERY].value){
                    mut_counter = 0;
                    int choice = rand() % (int) params[STEPS_PARAM].value;
                    if(!lock_state[choice]){

                        bool mutate_oct = rand() & 1;
                        if(mutate_oct){
                            if(rand() & 1){
                                int plus_distance = (int) rand() & (int) params[OCT_DEPTH].value + 1;
                                seq_octaves[choice] = seq_octaves[choice] + plus_distance;
                            }else{
                                int minus_distance = (int) rand() & (int) params[OCT_DEPTH].value + 1;
                                seq_octaves[choice] = seq_octaves[choice] - minus_distance;
                            }

                            if(seq_octaves[choice] < 0)
                                seq_octaves[choice] = 0;
                            if(seq_octaves[choice] > 6)
                                seq_octaves[choice] = 6;
                        }

                        bool mutate_semi = rand() & 1;
                        if(mutate_semi){
                            if(rand() & 1){
                                int plus_distance = (int) rand() & (int) params[NOTE_DEPTH].value + 1;
                                seq_notes[choice] = seq_notes[choice] + plus_distance;
                            }else{
                                int minus_distance = (int) rand() & (int) params[NOTE_DEPTH].value + 1;
                                seq_notes[choice] = seq_notes[choice] - minus_distance;
                            }

                            if(seq_notes[choice] < 0)
                                seq_notes[choice] = 0;
                            if(seq_notes[choice] > 11)
                                seq_notes[choice] = 11;
                        }

                    }
                }
            }
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

        // If knobs have been nudged since last seen, use those.
        if(params[OCT_PARAM + index].value != last_octaves[index]){
            seq_octaves[index] = params[OCT_PARAM + index].value;
        }
        last_octaves[index] = params[OCT_PARAM + index].value;
        if(params[SEMI_PARAM + index].value != last_notes[index]){
            seq_notes[index] = params[SEMI_PARAM + index].value;
        }
        last_notes[index] = params[SEMI_PARAM + index].value;

        float oct_param = seq_octaves[index];
        float note_param = seq_notes[index];
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
    setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/MutateSeq.svg")));

    // MAGNETS
    // addChild(createWidget<ScrewSilver>(Vec(15, 0)));
    // addChild(createWidget<ScrewSilver>(Vec(box.size.x - 30, 0)));
    // addChild(createWidget<ScrewSilver>(Vec(15, 365)));
    // addChild(createWidget<ScrewSilver>(Vec(box.size.x - 30, 365)));

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
    addParam(createParam<MutateSnapKnobLg>(mm2px(Vec(4.0, 102.0)), module, MutateSeq::MUTATE_EVERY));
    addParam(createParam<MutateSnapKnobLg>(mm2px(Vec(16.0, 102.0)), module, MutateSeq::OCT_DEPTH));
    addParam(createParam<MutateSnapKnobLg>(mm2px(Vec(28.0, 102.0)), module, MutateSeq::NOTE_DEPTH));

    // Ins/Outs
    addInput(createInput<PJ301MPort>(mm2px(Vec(4.214, 117.809)), module, MutateSeq::IN_INPUT));
    addParam(createParam<MutateSnapKnob>(mm2px(Vec(16.57, 117.809)), module, MutateSeq::STEPS_PARAM));
    addOutput(createOutput<PJ301MPort>(mm2px(Vec(28.214, 117.809)), module, MutateSeq::OUT_OUTPUT));


}

Model *modelMutateSeq = createModel<MutateSeq, MutateSeqWidget>("MutateSeq");
