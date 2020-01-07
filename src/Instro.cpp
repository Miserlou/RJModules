#include "RJModules.hpp"
#include "common.hpp"

#include "Rhodey.h"
#include "Flute.h"
#include "Brass.h"
#include "Saxofony.h"
#include "BlowBotl.h"
#include "BlowHole.h"
#include "Bowed.h"
#include "Clarinet.h"
#include "Drummer.h"
#include "FMVoices.h"
#include "Guitar.h"
#include "HevyMetl.h"
#include "Mandolin.h"
#include "ModalBar.h"
#include "Moog.h"
#include "PercFlut.h"
#include "Plucked.h"
#include "Recorder.h"
#include "Saxofony.h"
#include "Shakers.h"
#include "Sitar.h"
#include "StifKarp.h"
#include "TubeBell.h"
#include "Twang.h"
#include "Whistle.h"
#include "Wurley.h"

#include <iostream>
#include <cmath>
#include <sstream>
#include <iomanip>
#include <unistd.h>
#include <mutex>

using namespace std;
#define HISTORY_SIZE (1<<21)

/*
Display
*/

struct InstroSmallStringDisplayWidget : TransparentWidget {

  std::string *value;
  std::shared_ptr<Font> font;

  InstroSmallStringDisplayWidget() {
    font = Font::load(assetPlugin(pluginInstance, "res/Pokemon.ttf"));
  };

  void draw(NVGcontext *vg) override
  {

    // Shadow
    NVGcolor backgroundColorS = nvgRGB(0xA0, 0xA0, 0xA0);
    nvgBeginPath(vg);
    nvgRoundedRect(vg, 0.0, 0.0, box.size.x, box.size.y + 2.0, 4.0);
    nvgFillColor(vg, backgroundColorS);
    nvgFill(vg);

    // Background
    NVGcolor backgroundColor = nvgRGB(0xC0, 0xC0, 0xC0);
    nvgBeginPath(vg);
    nvgRoundedRect(vg, 0.0, 0.0, box.size.x, box.size.y, 4.0);
    nvgFillColor(vg, backgroundColor);
    nvgFill(vg);

    // text
    nvgFontSize(vg, 20);
    nvgFontFaceId(vg, font->handle);
    nvgTextLetterSpacing(vg, 0.4);

    std::stringstream to_display;
    to_display << std::setw(3) << *value;

    Vec textPos = Vec(12.0f, 28.0f);
    NVGcolor textColor = nvgRGB(0x00, 0x00, 0x00);
    nvgFillColor(vg, textColor);
    nvgText(vg, textPos.x, textPos.y, to_display.str().c_str(), NULL);
  }
};

struct InstroRoundLargeBlackKnob : RoundLargeBlackKnob
{
    InstroRoundLargeBlackKnob()
    {
        setSVG(SVG::load(assetPlugin(pluginInstance, "res/KTFRoundHugeBlackKnob.svg")));
    }
};

struct InstroRoundBlackSnapKnob : RoundBlackKnob
{
    InstroRoundBlackSnapKnob()
    {
        setSVG(SVG::load(assetPlugin(pluginInstance, "res/KTFRoundLargeBlackKnob.svg")));
        minAngle = -0.83 * M_PI;
        maxAngle = 0.83 * M_PI;
        snap = true;
    }
};

/*
Widget
*/

struct Instro : Module {
    enum ParamIds {
        INSTRO_PARAM,
        PARAM_1,
        PARAM_2,
        PARAM_3,
        PARAM_4,
        NUM_PARAMS
    };
    enum InputIds {
        IN_INPUT,
        GATE_INPUT,

        TIME_INPUT,
        FEEDBACK_INPUT,
        COLOR_INPUT,
        COLOR_INPUT_RIGHT,

        COLOR_RETURN,
        COLOR_RETURN_RIGHT,

        MIX_INPUT,
        BYPASS_CV_INPUT,

        NUM_INPUTS
    };
    enum OutputIds {
        LEFT_OUTPUT,
        RIGHT_OUTPUT,

        COLOR_SEND,
        COLOR_SEND_RIGHT,

        NUM_OUTPUTS
    };
    enum LightIds {
        NUM_LIGHTS
    };

    // Display
    std::string voice_display = "Flute";

    // State
    bool note_on = false;

    // Instros
    stk::Flute flute = stk::Flute(60.0f);
    stk::Rhodey rhodey;
    stk::Brass brass;
    stk::Saxofony saxofony = stk::Saxofony(60.0f);
    stk::BlowBotl blowbotl = stk::BlowBotl();
    stk::BlowHole blowhole = stk::BlowHole(60.0f);
    stk::Bowed bowed = stk::Bowed(60.0f);
    stk::Clarinet clarinet = stk::Clarinet(60.0f);
    stk::Drummer drummer = stk::Drummer();
    stk::FMVoices fmvoices = stk::FMVoices();
    stk::Guitar guitar = stk::Guitar();
    stk::HevyMetl heavy = stk::HevyMetl();
    stk::Mandolin mandolin = stk::Mandolin(60.0f);
    stk::ModalBar modalbar = stk::ModalBar();
    stk::Moog moog = stk::Moog();
    stk::PercFlut percflut = stk::PercFlut();
    stk::Plucked plucked = stk::Plucked(60.0f);
    stk::Recorder recorder = stk::Recorder();
    stk::Shakers shakers = stk::Shakers();
    stk::Sitar sitar = stk::Sitar();
    stk::StifKarp stifkarp = stk::StifKarp();
    stk::TubeBell tubebell = stk::TubeBell();
    stk::Whistle whistle = stk::Whistle();
    stk::Wurley wurley = stk::Wurley();

