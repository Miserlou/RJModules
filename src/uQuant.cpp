/*

uQuant is basically a bastard skin for AmalgamaticHarmonic's Scale Quantizer MKII:
https://github.com/jhoar/AmalgamatedHarmonics

*/

#include "dsp/digital.hpp"

#include "RJModules.hpp"
#include "Core.hpp"
#include "UI.hpp"

#include <iostream>
#include <cmath>
#include <sstream>
#include <iomanip>

// Displays
struct TinyStringDisplayWidget : TransparentWidget {

  std::string *value;
  std::shared_ptr<Font> font;

  TinyStringDisplayWidget() {
    font = Font::load(assetPlugin(plugin, "res/Pokemon.ttf"));
  };

  void draw(NVGcontext *vg) override
  {
    // Background
    NVGcolor backgroundColor = nvgRGB(0xC0, 0xC0, 0xC0);
    nvgBeginPath(vg);
    nvgRoundedRect(vg, 0.0, 0.0, box.size.x, box.size.y, 4.0);
    nvgFillColor(vg, backgroundColor);
    nvgFill(vg);

    // text
    nvgFontSize(vg, 13);
    nvgFontFaceId(vg, font->handle);
    nvgTextLetterSpacing(vg, 2);

    std::stringstream to_display;
    to_display << std::setw(3) << *value;

    Vec textPos = Vec(4.0f, 20.0f);
    NVGcolor textColor = nvgRGB(0x00, 0x00, 0x00);
    nvgFillColor(vg, textColor);
    nvgText(vg, textPos.x, textPos.y, to_display.str().c_str(), NULL);
  }
};

// Module
struct uQuant : AHModule {

    enum ParamIds {
        KEY_PARAM,
        SCALE_PARAM,
        ENUMS(SHIFT_PARAM,8),
        TRANS_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        ENUMS(IN_INPUT,8),
        KEY_INPUT,
        SCALE_INPUT,
        TRANS_INPUT,
        ENUMS(HOLD_INPUT,8),
        NUM_INPUTS
    };
    enum OutputIds {
        ENUMS(OUT_OUTPUT,8),
        ENUMS(TRIG_OUTPUT,8),
        NUM_OUTPUTS
    };
    enum LightIds {
        ENUMS(KEY_LIGHT,12),
        ENUMS(SCALE_LIGHT,12),
        NUM_LIGHTS
    };

    uQuant() : AHModule(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {}

    void step() override;

    bool firstStep = true;
    int lastScale = 0;
    int lastRoot = 0;
    float lastTrans = -10000.0f;

    SchmittTrigger holdTrigger[8];
    float holdPitch[8] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0};
    float lastPitch[8] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0};
    PulseGenerator triggerPulse[8];

    std::string keys[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    std::string scales[12] = {"C", "Io", "Do", "Ph", "Ly", "Mi", "Ae", "Lo", "5M", "5m", "Hm", "B"};

    int currScale = 0;
    int currRoot = 0;

};

void uQuant::step() {

    AHModule::step();

    lastScale = currScale;
    lastRoot = currRoot;

    int currNote = 0;
    int currDegree = 0;

    if (inputs[KEY_INPUT].active) {
        float fRoot = inputs[KEY_INPUT].value;
        currRoot = CoreUtil().getKeyFromVolts(fRoot);
    } else {
        currRoot = params[KEY_PARAM].value;
    }

    if (inputs[SCALE_INPUT].active) {
        float fScale = inputs[SCALE_INPUT].value;
        currScale = CoreUtil().getScaleFromVolts(fScale);
    } else {
        currScale = params[SCALE_PARAM].value;
    }

    float trans = (inputs[TRANS_INPUT].value + params[TRANS_PARAM].value) / 12.0;
    if (trans != 0.0) {
        if (trans != lastTrans) {
            int i;
            int d;
            trans = CoreUtil().getPitchFromVolts(trans, Core::NOTE_C, Core::SCALE_CHROMATIC, &i, &d);
            lastTrans = trans;
        } else {
            trans = lastTrans;
        }
    }

    for (int i = 0; i < 8; i++) {
        float holdInput     = inputs[HOLD_INPUT + i].value;
        bool  holdActive    = inputs[HOLD_INPUT + i].active;
        bool  holdStatus    = holdTrigger[i].process(holdInput);

        float volts = inputs[IN_INPUT + i].value;
        float shift = params[SHIFT_PARAM + i].value;

        if (holdActive) {

            // Sample the pitch
            if (holdStatus && inputs[IN_INPUT + i].active) {
                holdPitch[i] = CoreUtil().getPitchFromVolts(volts, currRoot, currScale, &currNote, &currDegree);
            }

        } else {

            if (inputs[IN_INPUT + i].active) {
                holdPitch[i] = CoreUtil().getPitchFromVolts(volts, currRoot, currScale, &currNote, &currDegree);
            }

        }

        // If the quantised pitch has changed
        if (lastPitch[i] != holdPitch[i]) {
            // Pulse the gate
            triggerPulse[i].trigger(Core::TRIGGER);

            // Record the pitch
            lastPitch[i] = holdPitch[i];
        }

        if (triggerPulse[i].process(delta)) {
            outputs[TRIG_OUTPUT + i].value = 10.0f;
        } else {
            outputs[TRIG_OUTPUT + i].value = 0.0f;
        }

        outputs[OUT_OUTPUT + i].value = holdPitch[i] + shift + trans;

    }

    // std::cout << "Currscale\n";
    // std::cout << scales[currScale];

    // std::cout << "Curroot\n";
    // std::cout << keys[currRoot];

    if (lastScale != currScale || firstStep) {
        for (int i = 0; i < Core::NUM_NOTES; i++) {
            lights[SCALE_LIGHT + i].value = 0.0f;
        }
        lights[SCALE_LIGHT + currScale].value = 1.0f;
    }

    if (lastRoot != currRoot || firstStep) {
        for (int i = 0; i < Core::NUM_NOTES; i++) {
            lights[KEY_LIGHT + i].value = 0.0f;
        }
        lights[KEY_LIGHT + currRoot].value = 1.0f;
    }

    firstStep = false;

}

