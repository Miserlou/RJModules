#include "RJModules.hpp"
#include "dsp/digital.hpp"
#include "plugin.hpp"
#include <iostream>
#include <cmath>
#include <sstream>
#include <iomanip>

using simd::float_4;

// CV
class CV {
public:
    CV (float threshold) {
        this->threshold = threshold;
        this->reset();
        }

        bool newTrigger ( ) {
        // check to see if this is a status change, if so reset the states and return true
        if (this->triggered == true && this->lastTriggered == false) {
          this->lastTriggered = true;

          return true;
        }

        this->lastTriggered = this->triggered;

        return false;
    }

    void update (float current) {
        // set the last value to whatever the current value is
        this->lastValue = current;

        // increase the trigger interval count
        this->triggerIntervalCount++;

        // check the threshold, if it meets or is greater, then we make a change
        if (current >= this->threshold) {
          if (this->triggered == false) {
            this->triggered = true;

            // increment the total number of triggers fired
            this->triggerCount++;

            // set the last trigger interval to the interval
            this->lastTriggerInterval = this->triggerIntervalCount;

            // reset the count to 0
            this->triggerIntervalCount = 0;
          }
        } else {
          this->triggered = false;
        }
    }

    bool isHigh ( ) {
        return this->triggered;
    }

    bool isLow ( ) {
        return !this->triggered;
    }

    void reset ( ) {
        this->triggered = false;
        this->lastTriggered = false;
        this->lastValue = 0;
        this->triggerCount = 0;
        this->triggerIntervalCount = 0;
        this->lastTriggerInterval = 0;
    }

    float currentValue ( ) {
        return this->lastValue;
    }

    uint32_t triggerInterval ( ) {
        return this->lastTriggerInterval;
    }

    uint32_t triggerTotal ( ) {
        return this->triggerCount;
    }
protected:
  bool triggered;
  bool lastTriggered;
  uint32_t triggerCount;
private:
  float threshold;
  float lastValue;
  uint32_t triggerIntervalCount;
  uint32_t lastTriggerInterval;

};

// CLOCK
#define CLOCK_LIMIT 1024 * 8
#define CLK_ERROR_TOO_MANY 1
class Clock {
public:
    Clock (uint16_t count, float threshold) {
        if (count > CLOCK_LIMIT) {
          throw CLK_ERROR_TOO_MANY;
        }

        triggerThreshold = threshold;
        triggerCount = count;
        current = 0;
        ready = false;
        step = 0;

        cv = new CV(threshold);

        for (uint16_t i = 0; i < CLOCK_LIMIT; i++) {
          states[i] = false;
        }
    }

    bool *update (float input) {
        cv->update(input);

        // only become ready after the first trigger has occurred.  this allows for
        // an interval to be set up
        if (!ready) {
          if (cv->newTrigger()) {
            ready = true;
          }

          return states;
        }

        current++;

        if (cv->newTrigger()) {
          step++;
          current = 0;

          for (uint16_t i = 0; i < triggerCount; i++) {
            states[i] = ((step % (i + 1)) == 0) ? true : false;
          }
        } else if (current >= cv->triggerInterval() / 2) {
          for (uint16_t i = 0; i < triggerCount; i++) {
            states[i] = false;
          }
        }

        if (step >= triggerCount) {
          step = 0;
        }

        return states;
    }

    void reset ( ) {
        current = 0;
        ready = false;
        step = 0;

        for (uint16_t i = 0; i < CLOCK_LIMIT; i++) {
          states[i] = false;
        }

        cv->reset();
    }
protected:
  CV *cv;
  uint16_t triggerCount;
  bool ready;
  uint64_t current;
  uint16_t step;
  float triggerThreshold;
  bool states[CLOCK_LIMIT];
};

// MODULE
struct MegaDivider: Module {
    enum ParamIds {
        A_PARAM,
        B_PARAM,
        C_PARAM,
        D_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        CLOCK_CV,
        RESET_CV,
        A_CV,
        B_CV,
        C_CV,
        D_CV,
        NUM_INPUTS
    };
    enum OutputIds {
        ENUMS(CH_OUTPUT, 8 * 8),
        MULTI_A,
        MULTI_B,
        NUM_OUTPUTS
    };
    enum LightIds {
        ENUMS(CH_LIGHT, 8 * 8),
        A_LIGHT,
        B_LIGHT,
        NUM_LIGHTS
    };

