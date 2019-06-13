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
        OFFSET_PARAM,
        INVERT_PARAM,
        FREQ_PARAM,
        FM1_PARAM,
        FM2_PARAM,
        PW_PARAM,
        PWM_PARAM,
        NUM_PARAMS,
        CH1_PARAM,
        CH2_PARAM,
    };
    enum InputIds {
        VOCT_INPUT,
        GATE_INPUT,
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

    float frame[1000000];
    int head = -1;
    int current;

    std::string file_name = "Hello!";
    std::string preset_name = "Hello!";
    std::string last_path = "";

    tsf* tee_ess_eff;

    EssEff() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {}
    void step() override;
    void loadFile(std::string path);

};

void EssEff::loadFile(std::string path){
    this->loaded = false;

    char cstr[path.size() + 1];
    strcpy(cstr, path.c_str());

    tee_ess_eff = tsf_load_filename(cstr);
    if(tee_ess_eff == TSF_NULL){
        return;
    }

    std::cout << tee_ess_eff << "\n";
    std::cout << "output_setting\n";
    tsf_set_output(tee_ess_eff, TSF_MONO, engineGetSampleRate(), 0.0);

    this->loaded = true;
    this->output_set = false;
    this->output_set = true;

}

void EssEff::step() {

    // if (!loaded && !loading ){

    //     loading = true;
    //     std::cout << "LOADING!\n";
    //     tee_ess_eff = tsf_load_filename("soundfonts/Wii_Grand_Piano.sf2");
    //     std::cout << "LOADED!\n";
    //     loaded = true;
    //     // tsf_close(tee_ess_eff);
    //     //tee_ess_eff = tsfh.load_filename("soundfonts/Wii_Grand_Piano.sf2");
    //     // tee_ess_eff
    //     // file_name = "soundfonts/Wii_Grand_Piano.sf2";
    //     //
    //     // loaded = true;
    //     return;
    // }

    // std::cout << "STEP\n";

    if (!output_set){
        // std::cout << "SETTING\n";
        // output_setting = true;
        // tsf_set_output(tee_ess_eff, TSF_MONO, engineGetSampleRate(), 0.0);
        // output_set = true;
        // return;
    } else {

        // std::cout << "REND\n";
        // Display
        int ps_count = tsf_get_presetcount(tee_ess_eff);
        preset_name = tsf_get_presetname(tee_ess_eff, 0);
        // std::cout << ps_count << "\n";

        // Render
        if (inputs[GATE_INPUT].value >= 1.0f) {

            int note;
            if (inputs[VOCT_INPUT].active){
                note = (int) std::round(inputs[VOCT_INPUT].value * 12.f + 60.f);
                note = clamp(note, 0, 127);
            } else{
                note = 60;
            }
            tsf_note_on(tee_ess_eff, 0, note, 1.0f);
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

    // Inputs and Knobs
    addInput(Port::create<PJ301MPort>(Vec(11, 100), Port::INPUT, module, EssEff::VOCT_INPUT));
    addInput(Port::create<PJ301MPort>(Vec(11, 200), Port::INPUT, module, EssEff::GATE_INPUT));

    addOutput(Port::create<PJ301MPort>(Vec(11, 320), Port::OUTPUT, module, EssEff::MAIN_OUTPUT));
}

struct EssEffItem : MenuItem {
    EssEff *player;
    void onAction(EventAction &e) override {

        std::string dir = player->last_path.empty() ? assetLocal("") : stringDirectory(player->last_path);
        char *path = osdialog_file(OSDIALOG_OPEN, dir.c_str(), NULL, NULL);
        if (path) {
            player->loadFile(path);
            player->last_path = path;
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
    sampleItem->text = "Load file";
    sampleItem->player = player;
    menu->addChild(sampleItem);

    return menu;
}

Model *modelEssEff = Model::create<EssEff, EssEffWidget>("RJModules", "EssEff", "[GEN] EssEff", LFO_TAG);
