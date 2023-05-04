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

/*
Display
*/

struct PingPongSmallStringDisplayWidget : TransparentWidget {

  std::string *value;

  void draw(NVGcontext *vg) override
  {

    // Shadow
    NVGcolor backgroundColorS = nvgRGB(0xA0, 0xA0, 0xA0);
    nvgBeginPath(vg);
    nvgRoundedRect(vg, 0.0, 0.0, box.size.x, box.size.y + 2.0, 4.0);
    nvgFillColor(vg, backgroundColorS);
    nvgFill(vg);

    // Background
    NVGcolor backgroundColor = nvgRGB(0xC0, 0xC0, 0xC0);
    nvgBeginPath(vg);
    nvgRoundedRect(vg, 0.0, 0.0, box.size.x, box.size.y, 4.0);
    nvgFillColor(vg, backgroundColor);
    nvgFill(vg);

    // text
    std::shared_ptr<Font> font = APP->window->loadFont(asset::plugin(pluginInstance, "res/Pokemon.ttf"));
    if (font) {
    nvgFontSize(vg, 20);
    nvgFontFaceId(vg, font->handle);
    nvgTextLetterSpacing(vg, 0.4);
    }

    std::stringstream to_display;
    to_display << std::setw(3) << *value;

    Vec textPos = Vec(12.0f, 28.0f);
    NVGcolor textColor = nvgRGB(0x00, 0x00, 0x00);
    nvgFillColor(vg, textColor);
    nvgText(vg, textPos.x, textPos.y, to_display.str().c_str(), NULL);
  }
};

struct PingPongRoundLargeBlackKnob : RoundLargeBlackKnob
{
    PingPongRoundLargeBlackKnob()
    {
        setSVG(APP->window->loadSvg(asset::plugin(pluginInstance, "res/KTFRoundHugeBlackKnob.svg")));
    }
};

struct PingPongRoundBlackSnapKnob : RoundBlackKnob
{
    PingPongRoundBlackSnapKnob()
    {
        setSVG(APP->window->loadSvg(asset::plugin(pluginInstance, "res/KTFRoundLargeBlackKnob.svg")));
        minAngle = -0.83 * M_PI;
        maxAngle = 0.83 * M_PI;
        snap = true;
    }
};

/*
Widget
*/

struct PingPong : Module {
    enum ParamIds {
        RATE_PARAM,
        FEEDBACK_PARAM,
        NUDGE_PARAM,
        COLOR_PARAM,
        MIX_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        IN_INPUT,
        CLOCK_INPUT,

        TIME_INPUT,
        FEEDBACK_INPUT,
        COLOR_INPUT,
        COLOR_INPUT_RIGHT,

        COLOR_RETURN,
        COLOR_RETURN_RIGHT,

        MIX_INPUT,
        BYPASS_CV_INPUT,

        NUM_INPUTS
    };
    enum OutputIds {
        LEFT_OUTPUT,
        RIGHT_OUTPUT,

        COLOR_SEND,
        COLOR_SEND_RIGHT,

        NUM_OUTPUTS
    };
    enum LightIds {
        NUM_LIGHTS
    };

    // Display
    std::string rate_display = "1/4";

    // From AS + Koral
    // BPM detector variables
    bool inMemory = false;
    bool beatLock = false;
    float beatTime = 0.0f;
    int beatCount = 0;
    int beatCountMemory = 0;
    float beatOld = 0.0f;
    std::string tempo = "---";
    dsp::SchmittTrigger clockTrigger;
    dsp::PulseGenerator LightPulse;
    bool pulse = false;

    // Calculator variables
    float bpm = 120;
    float last_bpm = 0;
    float millisecs = 60000;
    float mult = 1000;
    float millisecondsPerBeat;
    float millisecondsPerMeasure;
    float bar = 1.0f;
    float secondsPerBeat = 0.0f;
    float secondsPerMeasure = 0.0f;

    //ms variables
    float half_note_d = 1.0f;
    float half_note = 1.0f;
    float half_note_t =1.0f;

    float qt_note_d = 1.0f;
    float qt_note = 1.0f;
    float qt_note_t = 1.0f;