    Clock *clock;
    CV *cv;

    // Displays
    int a_display = 4;
    int b_display = 7;
    int c_display = 4;
    int d_display = 7;

    MegaDivider() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(MegaDivider::A_PARAM, 1, 64, 2, "Mutli A");
        configParam(MegaDivider::B_PARAM, 1, 64, 7, "Mutli B");
        configParam(MegaDivider::C_PARAM, 1, 64, 4, "Mutli C");
        configParam(MegaDivider::D_PARAM, 1, 64, 9, "Mutli D");
        clock = new Clock(8 * 8, 1.7f);
        cv = new CV(1.7f);
    }

    void process(const ProcessArgs& args) override {

        int A = (int)params[A_PARAM].getValue() * clamp(inputs[A_CV].normalize(5.0f) / 5.0f, 0.0f, 1.0f);
        int B = (int)params[B_PARAM].getValue() * clamp(inputs[B_CV].normalize(5.0f) / 5.0f, 0.0f, 1.0f);
        int C = (int)params[C_PARAM].getValue() * clamp(inputs[C_CV].normalize(5.0f) / 5.0f, 0.0f, 1.0f);
        int D = (int)params[D_PARAM].getValue() * clamp(inputs[D_CV].normalize(5.0f) / 5.0f, 0.0f, 1.0f);

        a_display = A;
        b_display = B;
        c_display = C;
        d_display = D;

        if(!inputs[CLOCK_CV].isConnected()){
            return;
        }

        float reset_in = inputs[RESET_CV].value;
        cv->update(reset_in);

        if (cv->newTrigger()) {
            clock->reset();
        }

        float in = inputs[CLOCK_CV].value;
        bool *states = clock->update(in);

        for (int i = 0; i < 8 * 8; i++) {
            if (states[i] == true) {
              outputs[CH_OUTPUT + i].value = in;
              lights[i].value = 1.0f;
            } else {
              outputs[CH_OUTPUT + i].value = 0;
              lights[i].value = 0;
            }
        }

        if (states[A - 1] || states[B - 1]){
            outputs[MULTI_A].value = in;
            lights[A_LIGHT].value = 1.0f;
        } else{
            outputs[MULTI_A].value = 0;
            lights[A_LIGHT].value = 0.0f;
        }
        if (states[C - 1] || states[D - 1]){
            outputs[MULTI_B].value = in;
            lights[B_LIGHT].value = 1.0f;
        } else{
            outputs[MULTI_B].value = 0;
            lights[B_LIGHT].value = 0.0f;
        }
    }
};

/*
Display
*/

struct MegaDividerSmallStringDisplayWidget : TransparentWidget {

  MegaDivider *module;
  bool is_a = true;
  bool is_b = false;
  bool is_c = false;
  bool is_d = false;
  std::shared_ptr<Font> font;

  MegaDividerSmallStringDisplayWidget() {
    font = Font::load(assetPlugin(pluginInstance, "res/Pokemon.ttf"));
  };

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
    nvgFontSize(vg, 20);
    nvgFontFaceId(vg, font->handle);
    nvgTextLetterSpacing(vg, 0.4);

    if (!module)
        return;

    std::stringstream to_display;
    if(is_a){
        to_display << std::setw(2) << std::to_string(module->a_display);
    } else if (is_b){
        to_display << std::setw(2) << std::to_string(module->b_display);
    } else if (is_c){
        to_display << std::setw(2) << std::to_string(module->c_display);
    } else if (is_d){
        to_display << std::setw(2) << std::to_string(module->d_display);
    }

    Vec textPos = Vec(6.0f, 24.0f);
    NVGcolor textColor = nvgRGB(0x00, 0x00, 0x00);
    nvgFillColor(vg, textColor);
    nvgText(vg, textPos.x, textPos.y, to_display.str().c_str(), NULL);
  }
};

struct MegaDividerRoundSmallBlackKnob : RoundSmallBlackKnob
{
    MegaDividerRoundSmallBlackKnob()
    {
        setSVG(SVG::load(assetPlugin(pluginInstance, "res/KTFRoundSmallBlackKnob.svg")));
        minAngle = -0.83 * M_PI;
        maxAngle = 0.83 * M_PI;
        snap = true;
    }
};


