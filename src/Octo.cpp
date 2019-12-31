#include "RJModules.hpp"
#include "dsp/digital.hpp"
#include <iostream>
#include <cmath>
#include <sstream>
#include <iomanip>


struct OctoRoundLargeBlackKnob : RoundLargeBlackKnob
{
    OctoRoundLargeBlackKnob()
    {
        setSVG(SVG::load(assetPlugin(pluginInstance, "res/KTFRoundLargeBlackKnob.svg")));
    }
};

struct OctoRoundSmallBlackKnob : RoundSmallBlackKnob
{
    OctoRoundSmallBlackKnob()
    {
        setSVG(SVG::load(assetPlugin(pluginInstance, "res/KTFRoundSmallBlackKnob.svg")));
    }
};

struct Octo: Module {
    enum ParamIds {
        SPEED_PARAM,
        SPEED_ATTEN_PARAM,
        RESET_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        SPEED_CV,
        NUM_INPUTS
    };
    enum OutputIds {
        CH1_OUTPUT,
        CH2_OUTPUT,
        CH3_OUTPUT,
        CH4_OUTPUT,
        CH5_OUTPUT,
        CH6_OUTPUT,
        CH7_OUTPUT,
        CH8_OUTPUT,
        NUM_OUTPUTS
    };
    enum LightIds {
        CH1_LIGHT,
        CH2_LIGHT,
        CH3_LIGHT,
        CH4_LIGHT,
        CH5_LIGHT,
        CH6_LIGHT,
        CH7_LIGHT,
        CH8_LIGHT,
        CH1_NEG_LIGHT,
        CH2_NEG_LIGHT,
        CH3_NEG_LIGHT,
        CH4_NEG_LIGHT,
        CH5_NEG_LIGHT,
        CH6_NEG_LIGHT,
        CH7_NEG_LIGHT,
        CH8_NEG_LIGHT,
        NUM_LIGHTS
    };

    Octo() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(Octo::SPEED_PARAM, 0.0, 1.0, 0.5, "");
        configParam(Octo::SPEED_ATTEN_PARAM, 0.0, 1.0, 0.5, "");
    }
    void step() override;

};

void Octo::step() {

}

struct OctoWidget: ModuleWidget {
    OctoWidget(Octo *module);
};

OctoWidget::OctoWidget(Octo *module) {
    setModule(module);
    box.size = Vec(6*10, 380);

    {
        SVGPanel *panel = new SVGPanel();
        panel->box.size = box.size;
        panel->setBackground(SVG::load(assetPlugin(pluginInstance, "res/Octo.svg")));
        addChild(panel);
    }

    addParam(createParam<OctoRoundLargeBlackKnob>(Vec(12, 55), module, Octo::SPEED_PARAM));
    addParam(createParam<OctoRoundSmallBlackKnob>(Vec(5, 100), module, Octo::SPEED_ATTEN_PARAM));
    addInput(createPort<PJ301MPort>(Vec(32, 99), PortWidget::INPUT, module, Octo::SPEED_CV));

    int SPACE = 30;
    int BASE = 136;
    int LEFT = 18;
    addOutput(createPort<PJ301MPort>(Vec(LEFT, BASE), PortWidget::OUTPUT, module, Octo::CH1_OUTPUT));
    addOutput(createPort<PJ301MPort>(Vec(LEFT, BASE + SPACE * 1), PortWidget::OUTPUT, module, Octo::CH2_OUTPUT));
    addOutput(createPort<PJ301MPort>(Vec(LEFT, BASE + SPACE * 2), PortWidget::OUTPUT, module, Octo::CH3_OUTPUT));
    addOutput(createPort<PJ301MPort>(Vec(LEFT, BASE + SPACE * 3), PortWidget::OUTPUT, module, Octo::CH4_OUTPUT));
    addOutput(createPort<PJ301MPort>(Vec(LEFT, BASE + SPACE * 4), PortWidget::OUTPUT, module, Octo::CH5_OUTPUT));
    addOutput(createPort<PJ301MPort>(Vec(LEFT, BASE + SPACE * 5), PortWidget::OUTPUT, module, Octo::CH6_OUTPUT));
    addOutput(createPort<PJ301MPort>(Vec(LEFT, BASE + SPACE * 6), PortWidget::OUTPUT, module, Octo::CH7_OUTPUT));
    addOutput(createPort<PJ301MPort>(Vec(LEFT, BASE + SPACE * 7), PortWidget::OUTPUT, module, Octo::CH8_OUTPUT));

    BASE = BASE + 8;
    int RIGHT = 46;
    LEFT = 5;
    addChild(createLight<MediumLight<GreenRedLight>>(Vec(RIGHT, BASE            ), module, Octo::CH1_LIGHT));
    addChild(createLight<MediumLight<GreenRedLight>>(Vec(LEFT,  BASE + SPACE * 1), module, Octo::CH2_LIGHT));
    addChild(createLight<MediumLight<GreenRedLight>>(Vec(RIGHT, BASE + SPACE * 2), module, Octo::CH3_LIGHT));
    addChild(createLight<MediumLight<GreenRedLight>>(Vec(LEFT,  BASE + SPACE * 3), module, Octo::CH4_LIGHT));
    addChild(createLight<MediumLight<GreenRedLight>>(Vec(RIGHT, BASE + SPACE * 4), module, Octo::CH5_LIGHT));
    addChild(createLight<MediumLight<GreenRedLight>>(Vec(LEFT,  BASE + SPACE * 5), module, Octo::CH6_LIGHT));
    addChild(createLight<MediumLight<GreenRedLight>>(Vec(RIGHT, BASE + SPACE * 6), module, Octo::CH7_LIGHT));
    addChild(createLight<MediumLight<GreenRedLight>>(Vec(LEFT,  BASE + SPACE * 7), module, Octo::CH8_LIGHT));

}

Model *modelOcto = createModel<Octo, OctoWidget>("Octo");
