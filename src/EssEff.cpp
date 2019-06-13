#include "RJModules.hpp"
#include "dsp/digital.hpp"
#include "osdialog.h"
#include "common.hpp"
#include <iostream>
#include <cmath>
#include <sstream>
#include <iomanip>
#include <unistd.h>
#include <mutex>

using namespace std;

#define TSF_IMPLEMENTATION
#include "tsf.h"

// It is suprisingly annoying to cross platform list files, do it manually. Boo.
int num_files = 3;
string soundfont_files[3] = {
    "/Users/rjones/Sources/Rack/plugins/RJModules/soundfonts/FluidR3GM.sf2",
    "/Users/rjones/Sources/Rack/plugins/RJModules/soundfonts/Wii_Grand_Piano.sf2",
    "/Users/rjones/Sources/Rack/plugins/RJModules/soundfonts/pyrex.sf2",
};

/*
Display
*/

struct SmallStringDisplayWidget : TransparentWidget {

  std::string *value;
  std::shared_ptr<Font> font;

  SmallStringDisplayWidget() {
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
    nvgFontSize(vg, 32);
    nvgFontFaceId(vg, font->handle);
    nvgTextLetterSpacing(vg, 2.5);

    std::stringstream to_display;
    to_display << std::setw(3) << *value;

    Vec textPos = Vec(16.0f, 33.0f);
    NVGcolor textColor = nvgRGB(0x00, 0x00, 0x00);
    nvgFillColor(vg, textColor);
    nvgText(vg, textPos.x, textPos.y, to_display.str().c_str(), NULL);
  }
};

/*
Widget
*/

struct EssEff : Module {
    enum ParamIds {
        FILE_PARAM,
        PRESET_PARAM,
        MOD_PARAM,
        BEND_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        VOCT_INPUT,
        GATE_INPUT,
        FILE_INPUT,
        PRESET_INPUT,
        NUM_INPUTS
    };
    enum OutputIds {
        MAIN_OUTPUT,
        NUM_OUTPUTS
    };
    enum LightIds {
        PHASE_POS_LIGHT,
        PHASE_NEG_LIGHT,
        NUM_LIGHTS
    };

    bool output_setting = false;
    bool output_set = false;

    bool loading = false;
    bool loaded = false;
    bool file_chosen = false;
    int last_file = -1;
    int last_note = -1;
    int last_preset_sel = -1;

    float frame[1000000];
    int head = -1;
    int current;

    std::string file_name = "Hello!";
    std::string preset_name = "Hello!";
    std::string last_path = "";

    SchmittTrigger gateTrigger;
    tsf* tee_ess_eff;

    EssEff() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {}
    void step() override;
    void loadFile(std::string path);

};

void EssEff::loadFile(std::string path){
    this->loaded = false;
    this->output_set = false;

    char cstr[path.size() + 1];
    strcpy(cstr, path.c_str());

    TSF_FREE(tee_ess_eff);
    memset(frame, 0, sizeof(frame));

    tee_ess_eff = tsf_load_filename(cstr);
    if(tee_ess_eff == TSF_NULL){
        return;
    }
    tsf_set_output(tee_ess_eff, TSF_MONO, engineGetSampleRate(), 0.0);

    this->loaded = true;
    this->loading = false;
    this->output_set = true;
    this->file_name = path;
}

void EssEff::step() {

    if(!file_chosen && !loading){
        int cur_file = params[FILE_PARAM].value;
        if(cur_file != last_file){
            this->loading = true;
            this->loadFile(soundfont_files[cur_file]);
            last_file = cur_file;
        }
    }

    if (!output_set || !loaded){
    } else {

        // Display
        int ps_count = tsf_get_presetcount(tee_ess_eff);
        int preset_sel = params[PRESET_PARAM].value;
        if (preset_sel >= ps_count){
            preset_sel = ps_count - 1;
        }
        preset_name = tsf_get_presetname(tee_ess_eff, preset_sel);

        // Render
        if (gateTrigger.process(inputs[GATE_INPUT].value)) {
            memset(frame, 0.0, sizeof(frame));
            int note;
            if (inputs[VOCT_INPUT].active){
                note = (int) std::round(inputs[VOCT_INPUT].value * 12.f + 60.f);
                note = clamp(note, 0, 127);
            } else{
                note = 60;
            }

            // make sure the last note is off
            if (last_note != -1 && last_preset_sel != -1){
                //tsf_note_off(tee_ess_eff, preset_sel, note);
                tsf_reset(tee_ess_eff);
            }

            // new note on
            tsf_note_on(tee_ess_eff, preset_sel, note, 1.0f);
            last_note = note;
            last_preset_sel = preset_sel;

            head = -1;
        }

        if ( head < 0 || head >= 1000000 ){
            head = 0;
            tsf_render_float(tee_ess_eff, frame, 1000000, 0);
        } else{
            head++;
        }

        outputs[MAIN_OUTPUT].value  = frame[head];
    }
}