    float eight_note_d = 1.0f;
    float eight_note =1.0f;
    float eight_note_t = 1.0f;

    float sixth_note_d =1.0f;
    float sixth_note = 1.0f;
    float sixth_note_t = 1.0f;

    float trth_note_d = 1.0f;
    float trth_note = 1.0f;
    float trth_note_t = 1.0f;

    std::string selections[16] = {"1", "1/2d", "1/2", "1/2t", "1/4d", "1/4", "1/4t", "1/8d", "1/8", "1/8t", "1/16d", "1/16", "1/16t", "1/32d", "1/32", "1/32t"};

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
    dsp::RCFilter lowpassFilter_right;
    dsp::RCFilter highpassFilter_right;

    dsp::DoubleRingBuffer<float, HISTORY_SIZE> historyBuffer_right;
    dsp::DoubleRingBuffer<float, 16> outBuffer_right;

    dsp::SampleRateConverter<1> src_right;
    float lastWet_right = 0.0f;
    float fade_in_fx_right = 0.0f;
    float fade_in_dry_right = 0.0f;
    float fade_out_fx_right = 1.0f;
    float fade_out_dry_right = 1.0f;
    const float fade_speed_right = 0.001f;
    float thisWet_right = 0.0f;

    /* Menu Settings*/
    int feedback_mode_index = 0;
    int poly_mode_index = 0;
    int last_poly_mode_index = 0;

    /* Input Caching */
    int param_counter = 7;
    float FEEDBACK_PARAM_value;
    float MIX_PARAM_value;
    float RATE_PARAM_value;
    float NUDGE_PARAM_value;
    float COLOR_PARAM_value;

    /* */
    void calculateValues(float bpm){
        millisecondsPerBeat = millisecs/bpm;
        millisecondsPerMeasure = millisecondsPerBeat * 4;
        secondsPerBeat = 60 / bpm;
        secondsPerMeasure = secondsPerBeat * 4;
        bar = (millisecondsPerMeasure);
        half_note_d = ( millisecondsPerBeat * 3 );
        half_note = ( millisecondsPerBeat * 2 );
        half_note_t = ( half_note * 2 / 3 );
        qt_note_d = ( millisecondsPerBeat / 2 ) * 3;
        qt_note = millisecondsPerBeat;
        qt_note_t =  half_note  / 3;
        eight_note_d = ( millisecondsPerBeat / 4 ) * 3;
        eight_note = millisecondsPerBeat / 2;
        eight_note_t = millisecondsPerBeat / 3;
        sixth_note = millisecondsPerBeat / 4;
        sixth_note_d = ( sixth_note ) * 1.5;
        sixth_note_t = millisecondsPerBeat / 6;
        trth_note = millisecondsPerBeat / 8;
        trth_note_d = ( trth_note ) * 1.5;
        trth_note_t = trth_note * 2 / 3;
        last_bpm = bpm;
    }

    void refreshDetector() {
        inMemory = false;
        beatLock = false;
        beatTime = 0.0f;
        beatCount = 0;
        beatCountMemory = 0;
        beatOld = 0.0f;
        tempo = "---";
        pulse = false;
    }

    void onReset() override {
        refreshDetector();
    }

    void onInitialize() {
        refreshDetector();
    }

