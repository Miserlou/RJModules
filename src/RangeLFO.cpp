#include "RJModules.hpp"

#include "common.hpp"
#include <iostream>
#include <cmath>
#include <sstream>
#include <iomanip>

struct LowFrequencyOscillatorr {
    float phase = 0.0;
    float pw = 0.5;
    float freq = 1.0;
    bool offset = false;
    bool invert = false;
    dsp::SchmittTrigger resetTrigger;
    LowFrequencyOscillatorr() {}
    void setPitch(float pitch) {
        pitch = fminf(pitch, 8.0);
        freq = powf(2.0, pitch);
    }
    void setPulseWidth(float pw_) {
        const float pwMin = 0.01;
        pw = clamp(pw_, pwMin, 1.0 - pwMin);
    }
    void setReset(float reset) {
        if (resetTrigger.process(reset)) {
            phase = 0.0;
        }
    }
    void step(float dt) {
        float deltaPhase = fminf(freq * dt, 0.5);
        phase += deltaPhase;
        if (phase >= 1.0)
            phase -= 1.0;
    }
    float sin() {
        if (offset)
            return 1.0 - cosf(2*M_PI * phase) * (invert ? -1.0 : 1.0);
        else
            return sinf(2*M_PI * phase) * (invert ? -1.0 : 1.0);
    }
    float tri(float x) {
        return 4.0 * fabsf(x - roundf(x));
    }
    float tri() {
        if (offset)
            return tri(invert ? phase - 0.5 : phase);
        else
            return -1.0 + tri(invert ? phase - 0.25 : phase - 0.75);
    }
    float saw(float x) {
        return 2.0 * (x - roundf(x));
    }
    float saw() {
        if (offset)
            return invert ? 2.0 * (1.0 - phase) : 2.0 * phase;
        else
            return saw(phase) * (invert ? -1.0 : 1.0);
    }
    float sqr() {
        float sqr = (phase < pw) ^ invert ? 1.0 : -1.0;
        return offset ? sqr + 1.0 : sqr;
    }
    float light() {
        return sinf(2*M_PI * phase);
    }
};


/*
Display
*/


struct SmallIntegerDisplayWidgeterer : TransparentWidget {

  float *value;

  void draw(NVGcontext *vg) override
  {
    // Background
    NVGcolor backgroundColor = nvgRGB(0xC0, 0xC0, 0xC0);
    nvgBeginPath(vg);
    nvgRoundedRect(vg, 0.0, 0.0, box.size.x, box.size.y, 4.0);
    nvgFillColor(vg, backgroundColor);
    nvgFill(vg);

    // text
    std::shared_ptr<Font> font = APP->window->loadFont(asset::plugin(pluginInstance, "res/Segment7Standard.ttf"));
    if (font) {
    nvgFontSize(vg, 16);
    nvgFontFaceId(vg, font->handle);
    nvgTextLetterSpacing(vg, 0.5);
    }

    std::stringstream to_display;
    to_display = format4display(*value);

    Vec textPos = Vec(8.0f, 33.0f);
    NVGcolor textColor = nvgRGB(0x00, 0x00, 0x00);
    nvgFillColor(vg, textColor);
    nvgText(vg, textPos.x, textPos.y, to_display.str().substr(0, 4).c_str(), NULL);

    nvgFontSize(vg, 8);
    textPos = Vec(1.0f, (*value<0?28.0f:32.0f));
    nvgText(vg, textPos.x, textPos.y, (*value<0?"-":"+"), NULL);

  }
};


/*
Widget
*/

struct RangeLFO : Module {
    enum ParamIds {
        OFFSET_PARAM,
        INVERT_PARAM,
        FREQ_PARAM,
        FM1_PARAM,
        FM2_PARAM,
        PW_PARAM,
        PWM_PARAM,
        CH1_PARAM,
        CH2_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        FM1_INPUT,
        FM2_INPUT,
        RESET_INPUT,
        PW_INPUT,
        RATE_CV_INPUT,
        FROM_CV_INPUT,
        TO_CV_INPUT,
        NUM_INPUTS
    };
    enum OutputIds {
        SIN_OUTPUT,
        TRI_OUTPUT,
        SAW_OUTPUT,
        SQR_OUTPUT,
        NUM_OUTPUTS
    };
    enum LightIds {
        PHASE_POS_LIGHT,
        PHASE_NEG_LIGHT,
        NUM_LIGHTS
    };

