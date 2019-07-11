/*
A C I D

aka BADDrumpler

2x Wave + Envelope/VCA + FM Filter + Drumpler

Elements pinched from Lindenberg + dBiZ

*/

#include "RJModules.hpp"

#include <iostream>

#include "Oscillator.hpp"

#include <math.h>
#include "dr_wav.h"
#include <vector>
#include "cmath"
#include <dirent.h>
#include <algorithm> //----added by Joakim Lindbom
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

using deesp::DSPBLOscillator;
#define pi 3.14159265359

using namespace std;


/*
    UI
*/

struct DrumplerRoundLargeBlackKnob : RoundLargeBlackKnob {
    DrumplerRoundLargeBlackKnob() {
        setSVG(SVG::load(assetPlugin(pluginInstance, "res/DrumplerRoundLargeBlackKnob.svg")));
    }
};

struct DrumplerRoundLargeHappyKnob : RoundLargeBlackKnob {
    DrumplerRoundLargeHappyKnob() {
        setSVG(SVG::load(assetPlugin(pluginInstance, "res/DrumplerRoundLargeHappyKnob.svg")));
    }
};

struct DrumplerRoundLargeBlackSnapKnob : DrumplerRoundLargeBlackKnob
{
    DrumplerRoundLargeBlackSnapKnob()
    {
        minAngle = -0.83 * M_PI;
        maxAngle = 0.83 * M_PI;
        snap = true;
    }
};

struct TrigButton : SvgSwitch {
    TrigButton() {
        momentary = true;
        addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/LilLEDButton2.svg")));
    }
};

/*
Display
*/

struct DrumplerSmallStringDisplayWidget : TransparentWidget {

  std::string *value;
  std::shared_ptr<Font> font;

  DrumplerSmallStringDisplayWidget() {
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
    nvgFontSize(vg, 16);
    nvgFontFaceId(vg, font->handle);
    nvgTextLetterSpacing(vg, 0.5);

    std::stringstream to_display;
    to_display << std::setw(3) << *value;

    Vec textPos = Vec(8.0f, 16.0f);
    NVGcolor textColor = nvgRGB(0x00, 0x00, 0x00);
    nvgFillColor(vg, textColor);
    nvgText(vg, textPos.x, textPos.y, to_display.str().c_str(), NULL);
  }
};



/*
    Static Functions and Structions
*/

static float shapeDelta(float delta, float tau, float shape) {
    float lin = sgn(delta) * 10.0 / tau;
    if (shape < 0.0) {
        float log = sgn(delta) * 40.0 / tau / (fabsf(delta) + 1.0);
        return crossfade(lin, log, -shape * 0.95);
    }
    else {
        float exp = M_E * delta / tau;
        return crossfade(lin, exp, shape * 0.90);
    }
}

struct DrumplerMultiFilter
{
    float q;
    float freq;
    float smpRate;
    float hp = 0.0f,bp = 0.0f,lp = 0.0f,mem1 = 0.0f,mem2 = 0.0f;

    void setParams(float freq, float q, float smpRate) {
        this->freq = freq;
        this->q=q;
        this->smpRate=smpRate;
    }

    void calcOutput(float sample)
    {
        float g = tan(pi*freq/smpRate);
        float R = 1.0f/(2.0f*q);
        hp = (sample - (2.0f*R + g)*mem1 - mem2)/(1.0f + 2.0f*R*g + g*g);
        bp = g*hp + mem1;
        lp = g*bp +  mem2;
        mem1 = g*hp + bp;
        mem2 = g*bp + lp;
    }

};

/*
    Modules
*/
struct Drumpler : Module {
    enum ParamIds {

        // Wave
        WAVE_1_PARAM,
        WAVE_2_PARAM,
        WAVE_MIX_PARAM,

        // Env
        ENV_REL_PARAM,
        ENV_SHAPE_PARAM,
        ENV_AMT_PARAM,

        // Filter
        FILTER_CUT_PARAM,
        FILTER_FM_1_PARAM,
        FILTER_FM_2_PARAM,
        FILTER_Q_PARAM,
        FILTER_DRIVE_PARAM,

        // Pluck
        PLUCK_ATTACK_PARAM,
        PLUCK_REL_PARAM,
        PLUCK_EXP_PARAM,

        // Folder
        FOLD_PARAM,

        // Buttons
        TRIG0_BUTTON,

