/*
Slackback!
*/

#include "RJModules.hpp"
#include "common.hpp"
#include <iostream>
#include <cmath>
#include <sstream>
#include <iomanip>
#include <unistd.h>
#include <mutex>

using namespace std;
#define HISTORY_SIZE (1<<21)

struct SlapbackRoundSmallBlackKnob : RoundSmallBlackKnob
{
    SlapbackRoundSmallBlackKnob()
    {
        setSVG(SVG::load(assetPlugin(pluginInstance, "res/KTFRoundSmallBlackKnob.svg")));
    }
};

struct Slapback : Module {
    enum ParamIds {
        TIME_PARAM,
        TIME_PARAM_2,
        NUM_PARAMS
    };
    enum InputIds {
        IN_INPUT,
        IN_INPUT_2,
        TIME_INPUT,
        TIME_INPUT_2,

        NUM_INPUTS
    };
    enum OutputIds {
        OUT_OUTPUT,
        OUT_OUTPUT_2,
        NUM_OUTPUTS
    };
    enum LightIds {
        NUM_LIGHTS
};

    /* Delays - Left*/
    dsp::RCFilter lowpassFilter;
    dsp::RCFilter highpassFilter;

    dsp::DoubleRingBuffer<float, HISTORY_SIZE> historyBuffer;
    dsp::DoubleRingBuffer<float, 16> outBuffer;

    dsp::SampleRateConverter<1> src;

    dsp::SchmittTrigger bypass_button_trig;
    dsp::SchmittTrigger bypass_cv_trig;

    int lcd_tempo = 0;
    bool fx_bypass = false;
    float lastWet = 0.0f;

    float fade_in_fx = 0.0f;
    float fade_in_dry = 0.0f;
    float fade_out_fx = 1.0f;
    float fade_out_dry = 1.0f;
    const float fade_speed = 0.001f;

    /* Delays - Right */
    dsp::RCFilter lowpassFilter_right_2;
    dsp::RCFilter highpassFilter_right_2;

    dsp::DoubleRingBuffer<float, HISTORY_SIZE> historyBuffer_2;
    dsp::DoubleRingBuffer<float, 16> outBuffer_2;

    dsp::SampleRateConverter<1> src_2;
    float lastWet_2 = 0.0f;
    float fade_in_fx_2 = 0.0f;
    float fade_in_dry_2 = 0.0f;
    float fade_out_fx_2 = 1.0f;
    float fade_out_dry_2 = 1.0f;
    const float fade_speed_2 = 0.001f;
    float thisWet_2 = 0.0f;

    /* Input Caching */
    int param_counter = 7;
    float TIME_PARAM_value;
    float TIME_PARAM_2_value;

    Slapback() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