    LowFrequencyOscillatorr oscillator;

    float display1_val;
    float display2_val;

    RangeLFO() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(RangeLFO::CH1_PARAM, -12.0, 12.0, -12.0, "");
        configParam(RangeLFO::CH2_PARAM, -12.0, 12.0, 12.0, "");
        configParam(RangeLFO::FREQ_PARAM, -8.0, 6.0, -1.0, "");
        configParam(RangeLFO::FM1_PARAM, 0.0, 1.0, 0.0, "");
        configParam(RangeLFO::PW_PARAM, 0.0, 1.0, 0.5, "");
        configParam(RangeLFO::FM2_PARAM, 0.0, 1.0, 0.0, "");
        configParam(RangeLFO::PWM_PARAM, 0.0, 1.0, 0.0, "");
        configParam(RangeLFO::INVERT_PARAM, 0.0, 1.0, 0.0, "");
        configParam(RangeLFO::OFFSET_PARAM, 0.0, 1.0, 0.0, "");
    }
    void step() override;
};


void RangeLFO::step() {

    //display
    display1_val = params[CH1_PARAM].value * clamp(inputs[FROM_CV_INPUT].normalize(10.0f) / 10.0f, -1.0f, 1.0f);
    display2_val = params[CH2_PARAM].value * clamp(inputs[TO_CV_INPUT].normalize(10.0f) / 10.0f, -1.0f, 1.0f);

    float osc_pitch = params[FREQ_PARAM].value + params[FM1_PARAM].value * inputs[FM1_INPUT].value + params[FM2_PARAM].value * inputs[FM2_INPUT].value;
    osc_pitch = osc_pitch * clamp(inputs[RATE_CV_INPUT].normalize(10.0f) / 10.0f, 0.0f, 1.0f);
    oscillator.setPitch(osc_pitch);
    oscillator.setPulseWidth(params[PW_PARAM].value + params[PWM_PARAM].value * inputs[PW_INPUT].value / 10.0);
    oscillator.offset = (params[OFFSET_PARAM].value > 0.0);
    oscillator.invert = (params[INVERT_PARAM].value <= 0.0);
    oscillator.step(1.0 / APP->engine->getSampleRate());
    oscillator.setReset(inputs[RESET_INPUT].value);

    float sin = oscillator.sin();
    float tri = oscillator.tri();
    float saw = oscillator.saw();
    float sqr = oscillator.sqr();

    // new_value = ( (old_value - old_min) / (old_max - old_min) ) * (new_max - new_min) + new_min
    float sin_output = ( (sin - (-1)) / (1 - (-1)) ) * (display2_val - display1_val) + display1_val;
    float tri_output = ( (tri - (-1)) / (1 - (-1)) ) * (display2_val - display1_val) + display1_val;
    float saw_output = ( (saw - (-1)) / (1 - (-1)) ) * (display2_val - display1_val) + display1_val;
    float sqr_output = ( (sqr - (-1)) / (1 - (-1)) ) * (display2_val - display1_val) + display1_val;

    outputs[SIN_OUTPUT].value = sin_output;
    outputs[TRI_OUTPUT].value = tri_output;
    outputs[SAW_OUTPUT].value = saw_output;
    outputs[SQR_OUTPUT].value = sqr_output;

    lights[PHASE_POS_LIGHT].setBrightnessSmooth(fmaxf(0.0, oscillator.light()));
    lights[PHASE_NEG_LIGHT].setBrightnessSmooth(fmaxf(0.0, -oscillator.light()));
}


struct RangeLFOWidget: ModuleWidget {
    RangeLFOWidget(RangeLFO *module);
};