        NUM_PARAMS
    };
    enum InputIds {
        VOCT_INPUT,
        VOCT2_INPUT,
        TRIG_INPUT,

        // Wave
        WAVE_1_INPUT,
        WAVE_2_INPUT,
        WAVE_MIX_INPUT,

        // Env
        ENV_REL_INPUT,
        ENV_SHAPE_INPUT,
        ENV_AMT_INPUT,

        // Filter
        FILTER_CUT_INPUT,
        FILTER_FM_1_INPUT,
        FILTER_FM_2_INPUT,
        FILTER_Q_INPUT,
        FILTER_DRIVE_INPUT,

        // Pluck
        PLUCK_REL_INPUT,
        PLUCK_EXP_INPUT,

        // Trigger
        TRIG0_INPUT,

        EXP_INPUT,
        FOLD_INPUT,
        NUM_INPUTS
    };
    enum OutputIds {
        OUT_OUTPUT,
        ENV_OUTPUT,
        NUM_OUTPUTS
    };
    enum LightIds {
        ATTACK_LIGHT,
        DECAY_LIGHT,
        SUSTAIN_LIGHT,
        RELEASE_LIGHT,
        NUM_LIGHTS
    };

    // Wave
    DSPBLOscillator *osc1 = new DSPBLOscillator(engineGetSampleRate());
    DSPBLOscillator *osc2 = new DSPBLOscillator(engineGetSampleRate());

    // Sample
    unsigned int channels;
    unsigned int sampleRate;
    drwav_uint64 totalSampleCount;
    vector<vector<float>> playBuffer;
    bool loading = false;
    bool play = false;
    std::string lastPath = "";
    float samplePos = 0;
    float startPos = 0;
    std::string fileDesc;
    bool fileLoaded = false;
    vector <std::string> fichier;
    int sampnumber = 0;
    int retard = 0;
    bool reload = false ;
    bool oscState = false ;
    vector<double> displayBuff; // unused

    std::string bank0 = "Kick";
    std::string sample0 = "House";

    // Env
    SchmittTrigger env_trigger;
    float env_out = 0.0f;
    float env_gate = 0.0f;
    float env_in = 0.0f;

    // VCA
    float vca_out;
    float vca_last;

    // Filter
    DrumplerMultiFilter filter;

    Drumpler() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        playBuffer.resize(2);
        playBuffer[0].resize(0);
        playBuffer[1].resize(0);

        configParam(Drumpler::FOLD_PARAM, 1.f, 50.f, 25.f, "");
        configParam(Drumpler::WAVE_1_PARAM, 0.0, 5.0, 0.0, "");
        configParam(Drumpler::WAVE_2_PARAM, 0.0, 4.0, 0.0, "");
        configParam(Drumpler::WAVE_MIX_PARAM, 0.0, 1.0f, 0.0f, "");
        configParam(Drumpler::ENV_REL_PARAM, 0.0, 1.0, 0.0, "");
        configParam(Drumpler::ENV_AMT_PARAM, 0.0, 1.0, 1.0, "");
        configParam(Drumpler::ENV_SHAPE_PARAM, -1.0, 1.0, 0.0, "");
        configParam(Drumpler::FILTER_CUT_PARAM, 0.0, 1.0f, 0.90f, "");
        configParam(Drumpler::FILTER_FM_1_PARAM, -1.0, 1.0f, 0.0f, "");
        configParam(Drumpler::FILTER_Q_PARAM, 0.1f, 1.5f, 0.3f, "");
        configParam(Drumpler::FILTER_FM_2_PARAM, -1.0, 1.0f, 0.0f, "");
        configParam(Drumpler::FILTER_DRIVE_PARAM, -5.0f, 5.0f, 1.0f, "");
        configParam(Drumpler::PLUCK_REL_PARAM, 0.2f, 0.4f, 0.2f, "");
        configParam(Drumpler::PLUCK_EXP_PARAM, 0.0001f, .1f, 4.0f, "");
        configParam(Drumpler::PLUCK_ATTACK_PARAM, 0.0, 1.0f, 0.90f, "");

    }

    void step() override {

    }

    void loadSample(std::string path) {
        //from cF PLAYER.CPP

        loading = true;
        unsigned int c;
        unsigned int sr;
        drwav_uint64 sc;
        float* pSampleData;
        pSampleData = drwav_open_and_read_file_f32(path.c_str(), &c, &sr, &sc);

        if (pSampleData != NULL) {
            channels = c;
            sampleRate = sr;
            playBuffer[0].clear();
            playBuffer[1].clear();

            for (unsigned int i=0; i < sc; i = i + c) {
                playBuffer[0].push_back(pSampleData[i]);
                if (channels == 2)
                    playBuffer[1].push_back((float)pSampleData[i+1]);

            }
            totalSampleCount = playBuffer[0].size();
            drwav_free(pSampleData);
            loading = false;
            fileLoaded = true;
            vector<double>().swap(displayBuff);
            for (int i=0; i < floor(totalSampleCount); i = i + floor(totalSampleCount/130)) {
                displayBuff.push_back(playBuffer[0][i]);
            }
            fileDesc = "\n";
            if (reload) {
                DIR* rep = NULL;
                struct dirent* dirp = NULL;
                std::string dir = path.empty() ? assetLocal("") : "";

                rep = opendir(dir.c_str());
                int i = 0;
                fichier.clear();
                while ((dirp = readdir(rep)) != NULL) {
                    std::string name = dirp->d_name;

                    std::size_t found = name.find(".wav",name.length()-5);
                    if (found==std::string::npos) found = name.find(".WAV",name.length()-5);
                    if (found!=std::string::npos) {
                        fichier.push_back(name);
                        if ((dir + "/" + name)==path) {sampnumber = i;}
                        i=i+1;
                        }

                    }
            sort(fichier.begin(), fichier.end());  // Linux needs this to get files in right order
                for (int o=0;o<int(fichier.size()-1); o++) {
                    if ((dir + "/" + fichier[o])==path) {
                        sampnumber = o;
                    }
                }
                closedir(rep);
                reload = false;
            }
                lastPath = path;
        }
        else {
            fileLoaded = false;
        }
    }
};

