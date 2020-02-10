#include "RJModules.hpp"
#include "osdialog.h"
#include "common.hpp"
#include <locale> // for wstring_convert
#if defined ARCH_WIN
#include <codecvt>
#endif
#include <iostream>
#include <cmath>
#include <sstream>
#include <iomanip>
#include <unistd.h>
#include <mutex>

using namespace std;

#define TSF_IMPLEMENTATION
#include "tsf.h"

int num_files = 2;
std::string soundfont_files[2] = {
    "soundfonts/Grand_Piano.sf2",
    "soundfonts/8bit.sf2"
};

/*
Display
*/

struct EssEffSmallStringDisplayWidget : TransparentWidget {

  std::string *value;
  std::shared_ptr<Font> font;

  EssEffSmallStringDisplayWidget() {
    font = Font::load(assetPlugin(pluginInstance, "res/Pokemon.ttf"));
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

/*
Widget
*/

struct EssEff : Module {
    enum ParamIds {
        FILE_PARAM,
        PRESET_PARAM,
        MOD_PARAM,
        BEND_PARAM,
        REC_BUTTON,
        NUM_PARAMS
    };
    enum InputIds {
        VOCT_INPUT,
        GATE_INPUT,
        FILE_INPUT,
        PRESET_INPUT,
        BEND_INPUT,
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
    float last_frame = 0.0;
    int head = -1;
    int current;
    bool crossfade = false;

    std::string file_name = "Hello!";
    std::string preset_name = "Hello!";
    std::string last_path = "";

    SchmittTrigger gateTrigger;
    tsf* tee_ess_eff;

    EssEff() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(EssEff::FILE_PARAM, 0, num_files - 1, 0, "");
        configParam(EssEff::PRESET_PARAM, 0, 320, 0, "");
        configParam(EssEff::BEND_PARAM, 0, 16383, 8192, "");
        configParam(EssEff::REC_BUTTON, 0.0, 1.0, 0.0, "");

    }
    void step() override;
    std::string getAbsolutePath(std::string path);
    void loadFile(std::string path);

};

std::string EssEff::getAbsolutePath(std::string path){
    #if defined ARCH_LIN || defined ARCH_MAC
        char buf[PATH_MAX];
        char *absPathC = realpath(path.c_str(), buf);
        if (absPathC)
            return absPathC;
    #elif defined ARCH_WIN
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
        std::wstring pathW = converter.from_bytes(path);
        wchar_t buf[PATH_MAX];
        wchar_t *absPathC = _wfullpath(buf, pathW.c_str(), PATH_MAX);
        if (absPathC)
            return string::fromWstring(absPathC);
    #endif
    return "";
}

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

    std::string path_str(path);
    path_str = path_str.substr(path_str.length()-12, path_str.length()-1);
    this->file_name = path_str;

}

void EssEff::step() {

    if(!file_chosen && !loading){
        int cur_file;
        if(!inputs[FILE_INPUT].active){
            cur_file = params[FILE_PARAM].value;
        } else{
            cur_file = (int) inputs[FILE_INPUT].value;
            if(cur_file >= num_files){
                cur_file = num_files - 1;
            }
            if(cur_file < 0){
                cur_file = 0;
            }
        }
        if(cur_file != last_file){
            this->loading = true;
            this->loadFile(getAbsolutePath(assetPlugin(pluginInstance, soundfont_files[cur_file])));
            last_file = cur_file;
        }
    }

    if (!output_set || !loaded){
    } else {

        // Display
        int ps_count = tsf_get_presetcount(tee_ess_eff);
        int preset_sel;

        if(!inputs[PRESET_INPUT].active){
            preset_sel = params[PRESET_PARAM].value;
        } else{
            preset_sel = (int) inputs[PRESET_INPUT].value;
        }

        if (preset_sel >= ps_count){
            preset_sel = ps_count - 1;
        }
        if(preset_sel < 0){
            preset_sel = 0;
        }

        std::string preset_name_str(tsf_get_presetname(tee_ess_eff, preset_sel));
        preset_name = preset_name_str.substr(0, 11);

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
                crossfade = true;
            }

            // new note on
            int bend;
            if(inputs[BEND_INPUT].active){
                bend = params[BEND_PARAM].value * clamp(inputs[BEND_INPUT].normalize(10.0f) / 10.0f, 0.0f, 16383.0f);
            } else{
                bend = params[BEND_PARAM].value;
            }

            tsf_channel_set_pitchwheel(tee_ess_eff, preset_sel, bend);
            tsf_note_on(tee_ess_eff, preset_sel, note, 1.0f);

            last_note = note;
            last_preset_sel = preset_sel;

            head = -1;
        }else{
            crossfade = false;
        }

        if ( head < 0 || head >= 1000000 ){
            head = 0;
            tsf_render_float(tee_ess_eff, frame, 1000000, 0);
        } else{
            head++;
        }


