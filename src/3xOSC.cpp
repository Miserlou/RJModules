#include "dsp/digital.hpp"
#include <iostream>
#include "RJModules.hpp"

struct Osc {
    float phase = 0.0;
    float pw = 0.5;
    float freq = 1.0;
    bool offset = false;
    bool invert = false;

    SchmittTrigger resetTrigger;
    Osc() {}

    void setPitch(float pitch) {
        pitch = fminf(pitch, 8.0);
        freq = powf(2.0, pitch);
    }

    float getFreq() {
        return freq;
    }
    void setReset(float reset) {
        if (resetTrigger.process(reset)) {
            phase = 0.0;
        }
    }
    void step(float dt) {
        float deltaPhase = fminf(freq * dt, 0.5);
        phase += deltaPhase;
        if (phase >= 1.0)
            phase -= 1.0;
    }
    float saw(float x) {
        return 2.0 * (x - roundf(x));
    }
    float saw() {
        if (offset)
            return invert ? 2.0 * (1.0 - phase) : 2.0 * phase;
        else
            return saw(phase) * (invert ? -1.0 : 1.0);
    }
    float light() {
        return sinf(2*M_PI * phase);
    }
};

struct ThreeXOSC : Module {
    enum ParamIds {
        OFFSET_PARAM,
        INVERT_PARAM,
        FREQ_PARAM,
        DETUNE_PARAM,
        MIX_PARAM,
        THREE_OSC_PARAM,

        NUM_PARAMS
    };
    enum InputIds {
        FREQ_CV_INPUT,
        DETUNE_CV_INPUT,
        MIX_CV_INPUT,
        RESET_INPUT,
        PW_INPUT,
        NUM_INPUTS
    };
    enum OutputIds {
        SAW_OUTPUT,
        NUM_OUTPUTS
    };
    enum LightIds {
        PHASE_POS_LIGHT,
        PHASE_NEG_LIGHT,
        NUM_LIGHTS
    };

    Osc oscillator;
    Osc oscillator2;
    Osc oscillator3;

    float DETUNE_STEP = .075;

    ThreeXOSC() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {}
    void step() override;
};

void ThreeXOSC::step() {

    float root_pitch = params[FREQ_PARAM].value * clamp(inputs[FREQ_CV_INPUT].normalize(10.0f) / 10.0f, 0.0f, 1.0f);
    oscillator.setPitch(root_pitch);
    oscillator.offset = (params[OFFSET_PARAM].value > 0.0);
    oscillator.invert = (params[INVERT_PARAM].value <= 0.0);
    oscillator.step(1.0 / engineGetSampleRate());
    oscillator.setReset(inputs[RESET_INPUT].value);

    oscillator2.setPitch(root_pitch + (params[DETUNE_PARAM].value * DETUNE_STEP * clamp(inputs[DETUNE_CV_INPUT].normalize(10.0f) / 10.0f, 0.0f, 1.0f)));
    oscillator2.offset = (params[OFFSET_PARAM].value > 0.0);
    oscillator2.invert = (params[INVERT_PARAM].value <= 0.0);
    oscillator2.step(1.0 / engineGetSampleRate());
    oscillator2.setReset(inputs[RESET_INPUT].value);

    oscillator3.setPitch(root_pitch - (params[DETUNE_PARAM].value * DETUNE_STEP * clamp(inputs[DETUNE_CV_INPUT].normalize(10.0f) / 10.0f, 0.0f, 1.0f)));
    oscillator3.offset = (params[OFFSET_PARAM].value > 0.0);
    oscillator3.invert = (params[INVERT_PARAM].value <= 0.0);
    oscillator3.step(1.0 / engineGetSampleRate());
    oscillator3.setReset(inputs[RESET_INPUT].value);

    float osc3_saw = oscillator3.saw();
    if (params[OFFSET_PARAM].value < 1){
        osc3_saw = 0;
    } else{
        osc3_saw = oscillator3.saw();
    }

    float mix_percent = params[MIX_PARAM].value * clamp(inputs[MIX_CV_INPUT].normalize(10.0f) / 10.0f, 0.0f, 1.0f);
    outputs[SAW_OUTPUT].value = 5.0 * (( oscillator.saw() + (oscillator2.saw() * mix_percent) + (osc3_saw * mix_percent) / 3));

    lights[PHASE_POS_LIGHT].setBrightnessSmooth(fmaxf(0.0, oscillator.light()));
    lights[PHASE_NEG_LIGHT].setBrightnessSmooth(fmaxf(0.0, -oscillator.light()));
}

