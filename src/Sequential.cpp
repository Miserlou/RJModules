#include "RJModules.hpp"

#define NUM_CHANNELS 8

struct SequentialSnapKnob : RoundSmallBlackKnob
{
    SequentialSnapKnob()
    {
        minAngle = -0.83 * M_PI;
        maxAngle = 0.83 * M_PI;
        snap = true;
    }
};

struct SequentialSnapKnobLg : RoundLargeBlackKnob
{
    SequentialSnapKnobLg()
    {
        minAngle = -0.83 * M_PI;
        maxAngle = 0.83 * M_PI;
        snap = true;
    }
};

struct Sequential : Module {
    enum ParamIds {
        ENUMS(LOCK_PARAM, 8),
        MODE_PARAM,
        STEP_SIZE_PARAM,
        STEPS_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        ENUMS(IN_INPUT, 8),
        STEP_INPUT,
        NUM_INPUTS
    };
    enum OutputIds {
        ENUMS(OUT_GATE, 8),
        OUT_OUTPUT,
        NUM_OUTPUTS
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

    bool init = false;
    float seq_notes[8] = {-99,-99,-99,-99,-99,-99,-99,-99};
    float seq_octaves[8] = {-99,-99,-99,-99,-99,-99,-99,-99};
    float last_notes[8] = {-99,-99,-99,-99,-99,-99,-99,-99};
    float last_octaves[8] = {-99,-99,-99,-99,-99,-99,-99,-99};

    int mut_counter = 0;

    // Static
    float notes[12] = {0,   0.08, 0.17, 0.25, 0.33, 0.42,
                     0.5, 0.58, 0.67, 0.75, 0.83, 0.92};
    int octaves[6] = {-1, 0, 1, 2, 3, 4};

    // Modes
    int MODE_NORMAL[8] = {0,1,2,3,4,5,6,7};
    bool fb_forward = true;
    int MODE_FORWARD_BACK[14] = {0,1,2,3,4,5,6,7,6,5,4,3,2,1};
    int CRAB[14] = {0,1,0,2,0,3,0,4,0,5,0,6,0,7};
    bool crab_zero = true;
    int last_crab = 1;
    bool z_fwd = true;
    int ZED[8] = {0,4,1,5,2,6,3,7};

    Sequential() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(Sequential::STEPS_PARAM, 1.0f, 8.0f, 8.0f, "Steps");
        configParam(Sequential::MODE_PARAM, 1.0f, 5.0f, 1.0f, "Mode");
        configParam(Sequential::STEP_SIZE_PARAM, -7.0f, 7.0f, 1.0f, "Step Size");
    }

    void setIndex(int index, int stepSize) {
        int numSteps = (int) clamp(roundf(params[STEPS_PARAM].value), 1.0f, 8.0f);
        int mode = (int) clamp(roundf(params[MODE_PARAM].value), 1.0f, 5.0f);

        // MODE_NORMAL
        if(mode == 1){
            this->index = index + stepSize;
            if (this->index >= numSteps)
                this->index = this->index - numSteps;
            if (this->index < -numSteps)
                this->index = this->index + numSteps;
            if (this->index < 0)
                this->index = numSteps - (-1 * this->index);
        }
        // MODE_FORWARD_BACK
        if(mode == 2){

            //blah
            if(stepSize < 0){
                stepSize = -1 * stepSize;
            }

            if(fb_forward){
                this->index = this->index + stepSize;
            } else{
                this->index = this->index - stepSize;
            }

            if (this->index <= 0){
                fb_forward = true;
            }
            if (this->index >= numSteps - 1){
                fb_forward = false;
                this->index = numSteps - stepSize;
            }
        }
        // MODE_CRAB
        if(mode == 3){
            if(crab_zero){
                this->index = 0;
                crab_zero = false;
            } else{
                this->index = last_crab + stepSize;
                if (this->index >= numSteps){
                    this->index = 1;
                }
                if (this->index < 1)
                    this->index = numSteps - (-1 * this->index);
                last_crab = this->index;
                crab_zero = true;
            }
        }
        // MODE_ZEE
        if(mode == 4){

            //blah
            if(stepSize < 0){
                stepSize = -1 * stepSize;
            }

            for(int j=0;j<stepSize;j++){

                if(z_fwd){
                    this->index = this->index + 4;
                    if(this->index >= numSteps){
                        this->index = 0;
                        z_fwd = true;
                        break;
                    }
                    z_fwd = false;
                } else{
                    this->index = this->index - 3;
                    if(this->index >= numSteps){
                        this->index = 4;
                        z_fwd = true;
                        break;
                    }
                    z_fwd = true;

                }
            }
        }
        // MODE_RANDOM
        if(mode == 5){
            int range = numSteps - 0 + 1;
            int num = rand() % range + 0;
            this->index = num;
            if(this->index >=numSteps - 1){
                this->index = numSteps - 1;
            }
        }

    }