    PingPong() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(PingPong::RATE_PARAM, 0, 15, 5, "Rate");
        configParam(PingPong::FEEDBACK_PARAM, 0.0f, 1.0f, 0.5f, "Feedback");
        configParam(PingPong::NUDGE_PARAM, -.025f, .025f, 0.0f, "Nudge");
        configParam(PingPong::COLOR_PARAM, 0.0f, 1.0f, 0.5f, "Color");
        configParam(PingPong::MIX_PARAM, 0.0f, 1.0f, 1.0f, "Mix");
    }

    void process(const ProcessArgs &args) override {

    /* Get the values every 8 steps */
    if(param_counter>=7){
        FEEDBACK_PARAM_value = params[FEEDBACK_PARAM].getValue();
        MIX_PARAM_value = params[MIX_PARAM].getValue();;
        RATE_PARAM_value = params[RATE_PARAM].getValue();
        NUDGE_PARAM_value = params[NUDGE_PARAM].getValue();
        COLOR_PARAM_value = params[COLOR_PARAM].getValue();
        param_counter = 0;
    } else{
        param_counter++;
    }

    /* Clock Detector */
    float deltaTime = args.sampleTime;
    if ( inputs[CLOCK_INPUT].isConnected() ) {

        float clockInput = inputs[CLOCK_INPUT].getVoltage();
        //A rising slope
        if ( ( clockTrigger.process( inputs[CLOCK_INPUT].getVoltage() ) ) && !inMemory ) {
          beatCount++;
          if(!beatLock){
            LightPulse.trigger( 0.1f );
          }
          inMemory = true;

          //BPM is locked
          if ( beatCount == 2 ) {
            beatLock = true;
            beatOld = beatTime;
          }
          //BPM is lost
          if ( beatCount > 2 ) {
            if ( fabs( beatOld - beatTime ) > 0.0005f ) {
              beatLock = false;
              beatCount = 0;
              tempo = "---";
            }
          }
          beatTime = 0;
        }

        //Falling slope
        if ( clockInput <= 0 && inMemory ) {
            inMemory = false;
        }
        //When BPM is locked
        if ( beatLock ) {
          bpm = (int)round( 60 / beatOld );
          tempo = std::to_string( (int)round(bpm) );
          if(bpm!=last_bpm){
            if(bpm<999){
              calculateValues(bpm);
            }else{
              tempo = "OOR";
            }
          }
        } //end of beatLock routine

        beatTime += deltaTime;

      //when beat is lost
      if ( beatTime > 2 ) {
        beatLock = false;
        beatCount = 0;
        tempo = "---";
      }
      beatCountMemory = beatCount;

    } else {
      beatLock = false;
      beatCount = 0;
      //tempo = "OFF";
      //caluculate with knob value instead of bmp detector value
      bpm = 134;
      if (bpm<30){
        bpm = 30;
      }
      bpm = (int)round(bpm);
      tempo = std::to_string( (int)round(bpm) );
      if(bpm!=last_bpm){
          calculateValues(bpm);
      }
    }

    /* Rate Detector */
    int rate = RATE_PARAM_value;
    rate_display = selections[rate];
    float selected_value = 0.0f;
    switch(rate) {
       case 0:
          selected_value = bar;
          break;
       case 1:
          selected_value = half_note_d;
          break;
       case 2:
          selected_value = half_note;
          break;
       case 3:
          selected_value = half_note_t;
          break;
       case 4:
          selected_value = qt_note_d;
          break;
       case 5:
          selected_value = qt_note;
          break;
       case 6:
          selected_value = qt_note_t;
          break;
       case 7:
          selected_value = eight_note_d;
          break;
       case 8:
          selected_value = eight_note;
          break;
       case 9:
          selected_value = eight_note_t;
          break;
       case 10:
          selected_value = sixth_note_d;
          break;
       case 11:
          selected_value = sixth_note;
          break;
       case 12:
          selected_value = sixth_note_t;
          break;
       case 13:
          selected_value = trth_note_d;
          break;
       case 14:
          selected_value = trth_note;
          break;
       case 15:
          selected_value = trth_note_t;
          break;
    }
    selected_value = rescale(selected_value, 0.0f, 10000.0f, 0.0f, 10.0f);
    //float nudge = rescale(NUDGE_PARAM_value, -7500.0f, 7500.0f, -1.0f, 1.0f);
    selected_value = selected_value + NUDGE_PARAM_value;

    /* Left Channel */

    // Get input to delay block
    float signal_input = inputs[IN_INPUT].getVoltage();
    float feedback = clamp(FEEDBACK_PARAM_value + inputs[FEEDBACK_INPUT].getVoltage() / 10.0f, 0.0f, 1.0f);
    if(feedback_mode_index != 0){
        feedback = feedback * 2;
    }
    float dry = signal_input + (thisWet_right * feedback);

    // Compute delay time in seconds
    //float delay = 1e-3 * powf(10.0 / 1e-3, clampf(params[TIME_PARAM].getValue() + inputs[TIME_INPUT].getVoltage() / 10.0, 0.0, 1.0));
    //float delay = clamp(params[TIME_PARAM].getValue() + inputs[TIME_INPUT].getVoltage(), 0.0f, 10.0f);
    float delay = clamp(selected_value, 0.0f, 10.0f);
    // std::cout << rate << " rate \n";
    // std::cout << selected_value << " sel\n";
    // std::cout << delay << "\n";

    //LCD display tempo  - show value as ms
    lcd_tempo = std::round(delay*1000);
    // Number of delay samples
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

    if (outputs[COLOR_SEND].isConnected() == false) {
        //internal color
        // Apply color to delay wet output
        float color = clamp(COLOR_PARAM_value + inputs[COLOR_INPUT].getVoltage() / 10.0f, 0.0f, 1.0f);
        float lowpassFreq = 10000.0f * powf(10.0f, clamp(2.0*color, 0.0f, 1.0f));
        lowpassFilter.setCutoff(lowpassFreq / args.sampleRate);
        lowpassFilter.process(wet);
        wet = lowpassFilter.lowpass();
        float highpassFreq = 10.0f * powf(100.0f, clamp(2.0f*color - 1.0f, 0.0f, 1.0f));
        highpassFilter.setCutoff(highpassFreq / args.sampleRate);
        highpassFilter.process(wet);
        wet = highpassFilter.highpass();
        //lastWet = wet;
    }else {
        //external color, to filter the wet delay signal outside of the module, or to feed another module
        outputs[COLOR_SEND].setVoltage(wet);
        wet = inputs[COLOR_RETURN].getVoltage();
    }
    lastWet = wet;
    mix = clamp(MIX_PARAM_value + inputs[MIX_INPUT].getVoltage() / 10.0f, 0.0f, 1.0f);
    out = crossfade(signal_input, wet, mix);
    fade_in_fx += fade_speed;
    if ( fade_in_fx > 1.0f ) {
        fade_in_fx = 1.0f;
    }
    fade_out_dry -= fade_speed;
    if ( fade_out_dry < 0.0f ) {
        fade_out_dry = 0.0f;
    }
    float left_output = ( signal_input * fade_out_dry ) + ( out * fade_in_fx ) ;

    /* Right Channel! */
    // Get input to delay block
    float signal_input_right = left_output;
    float feedback_right = clamp(FEEDBACK_PARAM_value + inputs[FEEDBACK_INPUT].getVoltage() / 10.0f, 0.0f, 1.0f);
    float dry_right = signal_input_right;// + (lastWet * feedback_right);

    // Number of delay samples
    float index_right = delay * args.sampleRate;

    // Push dry sample into history buffer
    if (!historyBuffer_right.full()) {
        historyBuffer_right.push(dry_right);
    }

    // How many samples do we need consume to catch up?
    float consume_right = index_right - historyBuffer_right.size();

    if (outBuffer_right.empty()) {
        double ratio_right = 1.0;
        if (consume_right <= -16)
            ratio_right = 0.5;
        else if (consume_right >= 16)
            ratio_right = 2.0;

        float inSR_right = args.sampleRate;
        float outSR_right = ratio_right * inSR_right;

        int inFrames_right = fmin(historyBuffer_right.size(), 16);
        int outFrames_right = outBuffer_right.capacity();
        src_right.setRates(inSR_right, outSR_right);
        src_right.process((const dsp::Frame<1>*)historyBuffer_right.startData(), &inFrames_right, (dsp::Frame<1>*)outBuffer_right.endData(), &outFrames_right);
        historyBuffer_right.startIncr(inFrames_right);
        outBuffer_right.endIncr(outFrames_right);
    }

    float out_right;
    float mix_right;
    float wet_right = 0.0f;
    if (!outBuffer_right.empty()) {
        wet_right = outBuffer_right.shift();
    }

    if (outputs[COLOR_SEND_RIGHT].isConnected() == false) {
        //internal color
        // Apply color to delay wet output
        float color_right = clamp(COLOR_PARAM_value + inputs[COLOR_INPUT_RIGHT].getVoltage() / 10.0f, 0.0f, 1.0f);
        float lowpassFreq_right = 10000.0f * powf(10.0f, clamp(2.0*color_right, 0.0f, 1.0f));
        lowpassFilter_right.setCutoff(lowpassFreq_right / args.sampleRate);
        lowpassFilter_right.process(wet_right);
        wet_right = lowpassFilter_right.lowpass();
        float highpassFreq_right = 10.0f * powf(100.0f, clamp(2.0f*color_right - 1.0f, 0.0f, 1.0f));
        highpassFilter_right.setCutoff(highpassFreq_right / args.sampleRate);
        highpassFilter_right.process(wet_right);
        wet_right = highpassFilter_right.highpass();
        //lastWet = wet;
    }else {
        //external color, to filter the wet delay signal outside of the module, or to feed another module
        outputs[COLOR_SEND_RIGHT].setVoltage(wet_right);
        wet_right = inputs[COLOR_RETURN_RIGHT].getVoltage();
    }
    lastWet_right = wet_right;
    mix_right = clamp(MIX_PARAM_value + inputs[MIX_INPUT].getVoltage() / 10.0f, 0.0f, 1.0f);
    out_right = crossfade(signal_input_right, wet_right, mix_right);
    fade_in_fx_right += fade_speed_right;
    if ( fade_in_fx_right > 1.0f ) {
        fade_in_fx_right = 1.0f;
    }
    fade_out_dry_right -= fade_speed_right;
    if ( fade_out_dry_right < 0.0f ) {
        fade_out_dry_right = 0.0f;
    }
    thisWet_right = ( signal_input_right * fade_out_dry_right ) + ( out_right * fade_in_fx_right );
    outputs[RIGHT_OUTPUT].setVoltage(thisWet_right);

    if(poly_mode_index != 0){
        if(poly_mode_index != last_poly_mode_index){
            outputs[LEFT_OUTPUT].setChannels(2);
            last_poly_mode_index = poly_mode_index;
        }
        outputs[LEFT_OUTPUT].setVoltage(left_output, 0);
        outputs[LEFT_OUTPUT].setVoltage(thisWet_right, 1);
    } else{
        if(poly_mode_index != last_poly_mode_index){
            outputs[LEFT_OUTPUT].setChannels(1);
            last_poly_mode_index = poly_mode_index;
        }
        outputs[LEFT_OUTPUT].setVoltage(left_output);
    }

    }
};

