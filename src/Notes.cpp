#include "RJModules.hpp"

#define NUM_CHANNELS 10

struct NotesSnapKnob : RoundSmallBlackKnob
{
    NotesSnapKnob()
    {
        minAngle = -0.83 * M_PI;
        maxAngle = 0.83 * M_PI;
        snap = true;
    }
};

struct NotesSnapKnobLg : RoundBlackKnob
{
    NotesSnapKnobLg()
    {
        minAngle = -0.83 * M_PI;
        maxAngle = 0.83 * M_PI;
        snap = true;
    }
};

struct Notes : Module {
    enum ParamIds {
        ENUMS(OCT_PARAM, 10),
        ENUMS(SEMI_PARAM, 10),
        NUM_PARAMS
    };
    enum InputIds {
        NUM_INPUTS
    };
    enum OutputIds {
        ENUMS(OUT_OUTPUT, 10),
        OUT_POLY,
        NUM_OUTPUTS
    };
    enum LightIds {
        NUM_LIGHTS
    };

    // Static
    float notes[12] = {0,   0.08, 0.17, 0.25, 0.33, 0.42,
                     0.5, 0.58, 0.67, 0.75, 0.83, 0.92};
    int octaves[6] = {-1, 0, 1, 2, 3, 4};

    Notes() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

        configParam(Notes::OCT_PARAM + 0, 0.0, 5.0, 0.0, string::f("Ch %d octave", 0));
        configParam(Notes::OCT_PARAM + 1, 0.0, 5.0, 0.0, string::f("Ch %d octave", 1));
        configParam(Notes::OCT_PARAM + 2, 0.0, 5.0, 0.0, string::f("Ch %d octave", 2));
        configParam(Notes::OCT_PARAM + 3, 0.0, 5.0, 0.0, string::f("Ch %d octave", 3));
        configParam(Notes::OCT_PARAM + 4, 0.0, 5.0, 0.0, string::f("Ch %d octave", 4));
        configParam(Notes::OCT_PARAM + 5, 0.0, 5.0, 0.0, string::f("Ch %d octave", 5));
        configParam(Notes::OCT_PARAM + 6, 0.0, 5.0, 0.0, string::f("Ch %d octave", 6));
        configParam(Notes::OCT_PARAM + 7, 0.0, 5.0, 0.0, string::f("Ch %d octave", 7));
        configParam(Notes::OCT_PARAM + 8, 0.0, 5.0, 0.0, string::f("Ch %d octave", 8));
        configParam(Notes::OCT_PARAM + 9, 0.0, 5.0, 0.0, string::f("Ch %d octave", 9));

        configParam(Notes::SEMI_PARAM + 0, 0.0, 11.0, 0.0, string::f("Ch %d semi", 0));
        configParam(Notes::SEMI_PARAM + 1, 0.0, 11.0, 0.0, string::f("Ch %d semi", 1));
        configParam(Notes::SEMI_PARAM + 2, 0.0, 11.0, 0.0, string::f("Ch %d semi", 2));
        configParam(Notes::SEMI_PARAM + 3, 0.0, 11.0, 0.0, string::f("Ch %d semi", 3));
        configParam(Notes::SEMI_PARAM + 4, 0.0, 11.0, 0.0, string::f("Ch %d semi", 4));
        configParam(Notes::SEMI_PARAM + 5, 0.0, 11.0, 0.0, string::f("Ch %d semi", 5));
        configParam(Notes::SEMI_PARAM + 6, 0.0, 11.0, 0.0, string::f("Ch %d semi", 6));
        configParam(Notes::SEMI_PARAM + 7, 0.0, 11.0, 0.0, string::f("Ch %d semi", 7));
        configParam(Notes::SEMI_PARAM + 8, 0.0, 11.0, 0.0, string::f("Ch %d semi", 8));
        configParam(Notes::SEMI_PARAM + 9, 0.0, 11.0, 0.0, string::f("Ch %d semi", 9));

    }

    void process(const ProcessArgs &args) override {
        for (int i = 0; i < 10; i++) {
            float o = params[OCT_PARAM + i].value;
            float n = params[SEMI_PARAM + i].value;
            float voltage = octaves[(int)o] + notes[(int)n];
            outputs[OUT_OUTPUT + i].value = voltage;
            outputs[OUT_POLY].setVoltage(voltage, i);
        }
        outputs[OUT_POLY].setChannels(10);

    }
};

template <typename BASE>
struct MuteLight : BASE {
    MuteLight() {
        this->box.size = mm2px(Vec(6.0, 6.0));
    }
};

struct NotesWidget: ModuleWidget {
    NotesWidget(Notes *module);
};