    // void step() override;

    // json_t *dataToJson() override {
    //     json_t *rootJ = json_object();
    //     // states
    //     json_t *statesJ = json_array();
    //     for (int i = 0; i < NUM_CHANNELS; i++) {
    //         json_t *stateJ = json_boolean(lock_state[i]);
    //         json_array_append_new(statesJ, stateJ);
    //     }
    //     json_object_set_new(rootJ, "states", statesJ);
    //     return rootJ;
    // }
    // void dataFromJson(json_t *rootJ) override {
    //     // states
    //     json_t *statesJ = json_object_get(rootJ, "states");
    //     if (statesJ) {
    //         for (int i = 0; i < NUM_CHANNELS; i++) {
    //             json_t *stateJ = json_array_get(statesJ, i);
    //             if (stateJ)
    //                 lock_state[i] = json_boolean_value(stateJ);
    //         }
    //     }
    // }

    void process(const ProcessArgs &args) override {

        bool gateIn = false;
        bool stepActive = false;
        if (inputs[STEP_INPUT].active) {
            if (clockTrigger.process(inputs[STEP_INPUT].value)) {
                int stepSize = params[STEP_SIZE_PARAM].value;
                setIndex(index, stepSize);
                stepActive = true;
            }
            gateIn = clockTrigger.isHigh();
        }

        // Iterate rows
        for (int i = 0; i < 8; i++) {
            // Set light
            lights[LOCK_LIGHT + i].setBrightness((index == i) ? 1.0f : 0.f);
            if (i == index){
                outputs[OUT_OUTPUT].value = inputs[IN_INPUT + i].value;
                if(stepActive){
                    outputs[OUT_GATE + i].value = 10.0f;
                } else{
                    outputs[OUT_GATE + i].value = 0.0f;
                }
            } else{
                outputs[OUT_GATE + i].value = 0.0f;
            }
        }

    }
};

template <typename BASE>
struct SeqMuteLight : BASE {
    SeqMuteLight() {
        this->box.size = mm2px(Vec(6.0, 6.0));
    }
};

struct SequentialWidget: ModuleWidget {
    SequentialWidget(Sequential *module);
};

