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
        NUM_PARAMS
    };
    enum InputIds {
        CLOCK_CV,
        RESET_CV,
        NUM_INPUTS
    };
    enum OutputIds {
        ENUMS(CH_OUTPUT, 8 * 8),
        NUM_OUTPUTS
    };
    enum LightIds {
        ENUMS(CH_LIGHT, 8 * 8),
        NUM_LIGHTS
    };

    Clock *clock;
    CV *cv;

    MegaDivider() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        clock = new Clock(8 * 8, 1.7f);
        cv = new CV(1.7f);
    }

    void process(const ProcessArgs& args) override {

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
              outputs[i].value = in;
              lights[i].value = 1.0f;
            } else {
              outputs[i].value = 0;
              lights[i].value = 0;
            }
        }
    }
};

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