struct PingPongWidget : ModuleWidget {
  PingPongWidget(PingPong *module) {
    setModule(module);
    box.size = Vec(15*10, 380);

    SVGPanel *panel = new SVGPanel();
    panel->box.size = box.size;
    panel->setBackground(APP->window->loadSvg(asset::plugin(pluginInstance, "res/PingPong.svg")));
    addChild(panel);

    // Magnets
    // addChild(createWidget<ScrewSilver>(Vec(15, 0)));
    // addChild(createWidget<ScrewSilver>(Vec(box.size.x-30, 0)));
    // addChild(createWidget<ScrewSilver>(Vec(15, 365)));
    // addChild(createWidget<ScrewSilver>(Vec(box.size.x-30, 365)));

    // Displays
    if(module != NULL){
        PingPongSmallStringDisplayWidget *fileDisplay = new PingPongSmallStringDisplayWidget();
        fileDisplay->box.pos = Vec(20, 50);
        fileDisplay->box.size = Vec(70, 40);
        fileDisplay->value = &module->rate_display;
        addChild(fileDisplay);
    }

    // Knobs
    int LEFT = 14;
    int RIGHT = 65;
    int DIST = 82;
    int BASE = 115;
    addParam(createParam<PingPongRoundBlackSnapKnob>(Vec(100, 50), module, PingPong::RATE_PARAM));
    addParam(createParam<PingPongRoundLargeBlackKnob>(Vec(LEFT, BASE), module, PingPong::FEEDBACK_PARAM));
    addParam(createParam<PingPongRoundLargeBlackKnob>(Vec(LEFT + RIGHT, BASE), module, PingPong::NUDGE_PARAM));
    addParam(createParam<PingPongRoundLargeBlackKnob>(Vec(LEFT, BASE + DIST), module, PingPong::COLOR_PARAM));
    addParam(createParam<PingPongRoundLargeBlackKnob>(Vec(LEFT + RIGHT, BASE + DIST), module, PingPong::MIX_PARAM));

    // Inputs and Knobs
    addOutput(createOutput<PJ301MPort>(Vec(11, 277), module, PingPong::COLOR_SEND));
    addInput(createInput<PJ301MPort>(Vec(45, 277), module, PingPong::COLOR_RETURN));
    addOutput(createOutput<PJ301MPort>(Vec(80, 277), module, PingPong::COLOR_SEND_RIGHT));
    addInput(createInput<PJ301MPort>(Vec(112.5, 277), module, PingPong::COLOR_RETURN_RIGHT));

    addInput(createInput<PJ301MPort>(Vec(11, 320), module, PingPong::IN_INPUT));
    addInput(createInput<PJ301MPort>(Vec(45, 320), module, PingPong::CLOCK_INPUT));
    addOutput(createOutput<PJ301MPort>(Vec(80, 320), module, PingPong::LEFT_OUTPUT));
    addOutput(createOutput<PJ301MPort>(Vec(112.5, 320), module, PingPong::RIGHT_OUTPUT));
    }