// WIDGET
struct MegaDividerWidget: ModuleWidget {
  MegaDividerWidget(MegaDivider *module) {
        setModule(module);
        box.size = Vec(330, 380);

        {
            SVGPanel *panel = new SVGPanel();
            panel->box.size = box.size;
            panel->setBackground(SVG::load(assetPlugin(pluginInstance, "res/MegaDivider.svg")));
            addChild(panel);
        }

        if (!module)
            return;

        // Displays
        if(module != NULL){
            MegaDividerSmallStringDisplayWidget *a_Display = new MegaDividerSmallStringDisplayWidget();
            a_Display->box.pos = Vec(108, 36);
            a_Display->box.size = Vec(33, 33);
            a_Display->module = module;
            a_Display->is_a = true;
            a_Display->is_b = false;
            a_Display->is_c = false;
            a_Display->is_d = false;
            addChild(a_Display);
        }
        addParam(createParam<MegaDividerRoundSmallBlackKnob>(Vec(145, 40), module, MegaDivider::A_PARAM));
        addInput(createPort<PJ301MPort>(Vec(170, 39), PortWidget::INPUT, module, MegaDivider::A_CV));

        if(module != NULL){
            MegaDividerSmallStringDisplayWidget *b_Display = new MegaDividerSmallStringDisplayWidget();
            b_Display->box.pos = Vec(200, 36);
            b_Display->box.size = Vec(33, 33);
            b_Display->module = module;
            b_Display->is_a = false;
            b_Display->is_b = true;
            b_Display->is_c = false;
            b_Display->is_d = false;
            addChild(b_Display);
        }

        addParam(createParam<MegaDividerRoundSmallBlackKnob>(Vec(237, 40), module, MegaDivider::B_PARAM));
        addInput(createPort<PJ301MPort>(Vec(262, 39), PortWidget::INPUT, module, MegaDivider::B_CV));
        addOutput(createPort<PJ301MPort>(Vec(295, 39), PortWidget::OUTPUT, module, MegaDivider::MULTI_A));
        addChild(createLight<MediumLight<WhiteLight>>(Vec(302.5, 68), module, MegaDivider::A_LIGHT));

        // Row 2
        int ROW_TWO = 42;
        if(module != NULL){
            MegaDividerSmallStringDisplayWidget *c_Display = new MegaDividerSmallStringDisplayWidget();
            c_Display->box.pos = Vec(108, 36 + ROW_TWO);
            c_Display->box.size = Vec(33, 33);
            c_Display->module = module;
            c_Display->is_a = false;
            c_Display->is_b = false;
            c_Display->is_c = true;
            c_Display->is_d = false;
            addChild(c_Display);
        }
        addParam(createParam<MegaDividerRoundSmallBlackKnob>(Vec(145, 40 + ROW_TWO), module, MegaDivider::C_PARAM));
        addInput(createPort<PJ301MPort>(Vec(170, 39 + ROW_TWO), PortWidget::INPUT, module, MegaDivider::C_CV));

        if(module != NULL){
            MegaDividerSmallStringDisplayWidget *d_Display = new MegaDividerSmallStringDisplayWidget();
            d_Display->box.pos = Vec(200, 36 + ROW_TWO);
            d_Display->box.size = Vec(33, 33);
            d_Display->module = module;
            d_Display->is_a = false;
            d_Display->is_b = false;
            d_Display->is_c = false;
            d_Display->is_d = true;
            addChild(d_Display);
        }

        addParam(createParam<MegaDividerRoundSmallBlackKnob>(Vec(237, 40 + ROW_TWO), module, MegaDivider::D_PARAM));
        addInput(createPort<PJ301MPort>(Vec(262, 39 + ROW_TWO), PortWidget::INPUT, module, MegaDivider::D_CV));
        addOutput(createPort<PJ301MPort>(Vec(295, 39 + ROW_TWO), PortWidget::OUTPUT, module, MegaDivider::MULTI_B));
        addChild(createLight<MediumLight<WhiteLight>>(Vec(302.5, 68 + ROW_TWO), module, MegaDivider::B_LIGHT));

        // GRID
        int SPACE = 30;
        int BASE = 136;
        int LEFT = 38;

        addInput(createPort<PJ301MPort>(Vec(LEFT - 20, 30), PortWidget::INPUT, module, MegaDivider::CLOCK_CV));
        addInput(createPort<PJ301MPort>(Vec(LEFT - 20, 60), PortWidget::INPUT, module, MegaDivider::RESET_CV));
        // addInput(createPort<PJ301MPort>(Vec(LEFT - 20, 30), PortWidget::INPUT, module, MegaDivider::CLOCK_CV));

        int i = 0;
        for(int x = 1; x <= 8; x++){
            addOutput(createPort<PJ301MPort>(Vec((LEFT * x) - 20, BASE + (SPACE * 0)), PortWidget::OUTPUT, module, MegaDivider::CH_OUTPUT + i)); i++;
            addOutput(createPort<PJ301MPort>(Vec((LEFT * x) - 20, BASE + SPACE * 1), PortWidget::OUTPUT, module, MegaDivider::CH_OUTPUT + i)); i++;
            addOutput(createPort<PJ301MPort>(Vec((LEFT * x) - 20, BASE + SPACE * 2), PortWidget::OUTPUT, module, MegaDivider::CH_OUTPUT + i)); i++;
            addOutput(createPort<PJ301MPort>(Vec((LEFT * x) - 20, BASE + SPACE * 3), PortWidget::OUTPUT, module, MegaDivider::CH_OUTPUT + i)); i++;
            addOutput(createPort<PJ301MPort>(Vec((LEFT * x) - 20, BASE + SPACE * 4), PortWidget::OUTPUT, module, MegaDivider::CH_OUTPUT + i)); i++;
            addOutput(createPort<PJ301MPort>(Vec((LEFT * x) - 20, BASE + SPACE * 5), PortWidget::OUTPUT, module, MegaDivider::CH_OUTPUT + i)); i++;
            addOutput(createPort<PJ301MPort>(Vec((LEFT * x) - 20, BASE + SPACE * 6), PortWidget::OUTPUT, module, MegaDivider::CH_OUTPUT + i)); i++;
            addOutput(createPort<PJ301MPort>(Vec((LEFT * x) - 20, BASE + SPACE * 7), PortWidget::OUTPUT, module, MegaDivider::CH_OUTPUT + i)); i++;
        }

        BASE = BASE + 8;
        int RIGHT = 38;
        i = 0;
        for(int x = 1; x <= 8; x++){
            addChild(createLight<MediumLight<WhiteLight>>(Vec((RIGHT * x) + 7, BASE            ), module, MegaDivider::CH_LIGHT + i)); i++;
            addChild(createLight<MediumLight<WhiteLight>>(Vec((RIGHT * x) + 7, BASE + SPACE * 1), module, MegaDivider::CH_LIGHT + i)); i++;
            addChild(createLight<MediumLight<WhiteLight>>(Vec((RIGHT * x) + 7, BASE + SPACE * 2), module, MegaDivider::CH_LIGHT + i)); i++;
            addChild(createLight<MediumLight<WhiteLight>>(Vec((RIGHT * x) + 7, BASE + SPACE * 3), module, MegaDivider::CH_LIGHT + i)); i++;
            addChild(createLight<MediumLight<WhiteLight>>(Vec((RIGHT * x) + 7, BASE + SPACE * 4), module, MegaDivider::CH_LIGHT + i)); i++;
            addChild(createLight<MediumLight<WhiteLight>>(Vec((RIGHT * x) + 7, BASE + SPACE * 5), module, MegaDivider::CH_LIGHT + i)); i++;
            addChild(createLight<MediumLight<WhiteLight>>(Vec((RIGHT * x) + 7, BASE + SPACE * 6), module, MegaDivider::CH_LIGHT + i)); i++;
            addChild(createLight<MediumLight<WhiteLight>>(Vec((RIGHT * x) + 7, BASE + SPACE * 7), module, MegaDivider::CH_LIGHT + i)); i++;
        }
    }

};

Model *modelMegaDivider = createModel<MegaDivider, MegaDividerWidget>("MegaDivider");