RangeLFOWidget::RangeLFOWidget(RangeLFO *module) {
		setModule(module);
    box.size = Vec(15*10, 380);

    {
        SVGPanel *panel = new SVGPanel();
        panel->box.size = box.size;
        panel->setBackground(APP->window->loadSvg(asset::plugin(pluginInstance, "res/RangeLFO.svg")));
        addChild(panel);
    }

    addChild(createWidget<ScrewSilver>(Vec(15, 0)));
    addChild(createWidget<ScrewSilver>(Vec(box.size.x-30, 0)));
    addChild(createWidget<ScrewSilver>(Vec(15, 365)));
    addChild(createWidget<ScrewSilver>(Vec(box.size.x-30, 365)));

    if(module != NULL){
        SmallIntegerDisplayWidgeterer *display = new SmallIntegerDisplayWidgeterer();
        display->box.pos = Vec(23, 160);
        display->box.size = Vec(50, 40);
        display->value = &module->display1_val;
        addChild(display);

        addParam(createParam<RoundBlackKnob>(Vec(28, 205), module, RangeLFO::CH1_PARAM));
        addInput(createInput<PJ301MPort>(Vec(5, 235), module, RangeLFO::FROM_CV_INPUT));

        SmallIntegerDisplayWidgeterer *display2 = new SmallIntegerDisplayWidgeterer();
        display2->box.pos = Vec(83, 160);
        display2->box.size = Vec(50, 40);
        display2->value = &module->display2_val;
        addChild(display2);

        addParam(createParam<RoundBlackKnob>(Vec(88, 205), module, RangeLFO::CH2_PARAM));
        addInput(createInput<PJ301MPort>(Vec(62, 235), module, RangeLFO::TO_CV_INPUT));
    }

    addParam(createParam<RoundHugeBlackKnob>(Vec(47, 61), module, RangeLFO::FREQ_PARAM));
    // addParam(createParam<RoundBlackKnob>(Vec(23, 143), module, RangeLFO::FM1_PARAM));
    // addParam(createParam<RoundBlackKnob>(Vec(91, 143), module, RangeLFO::PW_PARAM));
    // addParam(createParam<RoundBlackKnob>(Vec(23, 208), module, RangeLFO::FM2_PARAM));
    // addParam(createParam<RoundBlackKnob>(Vec(91, 208), module, RangeLFO::PWM_PARAM));
    addInput(createInput<PJ301MPort>(Vec(22, 100), module, RangeLFO::RATE_CV_INPUT));

    addInput(createInput<PJ301MPort>(Vec(11, 276), module, RangeLFO::FM1_INPUT));
    addInput(createInput<PJ301MPort>(Vec(45, 276), module, RangeLFO::RESET_INPUT));
    // addInput(createInput<PJ301MPort>(Vec(80, 276), module, RangeLFO::RESET_INPUT));
    // addInput(createInput<PJ301MPort>(Vec(114, 276), module, RangeLFO::PW_INPUT));
    addParam(createParam<CKSS>(Vec(85, 276), module, RangeLFO::INVERT_PARAM));
    //addParam(createParam<CKSS>(Vec(119, 276), module, RangeLFO::OFFSET_PARAM));

    addOutput(createOutput<PJ301MPort>(Vec(11, 320), module, RangeLFO::SIN_OUTPUT));
    addOutput(createOutput<PJ301MPort>(Vec(45, 320), module, RangeLFO::TRI_OUTPUT));
    addOutput(createOutput<PJ301MPort>(Vec(80, 320), module, RangeLFO::SAW_OUTPUT));
    addOutput(createOutput<PJ301MPort>(Vec(114, 320), module, RangeLFO::SQR_OUTPUT));

    addChild(createLight<SmallLight<GreenRedLight>>(Vec(99, 60), module, RangeLFO::PHASE_POS_LIGHT));
}

Model *modelRangeLFO = createModel<RangeLFO, RangeLFOWidget>("RangeLFO");