    Instro() {

        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(Instro::INSTRO_PARAM, 0, 23, 0, "Instrument");
        configParam(Instro::PARAM_1, 0.0f, 1.0f, 0.5f, "Param 1");
        configParam(Instro::PARAM_2, 0.0f, 1.0f, 0.5f, "Param 2");
        configParam(Instro::PARAM_3, 0.0f, 1.0f, 0.5f, "Param 3");
        configParam(Instro::PARAM_4, 0.0f, 1.0f, 0.5f, "Param 4");
    }

    // Pitchies
    float referenceFrequency = 261.626; // C4; frequency at which Rack 1v/octave CVs are zero.
    float referenceSemitone = 60.0; // C4; value of C4 in semitones is arbitrary here, so have it match midi note numbers when rounded to integer.
    float twelfthRootTwo = 1.0594630943592953;
    float logTwelfthRootTwo = logf(1.0594630943592953);
    int referencePitch = 0;
    int referenceOctave = 4;

    float cvToFrequency(float cv) {
        return powf(2.0, cv) * referenceFrequency;
    }

    void process(const ProcessArgs &args) override {

        float voct = inputs[IN_INPUT].value;
        float processed = 0.0;
        int  instrument_choice = params[INSTRO_PARAM].value;

        // parameters
        float param_1 = params[PARAM_1].value;
        float param_2 = params[PARAM_2].value;
        float param_3 = params[PARAM_3].value;
        float param_4 = params[PARAM_4].value;

        // gate
        bool gate_connected = inputs[GATE_INPUT].isConnected();
        float gate_value = inputs[GATE_INPUT].value;
        bool turn_note_on = false;
        bool turn_note_off = false;
        if(gate_connected){
            if(note_on && gate_value == 0.0){
                turn_note_off = true;
            }
            if(!note_on && gate_value != 0.0){
                turn_note_on = true;
            }
        }

        if(instrument_choice == 0){
            if(!gate_connected){
                flute.noteOn(cvToFrequency(voct), 1.0);
            } else{
                if(turn_note_on){
                    flute.noteOn(cvToFrequency(voct), 1.0);
                    note_on = true;
                } else if (turn_note_off){
                    flute.noteOff(1.0);
                    note_on = false;
                }
            }
            processed = flute.tick( );
            voice_display = "Flute";
        } else if (instrument_choice == 1){
            rhodey.noteOn(cvToFrequency(voct), 1.0);
            processed = rhodey.tick( );
            voice_display = "Rhodes";
        } else if (instrument_choice == 2){
            brass.noteOn(cvToFrequency(voct), 1.0);
            processed = brass.tick( );
            voice_display = "Brass";
        } else if (instrument_choice == 3){
            saxofony.noteOn(cvToFrequency(voct), 1.0);
            processed = saxofony.tick( );
            voice_display = "Sax";
        } else if (instrument_choice == 4){
            blowbotl.noteOn(cvToFrequency(voct), 1.0);
            processed = blowbotl.tick( );
            voice_display = "Bottle";
        } else if (instrument_choice == 5){
            blowhole.noteOn(cvToFrequency(voct), 1.0);
            processed = blowhole.tick( );
            voice_display = "Hole";
        } else if (instrument_choice == 6){
            bowed.noteOn(cvToFrequency(voct), 1.0);
            processed = bowed.tick( );
            voice_display = "Bow";
        } else if (instrument_choice == 7){
            clarinet.noteOn(cvToFrequency(voct), 1.0);
            processed = bowed.tick( );
            voice_display = "Clarinet";
        } else if (instrument_choice == 8){
            drummer.noteOn(cvToFrequency(voct), 1.0);
            processed = drummer.tick( );
            voice_display = "Drums";
        } else if (instrument_choice == 9){
            fmvoices.noteOn(cvToFrequency(voct), 1.0);
            processed = fmvoices.tick( );
            voice_display = "Voices";
        } else if (instrument_choice == 10){
            guitar.noteOn(cvToFrequency(voct), 1.0);
            processed = guitar.tick( );
            voice_display = "Guitar";
        } else if (instrument_choice == 11){
            heavy.noteOn(cvToFrequency(voct), 1.0);
            processed = heavy.tick( );
            voice_display = "Heavy";
        } else if (instrument_choice == 12){
            mandolin.noteOn(cvToFrequency(voct), 1.0);
            processed = mandolin.tick( );
            voice_display = "Mandolin";
        } else if (instrument_choice == 13){
            modalbar.noteOn(cvToFrequency(voct), 1.0);
            processed = modalbar.tick( );
            voice_display = "Bar";
        } else if (instrument_choice == 14){
            moog.noteOn(cvToFrequency(voct), 1.0);
            processed = moog.tick( );
            voice_display = "Moog";
        } else if (instrument_choice == 15){
            percflut.noteOn(cvToFrequency(voct), 1.0);
            processed = percflut.tick( );
            voice_display = "Flute 2";
        } else if (instrument_choice == 16){
            plucked.noteOn(cvToFrequency(voct), 1.0);
            processed = plucked.tick( );
            voice_display = "Plucked";
        } else if (instrument_choice == 17){
            recorder.noteOn(cvToFrequency(voct), 1.0);
            processed = recorder.tick( );
            voice_display = "Recrdr";
        } else if (instrument_choice == 18){
            shakers.noteOn(cvToFrequency(voct), 1.0);
            processed = shakers.tick( );
            voice_display = "Shakers";
        } else if (instrument_choice == 19){
            sitar.noteOn(cvToFrequency(voct), 1.0);
            processed = sitar.tick( );
            voice_display = "Sitar";
        } else if (instrument_choice == 20){
            stifkarp.noteOn(cvToFrequency(voct), 1.0);
            processed = stifkarp.tick( );
            voice_display = "StfKarp";
        } else if (instrument_choice == 21){
            tubebell.noteOn(cvToFrequency(voct), 1.0);
            processed = tubebell.tick( );
            voice_display = "TbBell";
        } else if (instrument_choice == 22){
            whistle.noteOn(cvToFrequency(voct), 1.0);
            processed = whistle.tick( );
            voice_display = "Whistl";
        } else if (instrument_choice == 23){
            wurley.noteOn(cvToFrequency(voct), 1.0);
            processed = wurley.tick( );
            voice_display = "Wurley";
        }

        outputs[RIGHT_OUTPUT].value = processed;

    }
};