struct uQuantWidget : ModuleWidget {
    uQuantWidget(uQuant *module);
};

uQuantWidget::uQuantWidget(uQuant *module) : ModuleWidget(module) {

    UI ui;

    //box.size = Vec(35, 380);
    box.size = Vec(300, 380);

    {
        SVGPanel *panel = new SVGPanel();
        panel->box.size = box.size;
        panel->setBackground(SVG::load(assetPlugin(plugin, "res/uQuant.svg")));
        addChild(panel);
    }

    addInput(Port::create<PJ301MPort>(ui.getPosition(UI::PORT, 0, 5, true, false), Port::INPUT, module, uQuant::KEY_INPUT));
    //addParam(ParamWidget::create<AHKnobSnap>(ui.getPosition(UI::PORT, 0, 0, true, false), module, uQuant::KEY_PARAM, 0.0f, 11.0f, 0.0f)); // 12 notes
    addParam(ParamWidget::create<AHTrimpotSnap>(Vec(9, 101), module, uQuant::KEY_PARAM, 0.0f, 11.0f, 0.0f)); // 12 notes

    addInput(Port::create<PJ301MPort>(ui.getPosition(UI::PORT, 3, 5, true, false), Port::INPUT, module, uQuant::SCALE_INPUT));
    addParam(ParamWidget::create<AHKnobSnap>(ui.getPosition(UI::PORT, 4, 5, true, false), module, uQuant::SCALE_PARAM, 0.0f, 11.0f, 0.0f)); // 12 notes

    addInput(Port::create<PJ301MPort>(ui.getPosition(UI::PORT, 6, 5, true, false), Port::INPUT, module, uQuant::TRANS_INPUT));
    addParam(ParamWidget::create<AHKnobSnap>(ui.getPosition(UI::PORT, 7, 5, true, false), module, uQuant::TRANS_PARAM, -11.0f, 11.0f, 0.0f)); // 12 notes

    for (int i = 0; i < 8; i++) {
        addInput(Port::create<PJ301MPort>(Vec(6 + i * 29, 41), Port::INPUT, module, uQuant::IN_INPUT + i));
        //addParam(ParamWidget::create<AHTrimpotSnap>(Vec(9 + i * 29.1, 101), module, uQuant::SHIFT_PARAM + i, -3.0f, 3.0f, 0.0f));
        addOutput(Port::create<PJ301MPort>(Vec(6 + i * 29, 125), Port::OUTPUT, module, uQuant::OUT_OUTPUT + i));
        //addInput(Port::create<PJ301MPort>(Vec(6 + i * 29, 71), Port::INPUT, module, uQuant::HOLD_INPUT + i));
        addOutput(Port::create<PJ301MPort>(Vec(6 + i * 29, 155), Port::OUTPUT, module, uQuant::TRIG_OUTPUT + i));
    }

    float xOffset = 18.0;
    float xSpace = 21.0;
    float xPos = 0.0;
    float yPos = 0.0;
    int scale = 0;

    // for (int i = 0; i < 12; i++) {
    //     addChild(ModuleLightWidget::create<SmallLight<GreenLight>>(Vec(xOffset + i * 18.0f, 280.0f), module, uQuant::SCALE_LIGHT + i));

    //     ui.calculateKeyboard(i, xSpace, xOffset, 230.0f, &xPos, &yPos, &scale);
    //     addChild(ModuleLightWidget::create<SmallLight<GreenLight>>(Vec(xPos, yPos), module, uQuant::KEY_LIGHT + scale));

    // }

    TinyStringDisplayWidget *displayKey = new TinyStringDisplayWidget();
    displayKey = new TinyStringDisplayWidget();
    displayKey->box.pos = Vec(6, 71);
    displayKey->box.size = Vec(25, 25);
    //displayKey->value = &module->keys[currRoot];
    displayKey->value = &module->keys[1];
    addChild(displayKey);

}

Model *modeluQuant = Model::create<uQuant, uQuantWidget>( "RJModules", "uQuant", "uQuant", QUANTIZER_TAG);

