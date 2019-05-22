#include "RJModules.hpp"
#include "dsp/samplerate.hpp"
#include "dsp/ringbuffer.hpp"
#include "dsp/filter.hpp"

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

    DoubleRingBuffer<float, HISTORY_SIZE> historyBuffer;
    DoubleRingBuffer<float, 16> outBuffer;
    SampleRateConverter<1> src;
    float lastWet = 0.0;
    RCFilter lowpassFilter;
    RCFilter highpassFilter;

    ReplayKnob() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS) {}

    void step() override;
};


void ReplayKnob::step() {

    float param = params[BIG_PARAM].value;

    // If we've never recorded anything, or we're recording right now, pass the knob value through.
    outputs[OUT_OUTPUT].value = param;
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

    addParam(ParamWidget::create<RoundHugeBlackKnob>(Vec(47, 61), module, ReplayKnob::BIG_PARAM, 0.0, 8.0, 5.0));
    addInput(Port::create<PJ301MPort>(Vec(17, 60), Port::INPUT, module, ReplayKnob::REC_CV_INPUT));
    addParam(ParamWidget::create<LEDButton>(Vec(18, 110), module, ReplayKnob::REC_PARAM, 0.0, 1.0, 0.0));

    // addInput(Port::create<PJ301MPort>(Vec(22, 65), Port::INPUT, module, ReplayKnob::TIME_INPUT));
    addOutput(Port::create<PJ301MPort>(Vec(105, 305), Port::OUTPUT, module, ReplayKnob::OUT_OUTPUT));
}
Model *modelReplayKnob = Model::create<ReplayKnob, ReplayKnobWidget>("RJModules", "ReplayKnob", "[LIVE] ReplayKnob", DELAY_TAG);
