#include "RJModules.hpp"

#define HISTORY_SIZE (1<<21)

struct FilterDelay : Module {
    enum ParamIds {
        TIME_PARAM,
        FEEDBACK_PARAM,
        COLOR_PARAM,
        MIX_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        TIME_INPUT,
        FEEDBACK_INPUT,
        COLOR_INPUT,
        MIX_INPUT,
        IN_INPUT,
        NUM_INPUTS
    };
    enum OutputIds {
        OUT_OUTPUT,
        NUM_OUTPUTS
    };

    dsp::DoubleRingBuffer<float, HISTORY_SIZE> historyBuffer;
    dsp::DoubleRingBuffer<float, 16> outBuffer;
    dsp::SampleRateConverter<1> src;
    float lastWet = 0.0;
    dsp::RCFilter lowpassFilter;
    dsp::RCFilter highpassFilter;

    FilterDelay() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS);
        configParam(FilterDelay::TIME_PARAM, 0.0, 1.0, 0.5, "");
        configParam(FilterDelay::FEEDBACK_PARAM, 0.0, 1.0, 0.5, "");
        configParam(FilterDelay::COLOR_PARAM, 0.0, 1.0, 0.5, "");
        configParam(FilterDelay::MIX_PARAM, 0.0, 1.0, 0.5, "");
}


    void step() override;
};


void FilterDelay::step() {
    // Get input to delay block
    float in = inputs[IN_INPUT].value;
    float feedback = clamp(params[FEEDBACK_PARAM].value + inputs[FEEDBACK_INPUT].value / 10.0f, 0.0f, 0.99f);
    float dry = in + lastWet * feedback;

    // Compute delay time in seconds
    float delay = .001 * powf(10.0 / .001, clamp(params[TIME_PARAM].value + inputs[TIME_INPUT].value / 10.0f, 0.0f, 1.0f));
    // Number of delay samples
    float index = delay * APP->engine->getSampleRate();

    // The delay algorithm is taken from Fundamentals,
    // which it seems is written by somebody who didn't know what they're doing,
    // at least according to the old comments.

    // Push dry sample into history buffer
    if (!historyBuffer.full()) {
        historyBuffer.push(dry);
    }

    // How many samples do we need consume to catch up?
    float consume = index - historyBuffer.size();
    if (outBuffer.empty()) {
        double ratio = 1.0;
        if (consume <= -16)
            ratio = 0.5;
        else if (consume >= 16)
            ratio = 2.0;

        float inSR = APP->engine->getSampleRate();
        float outSR = ratio * inSR;

        int inFrames = fmin(historyBuffer.size(), 16);
        int outFrames = outBuffer.capacity();
        src.setRates(inSR, outSR);
        src.process((const dsp::Frame<1>*)historyBuffer.startData(), &inFrames, (dsp::Frame<1>*)outBuffer.endData(), &outFrames);
        historyBuffer.startIncr(inFrames);
        outBuffer.endIncr(outFrames);
    }

    float wet = 0.0;
    if (!outBuffer.empty()) {
        wet = outBuffer.shift();
    }

    float color = clamp(params[COLOR_PARAM].value + inputs[COLOR_INPUT].value / 10.0f, 0.0f, 1.0f);
    float lowpassFreq = 4000.0 * powf(10.0, clamp(2.0f*color, 0.0f, 1.0f));
    lowpassFilter.setCutoff(lowpassFreq / APP->engine->getSampleRate());
    lowpassFilter.process(wet);
    wet = lowpassFilter.lowpass();

    // No muddy sub
    float highpassFreq = 80.0;
    highpassFilter.setCutoff(highpassFreq / APP->engine->getSampleRate());
    highpassFilter.process(wet);
    wet = highpassFilter.highpass();

    if (!historyBuffer.full()) {
        historyBuffer.push(wet);
    }

    lastWet = wet;

    float mix = clamp(params[MIX_PARAM].value + inputs[MIX_INPUT].value / 10.0f, 0.0f, 1.0f);
    float out = crossfade(in, wet, mix);
    outputs[OUT_OUTPUT].value = out;
}


struct FilterDelayWidget: ModuleWidget {
    FilterDelayWidget(FilterDelay *module);
};

FilterDelayWidget::FilterDelayWidget(FilterDelay *module) {
		setModule(module);
    box.size = Vec(15*10, 380);

    {
        SVGPanel *panel = new SVGPanel();
        panel->box.size = box.size;
        panel->setBackground(APP->window->loadSvg(asset::plugin(pluginInstance, "res/FilterDelay.svg")));
        addChild(panel);
    }

    addChild(createWidget<ScrewSilver>(Vec(15, 0)));
    addChild(createWidget<ScrewSilver>(Vec(box.size.x-30, 0)));
    addChild(createWidget<ScrewSilver>(Vec(15, 365)));
    addChild(createWidget<ScrewSilver>(Vec(box.size.x-30, 365)));

    addParam(createParam<RoundBlackKnob>(Vec(97, 60), module, FilterDelay::TIME_PARAM));
    addParam(createParam<RoundBlackKnob>(Vec(97, 120), module, FilterDelay::FEEDBACK_PARAM));
    addParam(createParam<RoundBlackKnob>(Vec(97, 180), module, FilterDelay::COLOR_PARAM));
    addParam(createParam<RoundBlackKnob>(Vec(97, 240), module, FilterDelay::MIX_PARAM));

    addInput(createInput<PJ301MPort>(Vec(22, 65), module, FilterDelay::TIME_INPUT));
    addInput(createInput<PJ301MPort>(Vec(22, 125), module, FilterDelay::FEEDBACK_INPUT));
    addInput(createInput<PJ301MPort>(Vec(22, 185), module, FilterDelay::COLOR_INPUT));
    addInput(createInput<PJ301MPort>(Vec(22, 245), module, FilterDelay::MIX_INPUT));

    addInput(createInput<PJ301MPort>(Vec(22, 305), module, FilterDelay::IN_INPUT));
    addOutput(createOutput<PJ301MPort>(Vec(105, 305), module, FilterDelay::OUT_OUTPUT));
}
Model *modelFilterDelay = createModel<FilterDelay, FilterDelayWidget>("FilterDelay");
