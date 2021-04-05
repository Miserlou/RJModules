/*
SubOsc
*/

#include "RJModules.hpp"
#include "common.hpp"
#include <stdlib.h>
#include <iostream>
#include <cmath>
#include <math.h>
#include <sstream>
#include <iomanip>
#include <unistd.h>
#include <mutex>

using namespace std;
#define HISTORY_SIZE (1<<21)

//

float rerange(float old_min, float old_max, float new_min, float new_max, float old_value){
  float output = (old_value - old_min) / (old_max - old_min) * (new_max - new_min) + new_min;
  return output;
}

float mix_3(
  float mix_1, float value_1, 
  float mix_2, float value_2, 
  float mix_3, float value_3)
{
  if (mix_1 == 0.0f && mix_2 == 0.0f && mix_3 == 0.0f){
    return 0.0f;
  }
  float sum = mix_1 + mix_2 + mix_3 + 1.f;
  return clamp(rerange(-2.5f, 2.5f, -5.f, 5.f, (
    ((mix_1 * value_1) / sum) +
    ((mix_2 * value_2) / sum) +
    ((mix_3 * value_3) / sum)
  )), -5.f, 5.f);
}

//

struct SORoundLargeBlackKnob : RoundHugeBlackKnob
{
    SORoundLargeBlackKnob()
    {
        setSVG(SVG::load(assetPlugin(pluginInstance, "res/KTFRoundHugeBlackKnob.svg")));
    }
};

struct SubOsc : Module {
    enum ParamIds {
        A_PARAM,
        B_PARAM,
        C_PARAM,
        D_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        IN_INPUT,
        NUM_INPUTS
    };
    enum OutputIds {
        OUT_OUTPUT,
        NUM_OUTPUTS
    };
    enum LightIds {
        NUM_LIGHTS
    };

    bool started = false;
    float current_location;
    float current_speed;

    /* Input Caching */
    int param_counter = 7;
    float A;
    float crossover;
    float C;
    float D;

    bool SUB_1 = false;
    bool SUB_1_state = false;
    int sub1_count = 0;

    bool SUB_2 = false;
    bool SUB_2_state = false;
    int sub2_count = 0;

    float sub1_out;
    float sub2_out;
    float max = 5.0;
    float min = -5.0;

    int xo_count = 2;
    int feedback_mode_index = 0;

    SubOsc() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

        // Min Max Default
        configParam(SubOsc::A_PARAM, 0.0, 1.0, 0.0, "Dry");
        configParam(SubOsc::B_PARAM, -4.999, 4.999, 0.0, "Crossover");
        configParam(SubOsc::C_PARAM, 0.0, 1.0, 1.0, "-1 Octave");
        configParam(SubOsc::D_PARAM, 0.0, 1.0, 0.0, "-2 Octave");
    }

    void setXO(int xo) {
        xo_count = xo;
    }

    void process(const ProcessArgs &args) override {

    /* Get the values every 8 steps */
    if(param_counter>=7){
        A = params[A_PARAM].getValue();
        crossover = params[B_PARAM].getValue();
        C = params[C_PARAM].getValue();
        D = params[D_PARAM].getValue();
        param_counter = 0;
    } else{
        param_counter++;
    }

    float input = inputs[IN_INPUT].value;
    xo_count = feedback_mode_index + 2;

    // SUB 1
    if(SUB_1){
        if(input<=crossover){
            SUB_1 = false;
            sub1_count++;
        }
    } else{
        if(input>crossover){
            SUB_1 = true;
            sub1_count++;
        }
    }
    if(sub1_count == xo_count){
        SUB_1_state = !SUB_1_state;
        sub1_count = 0;
    }
    if(SUB_1_state){
        sub1_out = max;
    } else{
        sub1_out = min;
    }

    // SUB2
    if(SUB_2){
        if(input<=crossover){
            SUB_2 = false;
            sub2_count++;
        }
    } else{
        if(input>crossover){
            SUB_2 = true;
            sub2_count++;
        }
    }
    if(sub2_count == xo_count*2){
        SUB_2_state = !SUB_2_state;
        sub2_count = 0;
    }
    if(SUB_2_state){
        sub2_out = max;
    } else{
        sub2_out = min;
    }

    // MIX
    float mix = mix_3(
        A, input,
        C, sub1_out,
        D, sub2_out
    );

    outputs[OUT_OUTPUT].setVoltage(mix);

    }

};

struct SubOscWidget : ModuleWidget {
    SubOscWidget(SubOsc *module) {
        setModule(module);
        setPanel(SVG::load(assetPlugin(pluginInstance, "res/SubOsc.svg")));

        int TWO = 45;
        int BUFF = 15;

        addParam(createParam<SORoundLargeBlackKnob>(mm2px(Vec(6, 5 + BUFF)), module, SubOsc::A_PARAM));
        addParam(createParam<SORoundLargeBlackKnob>(mm2px(Vec(6, 30 + BUFF)), module, SubOsc::B_PARAM));
        addParam(createParam<SORoundLargeBlackKnob>(mm2px(Vec(6, 55 + BUFF)), module, SubOsc::C_PARAM));
        addParam(createParam<SORoundLargeBlackKnob>(mm2px(Vec(6, 80 + BUFF)), module, SubOsc::D_PARAM));

        addInput(createPort<PJ301MPort>(mm2px(Vec(1.51398, 73.3 + TWO)), PortWidget::INPUT, module, SubOsc::IN_INPUT));

        addOutput(createPort<PJ301MPort>(mm2px(Vec(20, 73.3 + TWO)), PortWidget::OUTPUT, module, SubOsc::OUT_OUTPUT));

    }

    void appendContextMenu(Menu *menu) override
    {
        SubOsc *module = dynamic_cast<SubOsc *>(this->module);

        struct FeedbackIndexItem : MenuItem
        {
            SubOsc *module;
            int index;
            void onAction(const event::Action &e) override
            {
                module->feedback_mode_index = index;
            }
        };

        struct FeedbackItem : MenuItem
        {
            SubOsc *module;
            Menu *createChildMenu() override
            {
                Menu *menu = new Menu();
                const std::string feedbackLabels[] = {
                    "2",
                    "3",
                    "4",
                    "5",
                    "6",
                    "7"
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

        menu->addChild(new MenuEntry);
        FeedbackItem *feedbackItem = createMenuItem<FeedbackItem>("Crossover Count", ">");
        feedbackItem->module = module;
        menu->addChild(feedbackItem);

    }

};

Model *modelSubOsc = createModel<SubOsc, SubOscWidget>("SubOsc");