struct DrumplerWidget : ModuleWidget {
    DrumplerWidget(Drumpler *module) {
		setModule(module);
        setPanel(SVG::load(assetPlugin(pluginInstance, "res/Drumpler.svg")));

        /* Bullshit */
        addChild(createWidget<ScrewSilver>(Vec(15, 0)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x-30, 0)));
        addChild(createWidget<ScrewSilver>(Vec(15, 365)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x-30, 365)));

        int TOP_BUFFER = 60;
        int BOTTOM_OFFSET = 45;
        int LEFT_BUFFER = 45;
        int RIGHT_BUFFER = 50;

        int CV_SIZE = 2;
        int CV_DIST = 15;
        int CV_NEG_DIST = 10;

        int DISPLAY_SIZE_X = 60;
        int DISPLAY_SIZE_Y = 25;
        int ROW_OFFSET = 100;
        int BANK_OFFSET = 30;
        int BUTTON_SIZE = 5;

        /*
            Row 0
        */

        // Button / CV
        addParam(createParam<TrigButton>(Vec(10, TOP_BUFFER), module, Drumpler::TRIG0_BUTTON));
        addInput(createPort<PJ301MPort>(Vec(10, TOP_BUFFER + BANK_OFFSET), PortWidget::INPUT, module, Drumpler::TRIG0_INPUT));

        // Displays
        if(module != NULL){
            DrumplerSmallStringDisplayWidget *bankDisplay0 = new DrumplerSmallStringDisplayWidget();
            bankDisplay0->box.pos = Vec(LEFT_BUFFER, TOP_BUFFER);
            bankDisplay0->box.size = Vec(DISPLAY_SIZE_X, DISPLAY_SIZE_Y);
            bankDisplay0->value = &module->bank0;
            addChild(bankDisplay0);

            DrumplerSmallStringDisplayWidget *sampleDisplay0 = new DrumplerSmallStringDisplayWidget();
            sampleDisplay0->box.pos = Vec(LEFT_BUFFER, TOP_BUFFER + BANK_OFFSET);
            sampleDisplay0->box.size = Vec(DISPLAY_SIZE_X, DISPLAY_SIZE_Y);
            sampleDisplay0->value = &module->sample0;
            addChild(sampleDisplay0);
        }

        // Wave
        // addParam(createParam<DrumplerRoundLargeBlackSnapKnob>(mm2px(Vec(5 + LEFT_BUFFER, 20 + TOP_BUFFER)), module, Drumpler::WAVE_1_PARAM));
        // addParam(createParam<DrumplerRoundLargeBlackSnapKnob>(mm2px(Vec(30 + LEFT_BUFFER, 20 + TOP_BUFFER)), module, Drumpler::WAVE_2_PARAM));
        // addParam(createParam<DrumplerRoundLargeBlackKnob>(mm2px(Vec(17.5 + LEFT_BUFFER, 35 + TOP_BUFFER)), module, Drumpler::WAVE_MIX_PARAM));

        // addInput(createPort<PJ301MPort>(mm2px(Vec(5 + LEFT_BUFFER + CV_SIZE, 20 + TOP_BUFFER + CV_DIST)), PortWidget::INPUT, module, Drumpler::WAVE_1_INPUT));
        // addInput(createPort<PJ301MPort>(mm2px(Vec(30 + LEFT_BUFFER+ CV_SIZE, 20 + TOP_BUFFER + CV_DIST)), PortWidget::INPUT, module, Drumpler::WAVE_2_INPUT));
        // addInput(createPort<PJ301MPort>(mm2px(Vec(17.5 + LEFT_BUFFER + CV_SIZE, 35 + TOP_BUFFER + CV_DIST)), PortWidget::INPUT, module, Drumpler::WAVE_MIX_INPUT));

        // // Envelope
        // addParam(createParam<DrumplerRoundLargeBlackKnob>(mm2px(Vec(5 + LEFT_BUFFER, 20 + BOTTOM_OFFSET + TOP_BUFFER)), module, Drumpler::ENV_REL_PARAM));
        // addParam(createParam<DrumplerRoundLargeBlackKnob>(mm2px(Vec(30 + LEFT_BUFFER, 20 + BOTTOM_OFFSET + TOP_BUFFER)), module, Drumpler::ENV_AMT_PARAM));
        // addParam(createParam<DrumplerRoundLargeBlackKnob>(mm2px(Vec(17.5 + LEFT_BUFFER, 35 + BOTTOM_OFFSET + TOP_BUFFER)), module, Drumpler::ENV_SHAPE_PARAM));

        // addInput(createPort<PJ301MPort>(mm2px(Vec(5 + LEFT_BUFFER + CV_SIZE, 20 + TOP_BUFFER + BOTTOM_OFFSET + CV_DIST)), PortWidget::INPUT, module, Drumpler::ENV_REL_INPUT));
        // addInput(createPort<PJ301MPort>(mm2px(Vec(30 + LEFT_BUFFER+ CV_SIZE, 20 + TOP_BUFFER + BOTTOM_OFFSET + CV_DIST)), PortWidget::INPUT, module, Drumpler::ENV_AMT_INPUT));
        // addInput(createPort<PJ301MPort>(mm2px(Vec(17.5 + LEFT_BUFFER + CV_SIZE, 35 + TOP_BUFFER + BOTTOM_OFFSET + CV_DIST)), PortWidget::INPUT, module, Drumpler::ENV_SHAPE_INPUT));

        // /*
        //     Right Side
        // */

        // // Filter
        // addParam(createParam<DrumplerRoundLargeBlackKnob>(mm2px(Vec(17.5 + LEFT_BUFFER + RIGHT_BUFFER, 20 + TOP_BUFFER)), module, Drumpler::FILTER_CUT_PARAM));
        // addParam(createParam<DrumplerRoundLargeBlackKnob>(mm2px(Vec(5 + LEFT_BUFFER + RIGHT_BUFFER, 35 + TOP_BUFFER)), module, Drumpler::FILTER_FM_1_PARAM));
        // addParam(createParam<DrumplerRoundLargeBlackKnob>(mm2px(Vec(30 + LEFT_BUFFER + RIGHT_BUFFER, 35 + TOP_BUFFER)), module, Drumpler::FILTER_Q_PARAM));
        // addParam(createParam<DrumplerRoundLargeBlackKnob>(mm2px(Vec(5 + LEFT_BUFFER + RIGHT_BUFFER, 50 + TOP_BUFFER)), module, Drumpler::FILTER_FM_2_PARAM));
        // addParam(createParam<DrumplerRoundLargeBlackKnob>(mm2px(Vec(30 + LEFT_BUFFER + RIGHT_BUFFER, 50 + TOP_BUFFER)), module, Drumpler::FILTER_DRIVE_PARAM));

        // addInput(createPort<PJ301MPort>(mm2px(Vec(17.5 + LEFT_BUFFER + RIGHT_BUFFER + CV_SIZE, 20 + TOP_BUFFER + CV_DIST)), PortWidget::INPUT, module, Drumpler::FILTER_CUT_INPUT));
        // addInput(createPort<PJ301MPort>(mm2px(Vec(5 + LEFT_BUFFER + RIGHT_BUFFER + CV_SIZE, 35 + TOP_BUFFER - CV_NEG_DIST)), PortWidget::INPUT, module, Drumpler::FILTER_FM_1_INPUT));
        // addInput(createPort<PJ301MPort>(mm2px(Vec(30 + LEFT_BUFFER + RIGHT_BUFFER + CV_SIZE, 35 + TOP_BUFFER - CV_NEG_DIST)), PortWidget::INPUT, module, Drumpler::FILTER_Q_INPUT));
        // addInput(createPort<PJ301MPort>(mm2px(Vec(5 + LEFT_BUFFER + RIGHT_BUFFER + CV_SIZE, 50 + TOP_BUFFER + CV_DIST)), PortWidget::INPUT, module, Drumpler::FILTER_FM_2_INPUT));
        // addInput(createPort<PJ301MPort>(mm2px(Vec(30 + LEFT_BUFFER + RIGHT_BUFFER + CV_SIZE, 50 + TOP_BUFFER + CV_DIST)), PortWidget::INPUT, module, Drumpler::FILTER_DRIVE_INPUT));


        // // Pluck
        // addParam(createParam<DrumplerRoundLargeBlackKnob>(mm2px(Vec(5 + LEFT_BUFFER + RIGHT_BUFFER, 35 + BOTTOM_OFFSET + TOP_BUFFER)), module, Drumpler::PLUCK_REL_PARAM));
        // addParam(createParam<DrumplerRoundLargeBlackKnob>(mm2px(Vec(30 + LEFT_BUFFER + RIGHT_BUFFER, 35 + BOTTOM_OFFSET + TOP_BUFFER)), module, Drumpler::PLUCK_EXP_PARAM));
        // // addParam(createParam<DrumplerRoundLargeBlackKnob>(mm2px(Vec(17.5 + LEFT_BUFFER + RIGHT_BUFFER, 20 + BOTTOM_OFFSET + TOP_BUFFER)), module, Drumpler::PLUCK_ATTACK_PARAM));

        // addInput(createPort<PJ301MPort>(mm2px(Vec(5 + LEFT_BUFFER + RIGHT_BUFFER + CV_SIZE, 35 + TOP_BUFFER + BOTTOM_OFFSET + CV_DIST)), PortWidget::INPUT, module, Drumpler::PLUCK_REL_INPUT));
        // addInput(createPort<PJ301MPort>(mm2px(Vec(30 + LEFT_BUFFER + RIGHT_BUFFER + CV_SIZE, 35 + TOP_BUFFER + BOTTOM_OFFSET + CV_DIST)), PortWidget::INPUT, module, Drumpler::PLUCK_EXP_INPUT));

        /*
            Bottom
        */
        float OUTLINE = 117.5f;
        addInput(createPort<PJ301MPort>(mm2px(Vec(5 + LEFT_BUFFER + CV_SIZE, OUTLINE)), PortWidget::INPUT, module, Drumpler::VOCT_INPUT));
        addInput(createPort<PJ301MPort>(mm2px(Vec(17.5 + LEFT_BUFFER + CV_SIZE, OUTLINE)), PortWidget::INPUT, module, Drumpler::VOCT2_INPUT));
        addInput(createPort<PJ301MPort>(mm2px(Vec(30 + LEFT_BUFFER + CV_SIZE, OUTLINE)), PortWidget::INPUT, module, Drumpler::TRIG_INPUT));

        addOutput(createPort<PJ301MPort>(mm2px(Vec(17.5 + LEFT_BUFFER + RIGHT_BUFFER + CV_SIZE, OUTLINE)), PortWidget::OUTPUT, module, Drumpler::ENV_OUTPUT));
        addOutput(createPort<PJ301MPort>(mm2px(Vec(30 + LEFT_BUFFER + RIGHT_BUFFER + CV_SIZE, OUTLINE)), PortWidget::OUTPUT, module, Drumpler::OUT_OUTPUT));
    }
};


Model *modelDrumpler = createModel<Drumpler, DrumplerWidget>("Drumpler");