SequentialWidget::SequentialWidget(Sequential *module) {
    setModule(module);
    setPanel(SVG::load(assetPlugin(pluginInstance, "res/Sequential.svg")));

    // MAGNETS
    // addChild(createWidget<ScrewSilver>(Vec(15, 0)));
    // addChild(createWidget<ScrewSilver>(Vec(box.size.x - 30, 0)));
    // addChild(createWidget<ScrewSilver>(Vec(15, 365)));
    // addChild(createWidget<ScrewSilver>(Vec(box.size.x - 30, 365)));

    addInput(createPort<PJ301MPort>(mm2px(Vec(4.214, 17.810)), PortWidget::INPUT, module, Sequential::IN_INPUT + 0));
    addInput(createPort<PJ301MPort>(mm2px(Vec(4.214, 27.809)), PortWidget::INPUT, module, Sequential::IN_INPUT + 1));
    addInput(createPort<PJ301MPort>(mm2px(Vec(4.214, 37.809)), PortWidget::INPUT, module, Sequential::IN_INPUT + 2));
    addInput(createPort<PJ301MPort>(mm2px(Vec(4.214, 47.810)), PortWidget::INPUT, module, Sequential::IN_INPUT + 3));
    addInput(createPort<PJ301MPort>(mm2px(Vec(4.214, 57.810)), PortWidget::INPUT, module, Sequential::IN_INPUT + 4));
    addInput(createPort<PJ301MPort>(mm2px(Vec(4.214, 67.809)), PortWidget::INPUT, module, Sequential::IN_INPUT + 5));
    addInput(createPort<PJ301MPort>(mm2px(Vec(4.214, 77.810)), PortWidget::INPUT, module, Sequential::IN_INPUT + 6));
    addInput(createPort<PJ301MPort>(mm2px(Vec(4.214, 87.810)), PortWidget::INPUT, module, Sequential::IN_INPUT + 7));

    addOutput(createPort<PJ301MPort>(mm2px(Vec(16.57, 17.810)), PortWidget::OUTPUT, module, Sequential::OUT_GATE + 0));
    addOutput(createPort<PJ301MPort>(mm2px(Vec(16.57, 27.809)), PortWidget::OUTPUT, module, Sequential::OUT_GATE + 1));
    addOutput(createPort<PJ301MPort>(mm2px(Vec(16.57, 37.809)), PortWidget::OUTPUT, module, Sequential::OUT_GATE + 2));
    addOutput(createPort<PJ301MPort>(mm2px(Vec(16.57, 47.810)), PortWidget::OUTPUT, module, Sequential::OUT_GATE + 3));
    addOutput(createPort<PJ301MPort>(mm2px(Vec(16.57, 57.810)), PortWidget::OUTPUT, module, Sequential::OUT_GATE + 4));
    addOutput(createPort<PJ301MPort>(mm2px(Vec(16.57, 67.809)), PortWidget::OUTPUT, module, Sequential::OUT_GATE + 5));
    addOutput(createPort<PJ301MPort>(mm2px(Vec(16.57, 77.810)), PortWidget::OUTPUT, module, Sequential::OUT_GATE + 6));
    addOutput(createPort<PJ301MPort>(mm2px(Vec(16.57, 87.810)), PortWidget::OUTPUT, module, Sequential::OUT_GATE + 6));

    addParam(createParam<LEDBezel>(mm2px(Vec(28.214, 17.810)), module, Sequential::LOCK_PARAM + 0));
    addParam(createParam<LEDBezel>(mm2px(Vec(28.214, 27.809)), module, Sequential::LOCK_PARAM + 1));
    addParam(createParam<LEDBezel>(mm2px(Vec(28.214, 37.809)), module, Sequential::LOCK_PARAM + 2));
    addParam(createParam<LEDBezel>(mm2px(Vec(28.214, 47.810)), module, Sequential::LOCK_PARAM + 3));
    addParam(createParam<LEDBezel>(mm2px(Vec(28.214, 57.810)), module, Sequential::LOCK_PARAM + 4));
    addParam(createParam<LEDBezel>(mm2px(Vec(28.214, 67.809)), module, Sequential::LOCK_PARAM + 5));
    addParam(createParam<LEDBezel>(mm2px(Vec(28.214, 77.810)), module, Sequential::LOCK_PARAM + 6));
    addParam(createParam<LEDBezel>(mm2px(Vec(28.214, 87.810)), module, Sequential::LOCK_PARAM + 7));

    addChild(createLight<SeqMuteLight<GreenLight>>(mm2px(Vec(28.214 + 0.75, 17.81 + 0.75)), module, Sequential::LOCK_LIGHT + 0));
    addChild(createLight<SeqMuteLight<GreenLight>>(mm2px(Vec(28.214 + 0.75, 27.809 + 0.75)), module, Sequential::LOCK_LIGHT + 1));
    addChild(createLight<SeqMuteLight<GreenLight>>(mm2px(Vec(28.214 + 0.75, 37.809 + 0.75)), module, Sequential::LOCK_LIGHT + 2));
    addChild(createLight<SeqMuteLight<GreenLight>>(mm2px(Vec(28.214 + 0.75, 47.81 + 0.75)), module, Sequential::LOCK_LIGHT + 3));
    addChild(createLight<SeqMuteLight<GreenLight>>(mm2px(Vec(28.214 + 0.75, 57.81 + 0.75)), module, Sequential::LOCK_LIGHT + 4));
    addChild(createLight<SeqMuteLight<GreenLight>>(mm2px(Vec(28.214 + 0.75, 67.809 + 0.75)), module, Sequential::LOCK_LIGHT + 5));
    addChild(createLight<SeqMuteLight<GreenLight>>(mm2px(Vec(28.214 + 0.75, 77.81 + 0.75)), module, Sequential::LOCK_LIGHT + 6));
    addChild(createLight<SeqMuteLight<GreenLight>>(mm2px(Vec(28.214 + 0.75, 87.81 + 0.75)), module, Sequential::LOCK_LIGHT + 7));

    // addParam(createParam<SequentialSnapKnob>(mm2px(Vec(16.57, 17.165)), module, Sequential::OCT_PARAM + 0));
    // addParam(createParam<SequentialSnapKnob>(mm2px(Vec(16.57, 27.164)), module, Sequential::OCT_PARAM + 1));
    // addParam(createParam<SequentialSnapKnob>(mm2px(Vec(16.57, 37.164)), module, Sequential::OCT_PARAM + 2));
    // addParam(createParam<SequentialSnapKnob>(mm2px(Vec(16.57, 47.165)), module, Sequential::OCT_PARAM + 3));
    // addParam(createParam<SequentialSnapKnob>(mm2px(Vec(16.57, 57.164)), module, Sequential::OCT_PARAM + 4));
    // addParam(createParam<SequentialSnapKnob>(mm2px(Vec(16.57, 67.165)), module, Sequential::OCT_PARAM + 5));
    // addParam(createParam<SequentialSnapKnob>(mm2px(Vec(16.57, 77.164)), module, Sequential::OCT_PARAM + 6));
    // addParam(createParam<SequentialSnapKnob>(mm2px(Vec(16.57, 87.164)), module, Sequential::OCT_PARAM + 7));

    // addParam(createParam<SequentialSnapKnob>(mm2px(Vec(28.214, 17.165)),  module, Sequential::SEMI_PARAM + 0));
    // addParam(createParam<SequentialSnapKnob>(mm2px(Vec(28.214, 27.164)),  module, Sequential::SEMI_PARAM + 1));
    // addParam(createParam<SequentialSnapKnob>(mm2px(Vec(28.214, 37.164)),  module, Sequential::SEMI_PARAM + 2));
    // addParam(createParam<SequentialSnapKnob>(mm2px(Vec(28.214, 47.165)),  module, Sequential::SEMI_PARAM + 3));
    // addParam(createParam<SequentialSnapKnob>(mm2px(Vec(28.214, 57.164)),  module, Sequential::SEMI_PARAM + 4));
    // addParam(createParam<SequentialSnapKnob>(mm2px(Vec(28.214, 67.165)),  module, Sequential::SEMI_PARAM + 5));
    // addParam(createParam<SequentialSnapKnob>(mm2px(Vec(28.214, 77.164)),  module, Sequential::SEMI_PARAM + 6));
    // addParam(createParam<SequentialSnapKnob>(mm2px(Vec(28.214, 87.164)),  module, Sequential::SEMI_PARAM + 7));

    // Sequential Params
    addParam(createParam<SequentialSnapKnobLg>(mm2px(Vec(5.0, 101.0)), module, Sequential::MODE_PARAM));
    addParam(createParam<SequentialSnapKnobLg>(mm2px(Vec(23.0, 101.0)), module, Sequential::STEP_SIZE_PARAM));

    // Ins/Outs
    addInput(createPort<PJ301MPort>(mm2px(Vec(4.214, 117.809)), PortWidget::INPUT, module, Sequential::STEP_INPUT));
    addParam(createParam<SequentialSnapKnob>(mm2px(Vec(16.57, 117.809)), module, Sequential::STEPS_PARAM));
    addOutput(createPort<PJ301MPort>(mm2px(Vec(28.214, 117.809)), PortWidget::OUTPUT, module, Sequential::OUT_OUTPUT));


}

Model *modelSequential = createModel<Sequential, SequentialWidget>("Sequential");
