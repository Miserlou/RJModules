#include "RJModules.hpp"
#include <iostream>
#include <stdlib.h>
#include <random>
#include <cmath>

#define NUM_CHANNELS 10
#define HISTORY_SIZE (1<<21)

struct Buffers : Module {
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

    dsp::DoubleRingBuffer<float, HISTORY_SIZE> historyBuffer;
    dsp::DoubleRingBuffer<float, 16> outBuffer;
    dsp::SampleRateConverter<1> src;
    dsp::DoubleRingBuffer<float, HISTORY_SIZE> historyBuffer10;
    dsp::DoubleRingBuffer<float, 16> outBuffer10;
    dsp::SampleRateConverter<1> src10;
    dsp::DoubleRingBuffer<float, HISTORY_SIZE> historyBuffer2;
    dsp::DoubleRingBuffer<float, 16> outBuffer2;
    dsp::SampleRateConverter<1> src2;
    dsp::DoubleRingBuffer<float, HISTORY_SIZE> historyBuffer3;
    dsp::DoubleRingBuffer<float, 16> outBuffer3;
    dsp::SampleRateConverter<1> src3;
    dsp::DoubleRingBuffer<float, HISTORY_SIZE> historyBuffer4;
    dsp::DoubleRingBuffer<float, 16> outBuffer4;
    dsp::SampleRateConverter<1> src4;
    dsp::DoubleRingBuffer<float, HISTORY_SIZE> historyBuffer5;
    dsp::DoubleRingBuffer<float, 16> outBuffer5;
    dsp::SampleRateConverter<1> src5;
    dsp::DoubleRingBuffer<float, HISTORY_SIZE> historyBuffer6;
    dsp::DoubleRingBuffer<float, 16> outBuffer6;
    dsp::SampleRateConverter<1> src6;
    dsp::DoubleRingBuffer<float, HISTORY_SIZE> historyBuffer7;
    dsp::DoubleRingBuffer<float, 16> outBuffer7;
    dsp::SampleRateConverter<1> src7;
    dsp::DoubleRingBuffer<float, HISTORY_SIZE> historyBuffer8;
    dsp::DoubleRingBuffer<float, 16> outBuffer8;
    dsp::SampleRateConverter<1> src8;
    dsp::DoubleRingBuffer<float, HISTORY_SIZE> historyBuffer9;
    dsp::DoubleRingBuffer<float, 16> outBuffer9;
    dsp::SampleRateConverter<1> src9;


    Buffers() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(Buffers::MUTE_PARAM + 0, 0.0, 3.6, 0.0, "");
        configParam(Buffers::MUTE_PARAM + 1, 0.0, 3.6, 0.0, "");
        configParam(Buffers::MUTE_PARAM + 2, 0.0, 3.6, 0.0, "");
        configParam(Buffers::MUTE_PARAM + 3, 0.0, 3.6, 0.0, "");
        configParam(Buffers::MUTE_PARAM + 4, 0.0, 3.6, 0.0, "");
        configParam(Buffers::MUTE_PARAM + 5, 0.0, 2.6, 0.0, "");
        configParam(Buffers::MUTE_PARAM + 6, 0.0, 3.6, 0.0, "");
        configParam(Buffers::MUTE_PARAM + 7, 0.0, 3.6, 0.0, "");
        configParam(Buffers::MUTE_PARAM + 8, 0.0, 3.6, 0.0, "");
        configParam(Buffers::MUTE_PARAM + 9, 0.0, 3.6, 0.0, "");
        // reset();
    }
    void step() override;

    // void reset() override {
    //     for (int i = 0; i < NUM_CHANNELS; i++) {
    //         state[i] = true;
    //     }
    // }
    // void randomize() override {
    //     for (int i = 0; i < NUM_CHANNELS; i++) {
    //         state[i] = (random::uniform() < 0.5);
    //     }
    // }

    // json_t *dataToJson() override {
    //     json_t *rootJ = json_object();
    //     // states
    //     json_t *statesJ = json_array();
    //     for (int i = 0; i < NUM_CHANNELS; i++) {
    //         json_t *stateJ = json_boolean(state[i]);
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
    //                 state[i] = json_boolean_value(stateJ);
    //         }
    //     }
    // }
};

