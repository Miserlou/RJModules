/*
Gravity Glide!
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

struct GGRoundLargeBlackKnob : RoundHugeBlackKnob
{
    GGRoundLargeBlackKnob()
    {
        setSVG(SVG::load(assetPlugin(pluginInstance, "res/KTFRoundHugeBlackKnob.svg")));
    }
};

struct GravityGlide : Module {
    enum ParamIds {
        T_PARAM,
        M_PARAM,
        G_PARAM,
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
    float T;
    float M;
    float G;
    // float G = .001;
    // float step = .0001;
    float damping = .01;

    GravityGlide() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

        // Min Max Default
        configParam(GravityGlide::T_PARAM, .00001, .01, .0001, "Timestep");
        configParam(GravityGlide::M_PARAM, 0.1, .25, 0.5, "Mass");
        configParam(GravityGlide::G_PARAM, .05, 15.0, .5, "Gravity");

    }

    void process(const ProcessArgs &args) override {

    /* Get the values every 8 steps */
    if(param_counter>=7){
        T = params[T_PARAM].getValue();
        M = params[M_PARAM].getValue();
        G = params[G_PARAM].getValue();
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
    float damping_force = damping * current_speed;
    float forceY = M * G - damping_force;
    float accelerationY = forceY / M;


    //attempt 2
    // float accelerationY = G*M;///(d*d);
    // F is actually step

    // going up
    if(input > current_location){
        float forceY = M * G - damping_force;
        float accelerationY = forceY / M;
        current_speed = current_speed + accelerationY * T;
        current_location = current_location + current_speed * T;
    }
    else if (input <= current_location){
        float forceY = M * G + damping_force;
        float accelerationY = forceY / M;
        current_speed = current_speed - accelerationY * T;
        current_location = current_location + current_speed * T;
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

        addParam(createParam<GGRoundLargeBlackKnob>(mm2px(Vec(6, 30 + BUFF)), module, GravityGlide::T_PARAM));
        addParam(createParam<GGRoundLargeBlackKnob>(mm2px(Vec(6, 55 + BUFF)), module, GravityGlide::M_PARAM));
        addParam(createParam<GGRoundLargeBlackKnob>(mm2px(Vec(6, 80 + BUFF)), module, GravityGlide::G_PARAM));

        addInput(createPort<PJ301MPort>(mm2px(Vec(1.51398, 73.3 + TWO)), PortWidget::INPUT, module, GravityGlide::IN_INPUT));

        addOutput(createPort<PJ301MPort>(mm2px(Vec(20, 73.3 + TWO)), PortWidget::OUTPUT, module, GravityGlide::OUT_OUTPUT));

    }
};

Model *modelGravityGlide = createModel<GravityGlide, GravityGlideWidget>("GravityGlide");
