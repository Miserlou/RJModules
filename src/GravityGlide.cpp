/*
Gravity Glide!
*/

#include "RJModules.hpp"
#include "common.hpp"
#include <stdlib.h>
#include <iostream>
#include <cmath>
#include <sstream>
#include <iomanip>
#include <unistd.h>
#include <mutex>

using namespace std;
#define HISTORY_SIZE (1<<21)

struct GGRoundLargeBlackKnob : RoundHugeBlackKnob
{
    GGRoundLargeBlackKnob()
    {
        setSVG(SVG::load(assetPlugin(pluginInstance, "res/KTFRoundHugeBlackKnob.svg")));
    }
};

struct GravityGlide : Module {
    enum ParamIds {
        F_PARAM,
        M_PARAM,
        A_PARAM,
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
    float F;
    float M;
    float A;
    float G = .001;
    float step = .0001;

    GravityGlide() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

        configParam(GravityGlide::F_PARAM, 0.0, 1.0, 0.1, "Force");
        configParam(GravityGlide::M_PARAM, 0.1, 0.1, 10.0, "Mass");
        configParam(GravityGlide::A_PARAM, 0.0, 1.0, 0.1, "Acceleration");

    }

    void process(const ProcessArgs &args) override {

    /* Get the values every 8 steps */
    if(param_counter>=7){
        F = params[F_PARAM].getValue();
        M = params[M_PARAM].getValue();
        A = params[A_PARAM].getValue();
        param_counter = 0;
    } else{
        param_counter++;
    }

    if(!started){
        current_location = inputs[IN_INPUT].value;
        current_speed = 0;
        started = true;
    }

    // Math
    float input = inputs[IN_INPUT].value;

    if(input > current_location){
        float forceY = M * G;
        float accelerationY = forceY/M;

        current_speed = current_speed + accelerationY * step;
        current_location = current_location + current_speed * step;
        std::cout << "Location: " << current_location << "\n";
        std::cout << "Speed: " << current_speed << "\n";
    }
    else if (input < current_location){
        float forceY = M * G;
        float accelerationY = forceY / M;

        current_speed = current_speed + accelerationY * step;
        current_location = current_location - current_speed * step;
        std::cout << "Location: " << current_location << "\n";
        std::cout << "Speed: " << current_speed << "\n";
    }

    outputs[OUT_OUTPUT].setVoltage(current_location);

    }
};

struct GravityGlideWidget : ModuleWidget {
    GravityGlideWidget(GravityGlide *module) {
        setModule(module);
        setPanel(SVG::load(assetPlugin(pluginInstance, "res/GravityGlide.svg")));

        int TWO = 45;
        int BUFF = 6;

        addParam(createParam<GGRoundLargeBlackKnob>(mm2px(Vec(6, 30 + BUFF)), module, GravityGlide::F_PARAM));
        addParam(createParam<GGRoundLargeBlackKnob>(mm2px(Vec(6, 55 + BUFF)), module, GravityGlide::M_PARAM));
        addParam(createParam<GGRoundLargeBlackKnob>(mm2px(Vec(6, 80 + BUFF)), module, GravityGlide::A_PARAM));

        addInput(createPort<PJ301MPort>(mm2px(Vec(1.51398, 73.3 + TWO)), PortWidget::INPUT, module, GravityGlide::IN_INPUT));

        addOutput(createPort<PJ301MPort>(mm2px(Vec(20, 73.3 + TWO)), PortWidget::OUTPUT, module, GravityGlide::OUT_OUTPUT));

    }
};

Model *modelGravityGlide = createModel<GravityGlide, GravityGlideWidget>("GravityGlide");