void Buffers::step() {
    for (int i = 0; i < NUM_CHANNELS; i++) {
        float in = inputs[IN_INPUT + i].value;
        outputs[OUT_OUTPUT + i].value = in + round(params[MUTE_PARAM + i].value);
    }

    int outFrames = outBuffer.capacity();
    double ratio = 1.0;
    float inSR = APP->engine->getSampleRate();
    float outSR = ratio * inSR;
    int inFrames = fmin(historyBuffer.size(), 16);


    /*
        Oh god I'm so sorry I'm just so lazy right now
        Don't judge me on this, just send a PR
    */

    /*
        1
    */
    float in = inputs[IN_INPUT].value;
    // Compute delay time in seconds
    float delay = .01 * params[MUTE_PARAM].value;
    // Number of delay samples
    float index = delay * APP->engine->getSampleRate();
    if (!historyBuffer.full()) {
        historyBuffer.push(in);
    }
    // How many samples do we need consume to catch up?
    float consume = index - historyBuffer.size();
    if (outBuffer.empty()) {
        ratio = 1.0;
        if (consume <= -16)
            ratio = 0.5;
        else if (consume >= 16)
            ratio = 2.0;
        inSR = APP->engine->getSampleRate();
        outSR = ratio * inSR;
        inFrames = fmin(historyBuffer.size(), 16);
        outFrames = outBuffer.capacity();
        src.setRates(inSR, outSR);
        src.process((const dsp::Frame<1>*)historyBuffer.startData(), &inFrames, (dsp::Frame<1>*)outBuffer.endData(), &outFrames);
        historyBuffer.startIncr(inFrames);
        outBuffer.endIncr(outFrames);
    }

    float wet = 0.0;
    if (!outBuffer.empty()) {
        wet = outBuffer.shift();
    }
    outputs[OUT_OUTPUT].value = wet;

    /*
        2
    */
    in = inputs[IN_INPUT + 1].value;
    // Compute delay time in seconds
    delay = .01 * params[MUTE_PARAM + 1].value;
    // Number of delay samples
    index = delay * APP->engine->getSampleRate();
    if (!historyBuffer2.full()) {
        historyBuffer2.push(in);
    }
    // How many samples do we need consume to catch up?
    consume = index - historyBuffer2.size();
    if (outBuffer2.empty()) {
        ratio = 1.0;
        if (consume <= -16)
            ratio = 0.5;
        else if (consume >= 16)
            ratio = 2.0;
        outSR = ratio * inSR;
        inFrames = fmin(historyBuffer2.size(), 16);
        outFrames = outBuffer2.capacity();
        src.setRates(inSR, outSR);
        src.process((const dsp::Frame<1>*)historyBuffer2.startData(), &inFrames, (dsp::Frame<1>*)outBuffer2.endData(), &outFrames);
        historyBuffer2.startIncr(inFrames);
        outBuffer2.endIncr(outFrames);
    }
    wet = 0.0;
    if (!outBuffer2.empty()) {
        wet = outBuffer2.shift();
    }
    outputs[OUT_OUTPUT+1].value = wet;

    /*
        3
    */
    in = inputs[IN_INPUT + 2].value;
    // Compute delay time in seconds
    delay = .01 * params[MUTE_PARAM + 2].value;
    // Number of delay samples
    index = delay * APP->engine->getSampleRate();
    if (!historyBuffer3.full()) {
        historyBuffer3.push(in);
    }
    // How many samples do we need consume to catch up?
    consume = index - historyBuffer3.size();
    if (outBuffer3.empty()) {
        ratio = 1.0;
        if (consume <= -16)
            ratio = 0.5;
        else if (consume >= 16)
            ratio = 2.0;
        outSR = ratio * inSR;
        inFrames = fmin(historyBuffer3.size(), 16);
        outFrames = outBuffer3.capacity();
        src.setRates(inSR, outSR);
        src.process((const dsp::Frame<1>*)historyBuffer3.startData(), &inFrames, (dsp::Frame<1>*)outBuffer3.endData(), &outFrames);
        historyBuffer3.startIncr(inFrames);
        outBuffer3.endIncr(outFrames);
    }
    wet = 0.0;
    if (!outBuffer3.empty()) {
        wet = outBuffer3.shift();
    }
    outputs[OUT_OUTPUT+2].value = wet;

    /*
        4
    */
    in = inputs[IN_INPUT + 3].value;
    // Compute delay time in seconds
    delay = .01 * params[MUTE_PARAM + 3].value;
    // Number of delay samples
    index = delay * APP->engine->getSampleRate();
    if (!historyBuffer4.full()) {
        historyBuffer4.push(in);
    }
    // How many samples do we need consume to catch up?
    consume = index - historyBuffer4.size();
    if (outBuffer4.empty()) {
        ratio = 1.0;
        if (consume <= -16)
            ratio = 0.5;
        else if (consume >= 16)
            ratio = 2.0;
        outSR = ratio * inSR;
        inFrames = fmin(historyBuffer4.size(), 16);
        outFrames = outBuffer4.capacity();
        src.setRates(inSR, outSR);
        src.process((const dsp::Frame<1>*)historyBuffer4.startData(), &inFrames, (dsp::Frame<1>*)outBuffer4.endData(), &outFrames);
        historyBuffer4.startIncr(inFrames);
        outBuffer4.endIncr(outFrames);
    }
    wet = 0.0;
    if (!outBuffer4.empty()) {
        wet = outBuffer4.shift();
    }
    outputs[OUT_OUTPUT+3].value = wet;

    /*
        5
    */
    in = inputs[IN_INPUT + 4].value;
    // Compute delay time in seconds
    delay = .01 * params[MUTE_PARAM + 4].value;
    // Number of delay samples
    index = delay * APP->engine->getSampleRate();
    if (!historyBuffer5.full()) {
        historyBuffer5.push(in);
    }
    // How many samples do we need consume to catch up?
    consume = index - historyBuffer5.size();
    if (outBuffer5.empty()) {
        ratio = 1.0;
        if (consume <= -16)
            ratio = 0.5;
        else if (consume >= 16)
            ratio = 2.0;
        outSR = ratio * inSR;
        inFrames = fmin(historyBuffer5.size(), 16);
        outFrames = outBuffer5.capacity();
        src.setRates(inSR, outSR);
        src.process((const dsp::Frame<1>*)historyBuffer5.startData(), &inFrames, (dsp::Frame<1>*)outBuffer5.endData(), &outFrames);
        historyBuffer5.startIncr(inFrames);
        outBuffer5.endIncr(outFrames);
    }
    wet = 0.0;
    if (!outBuffer5.empty()) {
        wet = outBuffer5.shift();
    }
    outputs[OUT_OUTPUT+4].value = wet;

    /*
        6
    */
    in = inputs[IN_INPUT + 5].value;
    // Compute delay time in seconds
    delay = .01 * params[MUTE_PARAM + 5].value;
    // Number of delay samples
    index = delay * APP->engine->getSampleRate();
    if (!historyBuffer6.full()) {
        historyBuffer6.push(in);
    }
    // How many samples do we need consume to catch up?
    consume = index - historyBuffer6.size();
    if (outBuffer6.empty()) {
        ratio = 1.0;
        if (consume <= -16)
            ratio = 0.5;
        else if (consume >= 16)
            ratio = 2.0;
        outSR = ratio * inSR;
        inFrames = fmin(historyBuffer6.size(), 16);
        outFrames = outBuffer6.capacity();
        src.setRates(inSR, outSR);
        src.process((const dsp::Frame<1>*)historyBuffer6.startData(), &inFrames, (dsp::Frame<1>*)outBuffer6.endData(), &outFrames);
        historyBuffer6.startIncr(inFrames);
        outBuffer6.endIncr(outFrames);
    }
    wet = 0.0;
    if (!outBuffer6.empty()) {
        wet = outBuffer6.shift();
    }
    outputs[OUT_OUTPUT+5].value = wet;

    /*
        7
    */
    in = inputs[IN_INPUT + 6].value;
    // Compute delay time in seconds
    delay = .01 * params[MUTE_PARAM + 6].value;
    // Number of delay samples
    index = delay * APP->engine->getSampleRate();
    if (!historyBuffer7.full()) {
        historyBuffer7.push(in);
    }
    // How many samples do we need consume to catch up?
    consume = index - historyBuffer7.size();
    if (outBuffer7.empty()) {
        ratio = 1.0;
        if (consume <= -16)
            ratio = 0.5;
        else if (consume >= 16)
            ratio = 2.0;
        outSR = ratio * inSR;
        inFrames = fmin(historyBuffer7.size(), 16);
        outFrames = outBuffer7.capacity();
        src.setRates(inSR, outSR);
        src.process((const dsp::Frame<1>*)historyBuffer7.startData(), &inFrames, (dsp::Frame<1>*)outBuffer7.endData(), &outFrames);
        historyBuffer7.startIncr(inFrames);
        outBuffer7.endIncr(outFrames);
    }
    wet = 0.0;
    if (!outBuffer7.empty()) {
        wet = outBuffer7.shift();
    }
    outputs[OUT_OUTPUT+6].value = wet;

    /*
        8
    */
    in = inputs[IN_INPUT + 7].value;
    // Compute delay time in seconds
    delay = .01 * params[MUTE_PARAM + 7].value;
    // Number of delay samples
    index = delay * APP->engine->getSampleRate();
    if (!historyBuffer8.full()) {
        historyBuffer8.push(in);
    }
    // How many samples do we need consume to catch up?
    consume = index - historyBuffer8.size();
    if (outBuffer8.empty()) {
        ratio = 1.0;
        if (consume <= -16)
            ratio = 0.5;
        else if (consume >= 16)
            ratio = 2.0;
        outSR = ratio * inSR;
        inFrames = fmin(historyBuffer8.size(), 16);
        outFrames = outBuffer8.capacity();
        src.setRates(inSR, outSR);
        src.process((const dsp::Frame<1>*)historyBuffer8.startData(), &inFrames, (dsp::Frame<1>*)outBuffer8.endData(), &outFrames);
        historyBuffer8.startIncr(inFrames);
        outBuffer8.endIncr(outFrames);
    }
    wet = 0.0;
    if (!outBuffer8.empty()) {
        wet = outBuffer8.shift();
    }
    outputs[OUT_OUTPUT+7].value = wet;

    /*
        9
    */
    in = inputs[IN_INPUT + 8].value;
    // Compute delay time in seconds
    delay = .01 * params[MUTE_PARAM + 8].value;
    // Number of delay samples
    index = delay * APP->engine->getSampleRate();
    if (!historyBuffer9.full()) {
        historyBuffer9.push(in);
    }
    // How many samples do we need consume to catch up?
    consume = index - historyBuffer9.size();
    if (outBuffer9.empty()) {
        ratio = 1.0;
        if (consume <= -16)
            ratio = 0.5;
        else if (consume >= 16)
            ratio = 2.0;
        outSR = ratio * inSR;
        inFrames = fmin(historyBuffer9.size(), 16);
        outFrames = outBuffer9.capacity();
        src.setRates(inSR, outSR);
        src.process((const dsp::Frame<1>*)historyBuffer9.startData(), &inFrames, (dsp::Frame<1>*)outBuffer9.endData(), &outFrames);
        historyBuffer9.startIncr(inFrames);
        outBuffer9.endIncr(outFrames);
    }
    wet = 0.0;
    if (!outBuffer9.empty()) {
        wet = outBuffer9.shift();
    }
    outputs[OUT_OUTPUT+8].value = wet;

    /*
        10
    */
    in = inputs[IN_INPUT + 9].value;
    // Compute delay time in seconds
    delay = .01 * params[MUTE_PARAM + 9].value;
    // Number of delay samples
    index = delay * APP->engine->getSampleRate();
    if (!historyBuffer10.full()) {
        historyBuffer10.push(in);
    }
    // How many samples do we need consume to catch up?
    consume = index - historyBuffer10.size();
    if (outBuffer10.empty()) {
        ratio = 1.0;
        if (consume <= -16)
            ratio = 0.5;
        else if (consume >= 16)
            ratio = 2.0;
        outSR = ratio * inSR;
        inFrames = fmin(historyBuffer10.size(), 16);
        outFrames = outBuffer10.capacity();
        src.setRates(inSR, outSR);
        src.process((const dsp::Frame<1>*)historyBuffer10.startData(), &inFrames, (dsp::Frame<1>*)outBuffer10.endData(), &outFrames);
        historyBuffer10.startIncr(inFrames);
        outBuffer10.endIncr(outFrames);
    }
    wet = 0.0;
    if (!outBuffer10.empty()) {
        wet = outBuffer10.shift();
    }
    outputs[OUT_OUTPUT+9].value = wet;

}