struct ThreeXOSCWidget: ModuleWidget {
    ThreeXOSCWidget(ThreeXOSC *module);
};

ThreeXOSCWidget::ThreeXOSCWidget(ThreeXOSC *module) : ModuleWidget(module) {
     box.size = Vec(240, 380);

    {
        SVGPanel *panel = new SVGPanel();
        panel->box.size = box.size;
        panel->setBackground(SVG::load(assetPlugin(plugin, "res/ThreeXOSC.svg")));
        addChild(panel);
    }

    addChild(Widget::create<ScrewSilver>(Vec(15, 0)));
    addChild(Widget::create<ScrewSilver>(Vec(box.size.x-30, 0)));
    addChild(Widget::create<ScrewSilver>(Vec(15, 365)));
    addChild(Widget::create<ScrewSilver>(Vec(box.size.x-30, 365)));

    addParam(ParamWidget::create<RoundLargeBlackKnob>(Vec(25, 61), module, ThreeXOSC::FREQ_PARAM, 0.0, 8.0, 5.0));
    addParam(ParamWidget::create<RoundLargeBlackKnob>(Vec(75, 61), module, ThreeXOSC::DETUNE_PARAM, 0.0, 1.0, 0.1));
    addParam(ParamWidget::create<RoundLargeBlackKnob>(Vec(125, 61), module, ThreeXOSC::MIX_PARAM, 0.0, 1.0, 1.0));
    addParam(ParamWidget::create<RoundLargeBlackKnob>(Vec(175, 61), module, ThreeXOSC::MIX_PARAM, 0.0, 1.0, 1.0));
    addInput(Port::create<PJ301MPort>(Vec(30, 105), Port::INPUT, module, ThreeXOSC::FREQ_CV_INPUT));
    addInput(Port::create<PJ301MPort>(Vec(80, 105), Port::INPUT, module, ThreeXOSC::DETUNE_CV_INPUT));
    addInput(Port::create<PJ301MPort>(Vec(130, 105), Port::INPUT, module, ThreeXOSC::MIX_CV_INPUT));
    addInput(Port::create<PJ301MPort>(Vec(180, 105), Port::INPUT, module, ThreeXOSC::RESET_INPUT));
    addInput(Port::create<PJ301MPort>(Vec(30, 135), Port::INPUT, module, ThreeXOSC::FREQ_CV_INPUT));
    addInput(Port::create<PJ301MPort>(Vec(180, 135), Port::INPUT, module, ThreeXOSC::RESET_INPUT));
    addChild(ModuleLightWidget::create<SmallLight<GreenRedLight>>(Vec(210, 140), module, ThreeXOSC::PHASE_POS_LIGHT));


    // addInput(Port::create<PJ301MPort>(Vec(22, 100), Port::INPUT, module, ThreeXOSC::FREQ_CV_INPUT));
    // addInput(Port::create<PJ301MPort>(Vec(22, 190), Port::INPUT, module, ThreeXOSC::DETUNE_CV_INPUT));
    // addInput(Port::create<PJ301MPort>(Vec(22, 270), Port::INPUT, module, ThreeXOSC::MIX_CV_INPUT));
    // addInput(Port::create<PJ301MPort>(Vec(38, 310), Port::INPUT, module, ThreeXOSC::RESET_INPUT));

    // addOutput(Port::create<PJ301MPort>(Vec(100, 310), Port::OUTPUT, module, ThreeXOSC::SAW_OUTPUT));


}
Model *modelThreeXOSC = Model::create<ThreeXOSC, ThreeXOSCWidget>("RJModules", "ThreeXOSC", "[GEN] 3xOSC", LFO_TAG);