struct EssEffWidget: ModuleWidget {
    EssEffWidget(EssEff *module);
    Menu *createContextMenu() override;
};

EssEffWidget::EssEffWidget(EssEff *module) : ModuleWidget(module) {
    box.size = Vec(15*10, 380);

    {
        SVGPanel *panel = new SVGPanel();
        panel->box.size = box.size;
        panel->setBackground(SVG::load(assetPlugin(plugin, "res/EssEff.svg")));
        addChild(panel);
    }

    addChild(Widget::create<ScrewSilver>(Vec(15, 0)));
    addChild(Widget::create<ScrewSilver>(Vec(box.size.x-30, 0)));
    addChild(Widget::create<ScrewSilver>(Vec(15, 365)));
    addChild(Widget::create<ScrewSilver>(Vec(box.size.x-30, 365)));

    // Displays
    SmallStringDisplayWidget *fileDisplay = new SmallStringDisplayWidget();
    fileDisplay->box.pos = Vec(28, 70);
    fileDisplay->box.size = Vec(100, 40);
    fileDisplay->value = &module->file_name;
    addChild(fileDisplay);

    SmallStringDisplayWidget *presetDisplay = new SmallStringDisplayWidget();
    presetDisplay->box.pos = Vec(28, 170);
    presetDisplay->box.size = Vec(100, 40);
    presetDisplay->value = &module->preset_name;
    addChild(presetDisplay);

    // Knobs
    addParam(ParamWidget::create<RoundBlackSnapKnob>(Vec(85, 115), module, EssEff::FILE_PARAM, 0, num_files - 1, 0));
    addParam(ParamWidget::create<RoundBlackSnapKnob>(Vec(85, 215), module, EssEff::PRESET_PARAM, 0, 320, 0));
    addInput(Port::create<PJ301MPort>(Vec(37, 117.5), Port::INPUT, module, EssEff::FILE_INPUT));
    addInput(Port::create<PJ301MPort>(Vec(37, 217.5), Port::INPUT, module, EssEff::PRESET_INPUT));

    addParam(ParamWidget::create<RoundBlackKnob>(Vec(37, 262), module, EssEff::MOD_PARAM, 0.0, 1.0, 0.0));
    addParam(ParamWidget::create<RoundBlackKnob>(Vec(85, 262), module, EssEff::BEND_PARAM, -1.0, 1.0, 0.0));

    // Inputs and Knobs
    addInput(Port::create<PJ301MPort>(Vec(16, 320), Port::INPUT, module, EssEff::VOCT_INPUT));
    addInput(Port::create<PJ301MPort>(Vec(53, 320), Port::INPUT, module, EssEff::GATE_INPUT));
    addOutput(Port::create<PJ301MPort>(Vec(112.5, 320), Port::OUTPUT, module, EssEff::MAIN_OUTPUT));
}

struct EssEffItem : MenuItem {
    EssEff *player;
    void onAction(EventAction &e) override {

        std::string dir = player->last_path.empty() ? assetLocal("") : stringDirectory(player->last_path);
        char *path = osdialog_file(OSDIALOG_OPEN, dir.c_str(), NULL, NULL);
        if (path) {
            player->loadFile(path);
            player->last_path = path;
            player->file_chosen = true;
            free(path);
        }
    }
};

Menu *EssEffWidget::createContextMenu() {
    Menu *menu = ModuleWidget::createContextMenu();

    MenuLabel *spacerLabel = new MenuLabel();
    menu->addChild(spacerLabel);

    EssEff *player = dynamic_cast<EssEff*>(module);
    assert(player);

    EssEffItem *sampleItem = new EssEffItem();
    sampleItem->text = "Load .sf2 file";
    sampleItem->player = player;
    menu->addChild(sampleItem);

    return menu;
}

Model *modelEssEff = Model::create<EssEff, EssEffWidget>("RJModules", "EssEff", "[GEN] EssEff", LFO_TAG);