struct InstroWidget : ModuleWidget {
  InstroWidget(Instro *module) {
    setModule(module);
    box.size = Vec(15*10, 380);

    SVGPanel *panel = new SVGPanel();
    panel->box.size = box.size;
    panel->setBackground(SVG::load(assetPlugin(pluginInstance, "res/Instro.svg")));
    addChild(panel);

    // Displays
    if(module != NULL){
        InstroSmallStringDisplayWidget *fileDisplay = new InstroSmallStringDisplayWidget();
        fileDisplay->box.pos = Vec(20, 50);
        fileDisplay->box.size = Vec(70, 40);
        fileDisplay->value = &module->voice_display;
        addChild(fileDisplay);
    }

    // Knobs
    int LEFT = 14;
    int RIGHT = 65;
    int DIST = 82;
    int BASE = 115;
    addParam(createParam<InstroRoundBlackSnapKnob>(Vec(100, 50), module, Instro::INSTRO_PARAM));
    addParam(createParam<InstroRoundLargeBlackKnob>(Vec(LEFT, BASE), module, Instro::PARAM_1));
    addParam(createParam<InstroRoundLargeBlackKnob>(Vec(LEFT + RIGHT, BASE), module, Instro::PARAM_2));
    addParam(createParam<InstroRoundLargeBlackKnob>(Vec(LEFT, BASE + DIST), module, Instro::PARAM_3));
    addParam(createParam<InstroRoundLargeBlackKnob>(Vec(LEFT + RIGHT, BASE + DIST), module, Instro::PARAM_4));

    // Inputs and Knobs
    addOutput(createPort<PJ301MPort>(Vec(11, 277), PortWidget::OUTPUT, module, Instro::COLOR_SEND));
    addInput(createPort<PJ301MPort>(Vec(45, 277), PortWidget::INPUT, module, Instro::COLOR_RETURN));
    addOutput(createPort<PJ301MPort>(Vec(80, 277), PortWidget::OUTPUT, module, Instro::COLOR_SEND_RIGHT));
    addInput(createPort<PJ301MPort>(Vec(112.5, 277), PortWidget::INPUT, module, Instro::COLOR_RETURN_RIGHT));

    addInput(createPort<PJ301MPort>(Vec(11, 320), PortWidget::INPUT, module, Instro::IN_INPUT));
    addInput(createPort<PJ301MPort>(Vec(45, 320), PortWidget::INPUT, module, Instro::GATE_INPUT));
    // addOutput(createPort<PJ301MPort>(Vec(80, 320), PortWidget::OUTPUT, module, Instro::LEFT_OUTPUT));
    addOutput(createPort<PJ301MPort>(Vec(112.5, 320), PortWidget::OUTPUT, module, Instro::RIGHT_OUTPUT));
    }

};

Model *modelInstro = createModel<Instro, InstroWidget>("Instro");
