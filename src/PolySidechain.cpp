#include "RJModules.hpp"
#include <iostream>
#include <cmath>

struct PolySidechain: Module {
    enum ParamIds {
        RATIO_PARAM,
        DECAY_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        CH1_INPUT,
        TRIGGER_INPUT,
        RATIO_CV_INPUT,
        DECAY_CV_INPUT,
        NUM_INPUTS
    };
    enum OutputIds {
        CH1_OUTPUT,
        NUM_OUTPUTS
    };

    float decayAmount = 0.0;
    PolySidechain() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS);
        configParam(PolySidechain::RATIO_PARAM, 0.0, 1.0, 1.0, "");
        configParam(PolySidechain::DECAY_PARAM, 0.0, 1.0, 0.3, "");
    }
    void step() override;
};


#define ROUND(f) ((float)((f > 0.0) ? floor(f + 0.5) : ceil(f - 0.5)))

void PolySidechain::step() {

}


struct PolySidechainWidget: ModuleWidget {
    PolySidechainWidget(PolySidechain *module);
};

PolySidechainWidget::PolySidechainWidget(PolySidechain *module) {
        setModule(module);
    box.size = Vec(15*10, 380);

    {
        SVGPanel *panel = new SVGPanel();
        panel->box.size = box.size;
        panel->setBackground(SVG::load(assetPlugin(pluginInstance, "res/PolySidechain.svg")));
        addChild(panel);
    }

    addChild(createWidget<ScrewSilver>(Vec(15, 0)));
    addChild(createWidget<ScrewSilver>(Vec(box.size.x-30, 0)));
    addChild(createWidget<ScrewSilver>(Vec(15, 365)));
    addChild(createWidget<ScrewSilver>(Vec(box.size.x-30, 365)));

    addParam(createParam<RoundBlackKnob>(Vec(57, 159), module, PolySidechain::RATIO_PARAM));
    addParam(createParam<RoundBlackKnob>(Vec(57, 239), module, PolySidechain::DECAY_PARAM));

    addInput(createPort<PJ301MPort>(Vec(22, 100), PortWidget::INPUT, module, PolySidechain::CH1_INPUT));
    addInput(createPort<PJ301MPort>(Vec(22, 180), PortWidget::INPUT, module, PolySidechain::RATIO_CV_INPUT));
    addInput(createPort<PJ301MPort>(Vec(22, 260), PortWidget::INPUT, module, PolySidechain::DECAY_CV_INPUT));
    addInput(createPort<PJ301MPort>(Vec(110, 100), PortWidget::INPUT, module, PolySidechain::TRIGGER_INPUT));

    addOutput(createPort<PJ301MPort>(Vec(110, 305), PortWidget::OUTPUT, module, PolySidechain::CH1_OUTPUT));
}

Model *modelPolySidechain = createModel<PolySidechain, PolySidechainWidget>("PolySidechain");
