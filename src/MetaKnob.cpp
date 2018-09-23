#include "RJModules.hpp"
#include "dsp/digital.hpp"
#include <iostream>
#include <cmath>

struct MetaKnob: Module {
    enum ParamIds {
        BIG_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        NUM_INPUTS
    };
    enum OutputIds {
        CH1_OUTPUT,
        CH2_OUTPUT,
        CH3_OUTPUT,
        CH4_OUTPUT,
        CH5_OUTPUT,
        CH6_OUTPUT,
        NUM_OUTPUTS
    };
    enum LightIds {
        RESET_LIGHT,
        NUM_LIGHTS
    };
    float resetLight = 0.0;

    MetaKnob() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {}
    void step() override;
};


struct RoundGiantBlackKnob : RoundKnob {
    RoundGiantBlackKnob() {
        setSVG(SVG::load(assetPlugin(plugin, "res/RoundGiantBlackKnob.svg")));
    }
};

template <typename BASE>
struct GiantLight : BASE {
        GiantLight() {
                this->box.size = mm2px(Vec(34, 34));
        }
};

void MetaKnob::step() {


}

struct MetaKnobWidget: ModuleWidget {
    MetaKnobWidget(MetaKnob *module);
};

MetaKnobWidget::MetaKnobWidget(MetaKnob *module) : ModuleWidget(module) {
    box.size = Vec(15*10, 380);

    {
        SVGPanel *panel = new SVGPanel();
        panel->box.size = box.size;
        panel->setBackground(SVG::load(assetPlugin(plugin, "res/Button.svg")));
        addChild(panel);
    }

    addChild(Widget::create<ScrewSilver>(Vec(15, 0)));
    addChild(Widget::create<ScrewSilver>(Vec(box.size.x-30, 0)));
    addChild(Widget::create<ScrewSilver>(Vec(15, 365)));
    addChild(Widget::create<ScrewSilver>(Vec(box.size.x-30, 365)));

    addOutput(Port::create<PJ301MPort>(Vec(24, 223), Port::OUTPUT, module, MetaKnob::CH1_OUTPUT));
    addOutput(Port::create<PJ301MPort>(Vec(65, 223), Port::OUTPUT, module, MetaKnob::CH2_OUTPUT));
    addOutput(Port::create<PJ301MPort>(Vec(105, 223), Port::OUTPUT, module, MetaKnob::CH3_OUTPUT));

    addOutput(Port::create<PJ301MPort>(Vec(24, 274), Port::OUTPUT, module, MetaKnob::CH4_OUTPUT));
    addOutput(Port::create<PJ301MPort>(Vec(65, 274), Port::OUTPUT, module, MetaKnob::CH5_OUTPUT));
    addOutput(Port::create<PJ301MPort>(Vec(106, 274), Port::OUTPUT, module, MetaKnob::CH6_OUTPUT));

    addParam(ParamWidget::create<RoundGiantBlackKnob>(Vec(15, 60), module, MetaKnob::BIG_PARAM, 0.0, 1.0, 0.0));
    // addChild(ModuleLightWidget::create<GiantLight<GreenLight>>(Vec(25, 70), module, MetaKnob::RESET_LIGHT));

}

Model *modelMetaKnob = Model::create<MetaKnob, MetaKnobWidget>("RJModules", "MetaKnob", "[LIVE] MetaKnob", UTILITY_TAG);
