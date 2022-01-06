#include "RJModules.hpp"
#include <iostream>
#include <cmath>
#include <random>

struct Randoms: Module {
    enum ParamIds {
        NUM_PARAMS
    };
    enum InputIds {
        CH1_CV_INPUT_1,
        CH1_CV_INPUT_2,
        CH2_CV_INPUT_1,
        CH2_CV_INPUT_2,
        CH3_CV_INPUT_1,
        CH3_CV_INPUT_2,
        NUM_INPUTS
    };
    enum OutputIds {
        CH1_OUTPUT,
        CH2_OUTPUT,
        CH3_OUTPUT,
        NUM_OUTPUTS
    };

    Randoms() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS);}
    void step() override;

    std::random_device rd; // obtain a random number from hardware

};

void Randoms::step() {

    float mapped_ch1v1 = inputs[CH1_CV_INPUT_1].value;
    float mapped_ch1v2 = inputs[CH1_CV_INPUT_2].value;
    float mapped_ch2v1 = inputs[CH2_CV_INPUT_1].value;
    float mapped_ch2v2 = inputs[CH2_CV_INPUT_2].value;
    float mapped_ch3v1 = inputs[CH3_CV_INPUT_1].value;
    float mapped_ch3v2 = inputs[CH3_CV_INPUT_2].value;

    std::mt19937 eng(rd()); // seed the generator
    std::uniform_real_distribution<> distr1(mapped_ch1v1, mapped_ch1v2); // define the range
    std::uniform_real_distribution<> distr2(mapped_ch2v1, mapped_ch2v2);
    std::uniform_real_distribution<> distr3(mapped_ch3v1, mapped_ch3v2);

    if (mapped_ch1v1 == mapped_ch1v2){
        mapped_ch1v1 = -12;
        mapped_ch1v2 = 12;
    }

    outputs[CH1_OUTPUT].value = distr1(eng);

    if (mapped_ch2v1 == mapped_ch2v2){
        mapped_ch2v1 = -12;
        mapped_ch2v2 = 12;
    }

    outputs[CH1_OUTPUT].value = distr1(eng);

    if (mapped_ch3v1 == mapped_ch3v2){
        mapped_ch3v1 = -12;
        mapped_ch3v2 = 12;
    }

    outputs[CH1_OUTPUT].value = distr1(eng);
    outputs[CH2_OUTPUT].value = distr2(eng);
    outputs[CH3_OUTPUT].value = distr3(eng);
}

struct RandomsWidget: ModuleWidget {
    RandomsWidget(Randoms *module);
};

RandomsWidget::RandomsWidget(Randoms *module) {
		setModule(module);
    box.size = Vec(15*10, 380);

    {
        SVGPanel *panel = new SVGPanel();
        panel->box.size = box.size;
        panel->setBackground(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Randoms.svg")));
        addChild(panel);
    }

    addChild(createWidget<ScrewSilver>(Vec(15, 0)));
    addChild(createWidget<ScrewSilver>(Vec(box.size.x-30, 0)));
    addChild(createWidget<ScrewSilver>(Vec(15, 365)));
    addChild(createWidget<ScrewSilver>(Vec(box.size.x-30, 365)));


    addInput(createInput<PJ301MPort>(Vec(22, 70), module, Randoms::CH1_CV_INPUT_1));
    addInput(createInput<PJ301MPort>(Vec(22, 100), module, Randoms::CH1_CV_INPUT_2));

    addInput(createInput<PJ301MPort>(Vec(22, 150), module, Randoms::CH2_CV_INPUT_1));
    addInput(createInput<PJ301MPort>(Vec(22, 180), module, Randoms::CH2_CV_INPUT_2));

    addInput(createInput<PJ301MPort>(Vec(22, 230), module, Randoms::CH3_CV_INPUT_1));
    addInput(createInput<PJ301MPort>(Vec(22, 260), module, Randoms::CH3_CV_INPUT_2));

    addOutput(createOutput<PJ301MPort>(Vec(110, 85), module, Randoms::CH1_OUTPUT));
    addOutput(createOutput<PJ301MPort>(Vec(110, 165), module, Randoms::CH2_OUTPUT));
    addOutput(createOutput<PJ301MPort>(Vec(110, 245), module, Randoms::CH3_OUTPUT));
}

Model *modelRandoms = createModel<Randoms, RandomsWidget>("Randoms");