        float out_frame = frame[head];
        if(crossfade){
            out_frame = (out_frame + last_frame) / 2.0;
        }
        outputs[MAIN_OUTPUT].value  = out_frame * 3.f;
        last_frame = frame[head];
    }
}

/*
Button
*/

struct RecButton : SvgSwitch {
    RecButton() {
        momentary = true;
        addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/LilLEDButton.svg")));
    }

    void onDragStart(const event::DragStart &e) override {
        EssEff *module = dynamic_cast<EssEff*>(paramQuantity->module);
        if (module && module->last_path == ""){
            std::string dir = "";
            char *path = osdialog_file(OSDIALOG_OPEN, dir.c_str(), NULL, NULL);
            if (path) {
                module->loadFile(path);
                module->last_path = path;
                module->file_chosen = true;
                free(path);
            }
        }

        SvgSwitch::onDragStart(e);
    }
};

struct EssEffWidget: ModuleWidget {
    EssEffWidget(EssEff *module);
    Menu *createContextMenu();
};

EssEffWidget::EssEffWidget(EssEff *module) {
	setModule(module);
    box.size = Vec(15*10, 380);

    {
        SVGPanel *panel = new SVGPanel();
        panel->box.size = box.size;
        panel->setBackground(SVG::load(assetPlugin(pluginInstance, "res/EssEff.svg")));
        addChild(panel);
    }

    addChild(createWidget<ScrewSilver>(Vec(15, 0)));
    addChild(createWidget<ScrewSilver>(Vec(box.size.x-30, 0)));
    addChild(createWidget<ScrewSilver>(Vec(15, 365)));
    addChild(createWidget<ScrewSilver>(Vec(box.size.x-30, 365)));

    // Displays
    if(module != NULL){
        EssEffSmallStringDisplayWidget *fileDisplay = new EssEffSmallStringDisplayWidget();
        fileDisplay->box.pos = Vec(28, 70);
        fileDisplay->box.size = Vec(100, 40);
        fileDisplay->value = &module->file_name;
        addChild(fileDisplay);

        EssEffSmallStringDisplayWidget *presetDisplay = new EssEffSmallStringDisplayWidget();
        presetDisplay->box.pos = Vec(28, 170);
        presetDisplay->box.size = Vec(100, 40);
        presetDisplay->value = &module->preset_name;
        addChild(presetDisplay);
    }

    // Knobs
    addParam(createParam<RoundBlackSnapKnob>(Vec(85, 115), module, EssEff::FILE_PARAM));
    addParam(createParam<RoundBlackSnapKnob>(Vec(85, 215), module, EssEff::PRESET_PARAM));
    addInput(createPort<PJ301MPort>(Vec(37, 117.5), PortWidget::INPUT, module, EssEff::FILE_INPUT));
    addInput(createPort<PJ301MPort>(Vec(37, 217.5), PortWidget::INPUT, module, EssEff::PRESET_INPUT));

    addParam(createParam<RoundBlackKnob>(Vec(85, 262), module, EssEff::BEND_PARAM));
    addInput(createPort<PJ301MPort>(Vec(37, 264.5), PortWidget::INPUT, module, EssEff::BEND_INPUT));

    // Inputs and Knobs
    addInput(createPort<PJ301MPort>(Vec(16, 320), PortWidget::INPUT, module, EssEff::VOCT_INPUT));
    addInput(createPort<PJ301MPort>(Vec(53, 320), PortWidget::INPUT, module, EssEff::GATE_INPUT));
    addOutput(createPort<PJ301MPort>(Vec(112.5, 320), PortWidget::OUTPUT, module, EssEff::MAIN_OUTPUT));

    //Button
    addParam(createParam<RecButton>(Vec(114, 40), module, EssEff::REC_BUTTON));

}

struct EssEffItem : MenuItem {
    EssEff *player;
    void onAction(const event::Action &e) override {

        std::string dir = "";
        char *path = osdialog_file(OSDIALOG_OPEN, dir.c_str(), NULL, NULL);
        if (path) {
            player->loadFile(path);
            player->last_path = path;
            player->file_chosen = true;
            free(path);
        }
    }
};

Menu *EssEffcreateWidgetContextMenu() {

    Menu *menu = EssEffcreateWidgetContextMenu();

    MenuLabel *spacerLabel = new MenuLabel();
    menu->addChild(spacerLabel);

    // EssEff *player = dynamic_cast<EssEff*>(module);
    // assert(player);

    EssEffItem *sampleItem = new EssEffItem();
    sampleItem->text = "Load .sf2 file";
    // sampleItem->player = player;
    menu->addChild(sampleItem);

    return menu;
}



Model *modelEssEff = createModel<EssEff, EssEffWidget>("EssEff");