template <typename BASE>
struct MuteLight : BASE {
    MuteLight() {
        this->box.size = mm2px(Vec(6.0, 6.0));
    }
};

struct BuffersWidget: ModuleWidget {
    BuffersWidget(Buffers *module);
};

BuffersWidget::BuffersWidget(Buffers *module) {
		setModule(module);
    setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Buffers.svg")));

    addChild(createWidget<ScrewSilver>(Vec(15, 0)));
    addChild(createWidget<ScrewSilver>(Vec(box.size.x - 30, 0)));
    addChild(createWidget<ScrewSilver>(Vec(15, 365)));
    addChild(createWidget<ScrewSilver>(Vec(box.size.x - 30, 365)));

    addParam(createParam<RoundSmallBlackKnob>(mm2px(Vec(16.57, 17.165)), module, Buffers::MUTE_PARAM + 0));
    addParam(createParam<RoundSmallBlackKnob>(mm2px(Vec(16.57, 27.164)), module, Buffers::MUTE_PARAM + 1));
    addParam(createParam<RoundSmallBlackKnob>(mm2px(Vec(16.57, 37.164)), module, Buffers::MUTE_PARAM + 2));
    addParam(createParam<RoundSmallBlackKnob>(mm2px(Vec(16.57, 47.165)), module, Buffers::MUTE_PARAM + 3));
    addParam(createParam<RoundSmallBlackKnob>(mm2px(Vec(16.57, 57.164)), module, Buffers::MUTE_PARAM + 4));
    addParam(createParam<RoundSmallBlackKnob>(mm2px(Vec(16.57, 67.165)), module, Buffers::MUTE_PARAM + 5));
    addParam(createParam<RoundSmallBlackKnob>(mm2px(Vec(16.57, 77.164)), module, Buffers::MUTE_PARAM + 6));
    addParam(createParam<RoundSmallBlackKnob>(mm2px(Vec(16.57, 87.164)), module, Buffers::MUTE_PARAM + 7));
    addParam(createParam<RoundSmallBlackKnob>(mm2px(Vec(16.57, 97.165)), module, Buffers::MUTE_PARAM + 8));
    addParam(createParam<RoundSmallBlackKnob>(mm2px(Vec(16.57, 107.166)), module, Buffers::MUTE_PARAM + 9));

    addInput(createInput<PJ301MPort>(mm2px(Vec(4.214, 17.81)), module, Buffers::IN_INPUT + 0));
    addInput(createInput<PJ301MPort>(mm2px(Vec(4.214, 27.809)), module, Buffers::IN_INPUT + 1));
    addInput(createInput<PJ301MPort>(mm2px(Vec(4.214, 37.809)), module, Buffers::IN_INPUT + 2));
    addInput(createInput<PJ301MPort>(mm2px(Vec(4.214, 47.81)), module, Buffers::IN_INPUT + 3));
    addInput(createInput<PJ301MPort>(mm2px(Vec(4.214, 57.81)), module, Buffers::IN_INPUT + 4));
    addInput(createInput<PJ301MPort>(mm2px(Vec(4.214, 67.809)), module, Buffers::IN_INPUT + 5));
    addInput(createInput<PJ301MPort>(mm2px(Vec(4.214, 77.81)), module, Buffers::IN_INPUT + 6));
    addInput(createInput<PJ301MPort>(mm2px(Vec(4.214, 87.81)), module, Buffers::IN_INPUT + 7));
    addInput(createInput<PJ301MPort>(mm2px(Vec(4.214, 97.809)), module, Buffers::IN_INPUT + 8));
    addInput(createInput<PJ301MPort>(mm2px(Vec(4.214, 107.809)), module, Buffers::IN_INPUT + 9));

    addOutput(createOutput<PJ301MPort>(mm2px(Vec(28.214, 17.81)), module, Buffers::OUT_OUTPUT + 0));
    addOutput(createOutput<PJ301MPort>(mm2px(Vec(28.214, 27.809)), module, Buffers::OUT_OUTPUT + 1));
    addOutput(createOutput<PJ301MPort>(mm2px(Vec(28.214, 37.809)), module, Buffers::OUT_OUTPUT + 2));
    addOutput(createOutput<PJ301MPort>(mm2px(Vec(28.214, 47.81)), module, Buffers::OUT_OUTPUT + 3));
    addOutput(createOutput<PJ301MPort>(mm2px(Vec(28.214, 57.809)), module, Buffers::OUT_OUTPUT + 4));
    addOutput(createOutput<PJ301MPort>(mm2px(Vec(28.214, 67.809)), module, Buffers::OUT_OUTPUT + 5));
    addOutput(createOutput<PJ301MPort>(mm2px(Vec(28.214, 77.81)), module, Buffers::OUT_OUTPUT + 6));
    addOutput(createOutput<PJ301MPort>(mm2px(Vec(28.214, 87.81)), module, Buffers::OUT_OUTPUT + 7));
    addOutput(createOutput<PJ301MPort>(mm2px(Vec(28.214, 97.809)), module, Buffers::OUT_OUTPUT + 8));
    addOutput(createOutput<PJ301MPort>(mm2px(Vec(28.214, 107.809)), module, Buffers::OUT_OUTPUT + 9));
}

Model *modelBuffers = createModel<Buffers, BuffersWidget>("Buffers");