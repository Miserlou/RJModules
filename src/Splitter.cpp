#include "RJModules.hpp"
#include <iostream>
#include <cmath>

struct Splitter: Module {
    enum ParamIds {
        NUM_PARAMS
    };
    enum InputIds {
        CH1_INPUT,
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
        CH9_OUTPUT,
        NUM_OUTPUTS
    };

    Splitter() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS);}
    void step() override;
};


#define ROUND(f) ((float)((f > 0.0) ? floor(f + 0.5) : ceil(f - 0.5)))

void Splitter::step() {

    outputs[CH1_OUTPUT].value = inputs[CH1_INPUT].value;
    outputs[CH2_OUTPUT].value = inputs[CH1_INPUT].value;
    outputs[CH3_OUTPUT].value = inputs[CH1_INPUT].value;
    outputs[CH4_OUTPUT].value = inputs[CH1_INPUT].value;
    outputs[CH5_OUTPUT].value = inputs[CH1_INPUT].value;
    outputs[CH6_OUTPUT].value = inputs[CH1_INPUT].value;
    outputs[CH7_OUTPUT].value = inputs[CH1_INPUT].value;
    outputs[CH8_OUTPUT].value = inputs[CH1_INPUT].value;
    outputs[CH9_OUTPUT].value = inputs[CH1_INPUT].value;

}

struct SplitterWidget: ModuleWidget {
    SplitterWidget(Splitter *module);
};

SplitterWidget::SplitterWidget(Splitter *module) {
		setModule(module);
    box.size = Vec(15*10, 380);

    {
        SVGPanel *panel = new SVGPanel();
        panel->box.size = box.size;
        panel->setBackground(SVG::load(assetPlugin(pluginInstance, "res/Splitter.svg")));
        addChild(panel);
    }

    addChild(createWidget<ScrewSilver>(Vec(15, 0)));
    addChild(createWidget<ScrewSilver>(Vec(box.size.x-30, 0)));
    addChild(createWidget<ScrewSilver>(Vec(15, 365)));
    addChild(createWidget<ScrewSilver>(Vec(box.size.x-30, 365)));

    addInput(createPort<PJ301MPort>(Vec(64, 89), PortWidget::INPUT, module, Splitter::CH1_INPUT));

    addOutput(createPort<PJ301MPort>(Vec(24, 173), PortWidget::OUTPUT, module, Splitter::CH1_OUTPUT));
    addOutput(createPort<PJ301MPort>(Vec(65, 173), PortWidget::OUTPUT, module, Splitter::CH2_OUTPUT));
    addOutput(createPort<PJ301MPort>(Vec(105, 173), PortWidget::OUTPUT, module, Splitter::CH3_OUTPUT));

    addOutput(createPort<PJ301MPort>(Vec(24, 223), PortWidget::OUTPUT, module, Splitter::CH4_OUTPUT));
    addOutput(createPort<PJ301MPort>(Vec(65, 223), PortWidget::OUTPUT, module, Splitter::CH5_OUTPUT));
    addOutput(createPort<PJ301MPort>(Vec(105, 223), PortWidget::OUTPUT, module, Splitter::CH6_OUTPUT));

    addOutput(createPort<PJ301MPort>(Vec(24, 274), PortWidget::OUTPUT, module, Splitter::CH7_OUTPUT));
    addOutput(createPort<PJ301MPort>(Vec(65, 274), PortWidget::OUTPUT, module, Splitter::CH8_OUTPUT));
    addOutput(createPort<PJ301MPort>(Vec(106, 274), PortWidget::OUTPUT, module, Splitter::CH9_OUTPUT));

}

Model *modelSplitter = createModel<Splitter, SplitterWidget>("Splitter");