    // blah needs getters and setters
    // json_t *toJson() {
    //     json_t *rootJ = ModuleWidget::toJson();
    //     json_object_set_new(rootJ, "feed", json_real(feedback_mode_index));
    //     json_object_set_new(rootJ, "poly", json_real(poly_mode_index));
    //     return rootJ;
    // }

    // void fromJson(json_t *rootJ) {
    //     ModuleWidget::fromJson(rootJ);
    //     json_t *feedJ = json_object_get(rootJ, "feed");
    //     if (feedJ)
    //         feedback_mode_index = json_number_value(feedJ);
    //     json_t *polyJ = json_object_get(rootJ, "poly");
    //     if (polyJ)
    //         poly_mode_index = json_number_value(polyJ);
    // }

    void appendContextMenu(Menu *menu) override
    {
        PingPong *module = dynamic_cast<PingPong *>(this->module);

        struct FeedbackIndexItem : MenuItem
        {
            PingPong *module;
            int index;
            void onAction(const event::Action &e) override
            {
                module->feedback_mode_index = index;
            }
        };

        struct FeedbackItem : MenuItem
        {
            PingPong *module;
            Menu *createChildMenu() override
            {
                Menu *menu = new Menu();
                const std::string feedbackLabels[] = {
                    "0\% - 100\%",
                    "0\% - 200\%"
                };
                for (int i = 0; i < (int)LENGTHOF(feedbackLabels); i++)
                {
                    FeedbackIndexItem *item = createMenuItem<FeedbackIndexItem>(feedbackLabels[i], CHECKMARK(module->feedback_mode_index == i));
                    item->module = module;
                    item->index = i;
                    menu->addChild(item);
                }
                return menu;
            }
        };

        struct PolyIndexItem : MenuItem
        {
            PingPong *module;
            int index;
            void onAction(const event::Action &e) override
            {
                module->poly_mode_index = index;
            }
        };

        struct PolyItem : MenuItem
        {
            PingPong *module;
            Menu *createChildMenu() override
            {
                Menu *menu = new Menu();
                const std::string polyLabels[] = {
                    "Mono Out",
                    "Poly Out"
                };
                for (int i = 0; i < (int)LENGTHOF(polyLabels); i++)
                {
                    PolyIndexItem *item = createMenuItem<PolyIndexItem>(polyLabels[i], CHECKMARK(module->poly_mode_index == i));
                    item->module = module;
                    item->index = i;
                    menu->addChild(item);
                }
                return menu;
            }
        };

        menu->addChild(new MenuEntry);

        FeedbackItem *feedbackItem = createMenuItem<FeedbackItem>("Feedback Mode", ">");
        feedbackItem->module = module;
        menu->addChild(feedbackItem);

        PolyItem *polyItem = createMenuItem<PolyItem>("Poly Mode", ">");
        polyItem->module = module;
        menu->addChild(polyItem);
    }

};

Model *modelPingPong = createModel<PingPong, PingPongWidget>("PingPong");
