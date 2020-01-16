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

        PARAM_1_CV,
        PARAM_2_CV,
        PARAM_3_CV,
        PARAM_4_CV,

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
    std::string voice_display = "Rhodes";

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
        configParam(Instro::PARAM_1, 0, 128, 1, "Param 1");
        configParam(Instro::PARAM_2, 0, 128, 1, "Param 2");
        configParam(Instro::PARAM_3, 0, 128, 1, "Param 3");
        configParam(Instro::PARAM_4, 0, 128, 1, "Param 4");
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
        float param_1 = params[PARAM_1].value * rescale(inputs[PARAM_1_CV].normalize(1.0f), 0.f, 5.f, 0.f, 1.f);
        float param_2 = params[PARAM_2].value * rescale(inputs[PARAM_2_CV].normalize(1.0f), 0.f, 5.f, 0.f, 1.f);
        float param_3 = params[PARAM_3].value * rescale(inputs[PARAM_3_CV].normalize(1.0f), 0.f, 5.f, 0.f, 1.f);
        float param_4 = params[PARAM_4].value * rescale(inputs[PARAM_4_CV].normalize(1.0f), 0.f, 5.f, 0.f, 1.f);

        // gate
        bool gate_connected = inputs[GATE_INPUT].isConnected();
        float gate_value = inputs[GATE_INPUT].value;
        bool turn_note_on = false;
        bool turn_note_off = false;
        if(gate_connected){
            if(note_on && gate_value == 0.0){
                turn_note_off = true;
                turn_note_on = false;
            }
            if(!note_on && gate_value != 0.0){
                turn_note_on = true;
                turn_note_off = false;
            }
        }

        if(instrument_choice == 1){
            // Control
            flute.controlChange(2, param_1);
            flute.controlChange(4, param_2);
            flute.controlChange(1, param_3);
            flute.controlChange(11, param_4);

            // Gating
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

            // Tick
            processed = flute.tick( );
            voice_display = "Flute";
        } else if (instrument_choice == 0){
            // Control
            rhodey.controlChange(2, param_1);
            rhodey.controlChange(4, param_2);
            rhodey.controlChange(11, param_3);
            rhodey.controlChange(1, param_4);

            // Gating
            if(!gate_connected){
                rhodey.noteOn(cvToFrequency(voct), 1.0);
            } else{
                if(turn_note_on){
                    rhodey.noteOn(cvToFrequency(voct), 1.0);
                    note_on = true;
                } else if (turn_note_off){
                    rhodey.noteOff(1.0);
                    note_on = false;
                }
            }

            processed = rhodey.tick( );
            voice_display = "Rhodes";
        } else if (instrument_choice == 2){
            // Control
            brass.controlChange(2, param_1);
            brass.controlChange(4, param_2);
            brass.controlChange(11, param_3);
            brass.controlChange(1, param_4);

            // Gating
            if(!gate_connected){
                brass.noteOn(cvToFrequency(voct), 1.0);
            } else{
                if(turn_note_on){
                    brass.noteOn(cvToFrequency(voct), 1.0);
                    note_on = true;
                } else if (turn_note_off){
                    brass.noteOff(1.0);
                    note_on = false;
                }
            }
            processed = brass.tick( );
            voice_display = "Brass";
        } else if (instrument_choice == 3){
            // Control
            saxofony.controlChange(29, param_1);
            saxofony.controlChange(4, param_2);
            saxofony.controlChange(11, param_3);
            saxofony.controlChange(1, param_4);

            // Gating
            if(!gate_connected){
                saxofony.noteOn(cvToFrequency(voct), 1.0);
            } else{
                if(turn_note_on){
                    saxofony.noteOn(cvToFrequency(voct), 1.0);
                    note_on = true;
                } else if (turn_note_off){
                    saxofony.noteOff(1.0);
                    note_on = false;
                }
            }
            processed = saxofony.tick( );
            voice_display = "Sax";
        } else if (instrument_choice == 4){
            // Control
            blowbotl.controlChange(4, param_1);
            blowbotl.controlChange(11, param_2);
            blowbotl.controlChange(1, param_3);
            blowbotl.controlChange(128, param_4);

            // Gating
            if(!gate_connected){
                blowbotl.noteOn(cvToFrequency(voct), 1.0);
            } else{
                if(turn_note_on){
                    blowbotl.noteOn(cvToFrequency(voct), 1.0);
                    note_on = true;
                } else if (turn_note_off){
                    blowbotl.noteOff(1.0);
                    note_on = false;
                }
            }
            processed = blowbotl.tick( );
            voice_display = "Bottle";
        } else if (instrument_choice == 5){
            // Control
            blowhole.controlChange(2, param_1);
            blowhole.controlChange(4, param_2);
            blowhole.controlChange(11, param_3);
            blowhole.controlChange(1, param_4);

            // Gating
            if(!gate_connected){
                blowhole.noteOn(cvToFrequency(voct), 1.0);
            } else{
                if(turn_note_on){
                    blowhole.noteOn(cvToFrequency(voct), 1.0);
                    note_on = true;
                } else if (turn_note_off){
                    blowhole.noteOff(1.0);
                    note_on = false;
                }
            }
            processed = blowhole.tick( );
            voice_display = "Hole";
        } else if (instrument_choice == 6){
            // Control
            bowed.controlChange(2, param_1);
            bowed.controlChange(4, param_2);
            bowed.controlChange(11, param_3);
            bowed.controlChange(1, param_4);

            // Gating
            if(!gate_connected){
                bowed.noteOn(cvToFrequency(voct), 1.0);
            } else{
                if(turn_note_on){
                    bowed.noteOn(cvToFrequency(voct), 1.0);
                    note_on = true;
                } else if (turn_note_off){
                    bowed.noteOff(1.0);
                    note_on = false;
                }
            }
            processed = bowed.tick( );
            voice_display = "Bow";
        } else if (instrument_choice == 7){
            // Control
            clarinet.controlChange(2, param_1);
            clarinet.controlChange(4, param_2);
            clarinet.controlChange(11, param_3);
            clarinet.controlChange(1, param_4);

            // Gating
            if(!gate_connected){
                clarinet.noteOn(cvToFrequency(voct), 1.0);
            } else{
                if(turn_note_on){
                    clarinet.noteOn(cvToFrequency(voct), 1.0);
                    note_on = true;
                } else if (turn_note_off){
                    clarinet.noteOff(1.0);
                    note_on = false;
                }
            }
            processed = clarinet.tick( );
            voice_display = "Clarinet";
        } else if (instrument_choice == 8){
            // Control
            wurley.controlChange(2, param_1);
            wurley.controlChange(4, param_2);
            wurley.controlChange(11, param_3);
            wurley.controlChange(1, param_4);

            // Gating
            if(!gate_connected){
                wurley.noteOn(cvToFrequency(voct), 1.0);
            } else{
                if(turn_note_on){
                    wurley.noteOn(cvToFrequency(voct), 1.0);
                    note_on = true;
                } else if (turn_note_off){
                    wurley.noteOff(1.0);
                    note_on = false;
                }
            }
            processed = wurley.tick( );
            voice_display = "Wurley";
        } else if (instrument_choice == 9){
            // Control
            fmvoices.controlChange(4, param_1);
            fmvoices.controlChange(2, param_2);
            fmvoices.controlChange(11, param_3);
            fmvoices.controlChange(1, param_4);

            // Gating
            if(!gate_connected){
                fmvoices.noteOn(cvToFrequency(voct), 1.0);
            } else{
                if(turn_note_on){
                    fmvoices.noteOn(cvToFrequency(voct), 1.0);
                    note_on = true;
                } else if (turn_note_off){
                    fmvoices.noteOff(1.0);
                    note_on = false;
                }
            }
            processed = fmvoices.tick( );
            voice_display = "Voices";
        } else if (instrument_choice == 10){
            // Control
            guitar.controlChange(4, param_1);
            guitar.controlChange(2, param_2);
            guitar.controlChange(11, param_3);
            guitar.controlChange(1, param_4);

            // Gating
            if(!gate_connected){
                guitar.noteOn(cvToFrequency(voct), 1.0);
            } else{
                if(turn_note_on){
                    guitar.noteOn(cvToFrequency(voct), 1.0);
                    note_on = true;
                } else if (turn_note_off){
                    guitar.noteOff(1.0);
                    note_on = false;
                }
            }
            processed = guitar.tick( );
            voice_display = "Guitar";
        } else if (instrument_choice == 11){
            // Control
            heavy.controlChange(2, param_1);
            heavy.controlChange(4, param_2);
            heavy.controlChange(11, param_3);
            heavy.controlChange(1, param_4);

            // Gating
            if(!gate_connected){
                heavy.noteOn(cvToFrequency(voct), 1.0);
            } else{
                if(turn_note_on){
                    heavy.noteOn(cvToFrequency(voct), 1.0);
                    note_on = true;
                } else if (turn_note_off){
                    heavy.noteOff(1.0);
                    note_on = false;
                }
            }
            processed = heavy.tick( );
            voice_display = "Heavy";
        } else if (instrument_choice == 12){
            // Control
            mandolin.controlChange(2, param_1);
            mandolin.controlChange(4, param_2);
            mandolin.controlChange(11, param_3);
            mandolin.controlChange(1, param_4);

            // Gating
            if(!gate_connected){
                mandolin.noteOn(cvToFrequency(voct), 1.0);
            } else{
                if(turn_note_on){
                    mandolin.noteOn(cvToFrequency(voct), 1.0);
                    note_on = true;
                } else if (turn_note_off){
                    mandolin.noteOff(1.0);
                    note_on = false;
                }
            }
            processed = mandolin.tick( );
            voice_display = "Mandolin";
        } else if (instrument_choice == 13){
            // Control
            modalbar.controlChange(2, param_1);
            modalbar.controlChange(4, param_2);
            modalbar.controlChange(8, param_3);
            modalbar.controlChange(11, param_4);

            // Gating
            if(!gate_connected){
                modalbar.noteOn(cvToFrequency(voct), 1.0);
            } else{
                if(turn_note_on){
                    modalbar.noteOn(cvToFrequency(voct), 1.0);
                    note_on = true;
                } else if (turn_note_off){
                    modalbar.noteOff(1.0);
                    note_on = false;
                }
            }
            processed = modalbar.tick( );
            voice_display = "Bar";
        } else if (instrument_choice == 14){
            // Control
            moog.controlChange(2, param_1);
            moog.controlChange(4, param_2);
            moog.controlChange(1, param_3);
            moog.controlChange(11, param_4);

            // Gating
            if(!gate_connected){
                moog.noteOn(cvToFrequency(voct), 1.0);
            } else{
                if(turn_note_on){
                    moog.noteOn(cvToFrequency(voct), 1.0);
                    note_on = true;
                } else if (turn_note_off){
                    moog.noteOff(1.0);
                    note_on = false;
                }
            }
            processed = moog.tick( );
            voice_display = "Moog";
        } else if (instrument_choice == 15){
            // Control
            percflut.controlChange(2, param_1);
            percflut.controlChange(4, param_2);
            percflut.controlChange(11, param_3);
            percflut.controlChange(1, param_4);

            // Gating
            if(!gate_connected){
                percflut.noteOn(cvToFrequency(voct), 1.0);
            } else{
                if(turn_note_on){
                    percflut.noteOn(cvToFrequency(voct), 1.0);
                    note_on = true;
                } else if (turn_note_off){
                    percflut.noteOff(1.0);
                    note_on = false;
                }
            }
            processed = percflut.tick( );
            voice_display = "Flute 2";
        } else if (instrument_choice == 16){
            // Control
            // plucked.controlChange(2, param_1);
            // plucked.controlChange(4, param_2);
            // plucked.controlChange(11, param_3);
            // plucked.controlChange(1, param_4);

            // // Gating
            // if(!gate_connected){
            //     plucked.noteOn(cvToFrequency(voct), 1.0);
            // } else{
            //     if(turn_note_on){
            //         plucked.noteOn(cvToFrequency(voct), 1.0);
            //         note_on = true;
            //     } else if (turn_note_off){
            //         plucked.noteOff(1.0);
            //         note_on = false;
            //     }
            // }
            // processed = plucked.tick( );
            // voice_display = "Plucked";
            voice_display = "XXX";
        } else if (instrument_choice == 17){
            // Control
            // recorder.controlChange(2, param_1);
            // recorder.controlChange(4, param_2);
            // recorder.controlChange(16, param_3);
            // recorder.controlChange(11, param_4);

            // // Gating
            // if(!gate_connected){
            //     recorder.noteOn(cvToFrequency(voct), 1.0);
            // } else{
            //     if(turn_note_on){
            //         recorder.noteOn(cvToFrequency(voct), 1.0);
            //         note_on = true;
            //     } else if (turn_note_off){
            //         recorder.noteOff(1.0);
            //         note_on = false;
            //     }
            // }
            // processed = recorder.tick( );
            voice_display = "XXX";
        } else if (instrument_choice == 18){
            // Control
            shakers.controlChange(2, param_1);
            shakers.controlChange(4, param_2);
            shakers.controlChange(11, param_3);
            shakers.controlChange(1, param_4);

            // Gating
            if(!gate_connected){
                shakers.noteOn(cvToFrequency(voct), 1.0);
            } else{
                if(turn_note_on){
                    shakers.noteOn(cvToFrequency(voct), 1.0);
                    note_on = true;
                } else if (turn_note_off){
                    shakers.noteOff(1.0);
                    note_on = false;
                }
            }
            processed = shakers.tick( );
            voice_display = "Shakers";
        } else if (instrument_choice == 19){
            // Control
            sitar.controlChange(2, param_1);
            sitar.controlChange(4, param_2);
            sitar.controlChange(11, param_3);
            sitar.controlChange(1, param_4);

            // Gating
            if(!gate_connected){
                sitar.noteOn(cvToFrequency(voct), 1.0);
            } else{
                if(turn_note_on){
                    sitar.noteOn(cvToFrequency(voct), 1.0);
                    note_on = true;
                } else if (turn_note_off){
                    sitar.noteOff(1.0);
                    note_on = false;
                }
            }
            processed = sitar.tick( );
            voice_display = "Sitar";
        } else if (instrument_choice == 20){
            // Control
            stifkarp.controlChange(2, param_1);
            stifkarp.controlChange(4, param_2);
            stifkarp.controlChange(11, param_3);
            stifkarp.controlChange(1, param_4);

            // Gating
            if(!gate_connected){
                stifkarp.noteOn(cvToFrequency(voct), 1.0);
            } else{
                if(turn_note_on){
                    stifkarp.noteOn(cvToFrequency(voct), 1.0);
                    note_on = true;
                } else if (turn_note_off){
                    stifkarp.noteOff(1.0);
                    note_on = false;
                }
            }
            processed = stifkarp.tick( );
            voice_display = "StfKarp";
        } else if (instrument_choice == 21){
            // Control
            tubebell.controlChange(2, param_1);
            tubebell.controlChange(4, param_2);
            tubebell.controlChange(11, param_3);
            tubebell.controlChange(1, param_4);

            // Gating
            if(!gate_connected){
                tubebell.noteOn(cvToFrequency(voct), 1.0);
            } else{
                if(turn_note_on){
                    tubebell.noteOn(cvToFrequency(voct), 1.0);
                    note_on = true;
                } else if (turn_note_off){
                    tubebell.noteOff(1.0);
                    note_on = false;
                }
            }
            processed = tubebell.tick( );
            voice_display = "TbBell";
        } else if (instrument_choice == 22){
            // Control
            whistle.controlChange(2, param_1);
            whistle.controlChange(4, param_2);
            whistle.controlChange(11, param_3);
            whistle.controlChange(1, param_4);

            // Gating
            if(!gate_connected){
                whistle.noteOn(cvToFrequency(voct), 1.0);
            } else{
                if(turn_note_on){
                    whistle.noteOn(cvToFrequency(voct), 1.0);
                    note_on = true;
                } else if (turn_note_off){
                    whistle.noteOff(1.0);
                    note_on = false;
                }
            }
            processed = whistle.tick( );
            voice_display = "Whistl";
        } else if (instrument_choice == 23){
            // Gating
            if(!gate_connected){
                drummer.noteOn(cvToFrequency(voct), 1.0);
            } else{
                if(turn_note_on){
                    drummer.noteOn(cvToFrequency(voct), 1.0);
                    note_on = true;
                } else if (turn_note_off){
                    drummer.noteOff(1.0);
                    note_on = false;
                }
            }
            processed = drummer.tick( );
            voice_display = "Drums";
        }

        outputs[RIGHT_OUTPUT].value = processed * 3; // Boost as default volumes are too low

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
    addInput(createPort<PJ301MPort>(Vec(11, 277), PortWidget::INPUT, module, Instro::PARAM_1_CV));
    addInput(createPort<PJ301MPort>(Vec(45, 277), PortWidget::INPUT, module, Instro::PARAM_2_CV));
    addInput(createPort<PJ301MPort>(Vec(80, 277), PortWidget::INPUT, module, Instro::PARAM_3_CV));
    addInput(createPort<PJ301MPort>(Vec(112.5, 277), PortWidget::INPUT, module, Instro::PARAM_4_CV));

    addInput(createPort<PJ301MPort>(Vec(11, 320), PortWidget::INPUT, module, Instro::IN_INPUT));
    addInput(createPort<PJ301MPort>(Vec(45, 320), PortWidget::INPUT, module, Instro::GATE_INPUT));
    // addOutput(createPort<PJ301MPort>(Vec(80, 320), PortWidget::OUTPUT, module, Instro::LEFT_OUTPUT));
    addOutput(createPort<PJ301MPort>(Vec(112.5, 320), PortWidget::OUTPUT, module, Instro::RIGHT_OUTPUT));
    }

};

Model *modelInstro = createModel<Instro, InstroWidget>("Instro");