NotesWidget::NotesWidget(Notes *module) {
    setModule(module);
    setPanel(SVG::load(assetPlugin(pluginInstance, "res/Notes.svg")));

    // MAGNETS
    // addChild(createWidget<ScrewSilver>(Vec(15, 0)));
    // addChild(createWidget<ScrewSilver>(Vec(box.size.x - 30, 0)));
    // addChild(createWidget<ScrewSilver>(Vec(15, 365)));
    // addChild(createWidget<ScrewSilver>(Vec(box.size.x - 30, 365)));

    float LEFT = 4.214;
    float MIDDLE = 16.57;
    float RIGHT = 28.214;

    addParam(createParam<NotesSnapKnob>(mm2px(Vec(LEFT, 17.165)), module, Notes::OCT_PARAM + 0));
    addParam(createParam<NotesSnapKnob>(mm2px(Vec(LEFT, 27.164)), module, Notes::OCT_PARAM + 1));
    addParam(createParam<NotesSnapKnob>(mm2px(Vec(LEFT, 37.164)), module, Notes::OCT_PARAM + 2));
    addParam(createParam<NotesSnapKnob>(mm2px(Vec(LEFT, 47.165)), module, Notes::OCT_PARAM + 3));
    addParam(createParam<NotesSnapKnob>(mm2px(Vec(LEFT, 57.164)), module, Notes::OCT_PARAM + 4));
    addParam(createParam<NotesSnapKnob>(mm2px(Vec(LEFT, 67.165)), module, Notes::OCT_PARAM + 5));
    addParam(createParam<NotesSnapKnob>(mm2px(Vec(LEFT, 77.164)), module, Notes::OCT_PARAM + 6));
    addParam(createParam<NotesSnapKnob>(mm2px(Vec(LEFT, 87.164)), module, Notes::OCT_PARAM + 7));
    addParam(createParam<NotesSnapKnob>(mm2px(Vec(LEFT, 97.164)), module, Notes::OCT_PARAM + 8));
    addParam(createParam<NotesSnapKnob>(mm2px(Vec(LEFT, 107.164)), module, Notes::OCT_PARAM + 9));

    addParam(createParam<NotesSnapKnob>(mm2px(Vec(MIDDLE, 17.165)),  module, Notes::SEMI_PARAM + 0));
    addParam(createParam<NotesSnapKnob>(mm2px(Vec(MIDDLE, 27.164)),  module, Notes::SEMI_PARAM + 1));
    addParam(createParam<NotesSnapKnob>(mm2px(Vec(MIDDLE, 37.164)),  module, Notes::SEMI_PARAM + 2));
    addParam(createParam<NotesSnapKnob>(mm2px(Vec(MIDDLE, 47.165)),  module, Notes::SEMI_PARAM + 3));
    addParam(createParam<NotesSnapKnob>(mm2px(Vec(MIDDLE, 57.164)),  module, Notes::SEMI_PARAM + 4));
    addParam(createParam<NotesSnapKnob>(mm2px(Vec(MIDDLE, 67.165)),  module, Notes::SEMI_PARAM + 5));
    addParam(createParam<NotesSnapKnob>(mm2px(Vec(MIDDLE, 77.164)),  module, Notes::SEMI_PARAM + 6));
    addParam(createParam<NotesSnapKnob>(mm2px(Vec(MIDDLE, 87.164)),  module, Notes::SEMI_PARAM + 7));
    addParam(createParam<NotesSnapKnob>(mm2px(Vec(MIDDLE, 97.164)),  module, Notes::SEMI_PARAM + 8));
    addParam(createParam<NotesSnapKnob>(mm2px(Vec(MIDDLE, 107.164)),  module, Notes::SEMI_PARAM + 9));

    addOutput(createPort<PJ301MPort>(mm2px(Vec(RIGHT, 17.165)), PortWidget::OUTPUT, module, Notes::OUT_OUTPUT + 0));
    addOutput(createPort<PJ301MPort>(mm2px(Vec(RIGHT, 27.164)), PortWidget::OUTPUT, module, Notes::OUT_OUTPUT + 1));
    addOutput(createPort<PJ301MPort>(mm2px(Vec(RIGHT, 37.164)), PortWidget::OUTPUT, module, Notes::OUT_OUTPUT + 2));
    addOutput(createPort<PJ301MPort>(mm2px(Vec(RIGHT, 47.165)), PortWidget::OUTPUT, module, Notes::OUT_OUTPUT + 3));
    addOutput(createPort<PJ301MPort>(mm2px(Vec(RIGHT, 57.164)), PortWidget::OUTPUT, module, Notes::OUT_OUTPUT + 4));
    addOutput(createPort<PJ301MPort>(mm2px(Vec(RIGHT, 67.165)), PortWidget::OUTPUT, module, Notes::OUT_OUTPUT + 5));
    addOutput(createPort<PJ301MPort>(mm2px(Vec(RIGHT, 77.164)), PortWidget::OUTPUT, module, Notes::OUT_OUTPUT + 6));
    addOutput(createPort<PJ301MPort>(mm2px(Vec(RIGHT, 87.164)), PortWidget::OUTPUT, module, Notes::OUT_OUTPUT + 7));
    addOutput(createPort<PJ301MPort>(mm2px(Vec(RIGHT, 97.164)), PortWidget::OUTPUT, module, Notes::OUT_OUTPUT + 8));
    addOutput(createPort<PJ301MPort>(mm2px(Vec(RIGHT, 107.164)), PortWidget::OUTPUT, module, Notes::OUT_OUTPUT + 9));

    // Ins/Outs
    addOutput(createPort<PJ301MPort>(mm2px(Vec(28.214, 117.809)), PortWidget::OUTPUT, module, Notes::OUT_POLY));

}

Model *modelNotes = createModel<Notes, NotesWidget>("Notes");