        // 33 to 130ms
        configParam(Slapback::TIME_PARAM, 33.0, 130.0, 33.0, "Delay Time ms");
        configParam(Slapback::TIME_PARAM_2, 33.0, 130.0, 33.0, "Delay Time ms 2");

    }

    void process(const ProcessArgs &args) override {

    /* Get the values every 8 steps */
    if(param_counter>=7){
        TIME_PARAM_value = params[TIME_PARAM].getValue();
        TIME_PARAM_2_value = params[TIME_PARAM_2].getValue();;
        param_counter = 0;
    } else{
        param_counter++;
    }

    /* Left Channel */
    // Get input to delay block
    float adjusted = TIME_PARAM_value * clamp(inputs[TIME_INPUT].normalize(10.0f) / 10.0f, 0.0f, 1.0f);
    float selected_value = rescale(adjusted, 0.0f, 10000.0f, 0.0f, 10.0f);
    float signal_input = inputs[IN_INPUT].getVoltage();
    float dry = signal_input;
    float delay = clamp(selected_value, 0.0f, 10.0f);
    float index = delay * args.sampleRate;

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
        float inSR = args.sampleRate;
        float outSR = ratio * inSR;
        int inFrames = fmin(historyBuffer.size(), 16);
        int outFrames = outBuffer.capacity();
        src.setRates(inSR, outSR);
        src.process((const dsp::Frame<1>*)historyBuffer.startData(), &inFrames, (dsp::Frame<1>*)outBuffer.endData(), &outFrames);
        historyBuffer.startIncr(inFrames);
        outBuffer.endIncr(outFrames);
    }

    float out;
    float mix;
    float wet = 0.0f;
    if (!outBuffer.empty()) {
        wet = outBuffer.shift();
    }
    lastWet = wet;
    mix = 1.0f;
    out = crossfade(signal_input, wet, mix);
    fade_in_fx += fade_speed;
    if ( fade_in_fx > 1.0f ) {
        fade_in_fx = 1.0f;
    }
    fade_out_dry -= fade_speed;
    if ( fade_out_dry < 0.0f ) {
        fade_out_dry = 0.0f;
    }
    float left_output = signal_input + ( out * fade_in_fx ) ;
    outputs[OUT_OUTPUT].setVoltage(left_output);

    /* Right Channel */
    // Get input to delay block
    float adjusted_2 = TIME_PARAM_2_value * clamp(inputs[TIME_INPUT_2].normalize(10.0f) / 10.0f, 0.0f, 1.0f);
    float selected_value_2 = rescale(adjusted_2, 0.0f, 10000.0f, 0.0f, 10.0f);
    float signal_input_2 = inputs[IN_INPUT_2].getVoltage();
    float dry_2 = signal_input_2;
    float delay_2 = clamp(selected_value_2, 0.0f, 10.0f);
    float index_2 = delay_2 * args.sampleRate;

    // Push dry sample into history buffer
    if (!historyBuffer_2.full()) {
        historyBuffer_2.push(dry_2);
    }
    // How many samples do we need consume to catch up?
    float consume_2 = index_2 - historyBuffer_2.size();

    if (outBuffer_2.empty()) {
        double ratio_2 = 1.0;
        if (consume_2 <= -16)
            ratio_2 = 0.5;
        else if (consume_2 >= 16)
            ratio_2 = 2.0;
        float inSR_2 = args.sampleRate;
        float outSR_2 = ratio_2 * inSR_2;
        int inFrames_2 = fmin(historyBuffer_2.size(), 16);
        int outFrames_2 = outBuffer_2.capacity();
        src_2.setRates(inSR_2, outSR_2);
        src_2.process((const dsp::Frame<1>*)historyBuffer_2.startData(), &inFrames_2, (dsp::Frame<1>*)outBuffer_2.endData(), &outFrames_2);
        historyBuffer_2.startIncr(inFrames_2);
        outBuffer_2.endIncr(outFrames_2);
    }

    float out_2;
    float mix_2;
    float wet_2 = 0.0f;
    if (!outBuffer_2.empty()) {
        wet_2 = outBuffer_2.shift();
    }
    lastWet_2 = wet_2;
    mix_2 = 1.0f;
    out_2 = crossfade(signal_input_2, wet_2, mix_2);
    fade_in_fx_2 += fade_speed_2;
    if ( fade_in_fx_2 > 1.0f ) {
        fade_in_fx_2 = 1.0f;
    }
    fade_out_dry_2 -= fade_speed_2;
    if ( fade_out_dry_2 < 0.0f ) {
        fade_out_dry_2 = 0.0f;
    }
    float right_output = signal_input_2 + ( out_2 * fade_in_fx_2 ) ;
    outputs[OUT_OUTPUT_2].setVoltage(right_output);


    }
};

struct SlapbackWidget : ModuleWidget {
    SlapbackWidget(Slapback *module) {
		setModule(module);
        setPanel(SVG::load(assetPlugin(pluginInstance, "res/Slapback.svg")));

        int ONE = -4;
        addParam(createParam<SlapbackRoundSmallBlackKnob>(mm2px(Vec(3.5, 38.9593 + ONE)), module, Slapback::TIME_PARAM));
        addInput(createPort<PJ301MPort>(mm2px(Vec(3.51398, 48.74977 + ONE)), PortWidget::INPUT, module, Slapback::TIME_INPUT));
        addInput(createPort<PJ301MPort>(mm2px(Vec(3.51398, 62.3 + ONE)), PortWidget::INPUT, module, Slapback::IN_INPUT));
        addOutput(createPort<PJ301MPort>(mm2px(Vec(3.51398, 73.3 + ONE)), PortWidget::OUTPUT, module, Slapback::OUT_OUTPUT));

        int TWO = 45;
        addParam(createParam<SlapbackRoundSmallBlackKnob>(mm2px(Vec(3.5, 38.9593 + TWO)), module, Slapback::TIME_PARAM_2));
        addInput(createPort<PJ301MPort>(mm2px(Vec(3.51398, 48.74977 + TWO)), PortWidget::INPUT, module, Slapback::TIME_INPUT_2));
        addInput(createPort<PJ301MPort>(mm2px(Vec(3.51398, 62.3 + TWO)), PortWidget::INPUT, module, Slapback::IN_INPUT_2));
        addOutput(createPort<PJ301MPort>(mm2px(Vec(3.51398, 73.3 + TWO)), PortWidget::OUTPUT, module, Slapback::OUT_OUTPUT_2));


    }
};


Model *modelSlapback = createModel<Slapback, SlapbackWidget>("Slapback");
