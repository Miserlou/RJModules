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

    // Pluck
    float lastCv = 0.f;
    bool decaying = false;
    float env = 0.0f;
    SchmittTrigger trigger;

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

        /*
        Load up our 303 samples
        */
        if(!fileLoaded){
            loadSample(assetPlugin(pluginInstance, "samples/303_wavetable_c.wav"));
            return;
        }

        /*
            Inputs
        */
        float voct = inputs[VOCT_INPUT].value;
        float voct2 = inputs[VOCT_INPUT].value;
        if(inputs[VOCT2_INPUT].active){
            voct2 = inputs[VOCT2_INPUT].value;
        }

        /*
            Wave
        */

        // OSC1
        float wave1 = params[WAVE_1_PARAM].value * clamp(inputs[WAVE_1_INPUT].normalize(10.0f) / 10.0f, 0.0f, 1.0f);
        osc1->setInputs(voct, 0.0, 0.0, 0.f, -2.f);
        osc1->process();
        float osc1_out;
        switch(int(wave1)){
            // Sample
            case 0:
                osc1_out = 5 * playBuffer[0][floor(samplePos)];
                break;
            // Sin
            case 1:
                osc1_out = osc1->getSineWave();
                break;
            // Saw
            case 2:
                osc1_out = osc1->getSawWave();
                break;
            // Pulse
            case 3:
                osc1_out = osc1->getPulseWave();
                break;
            // Tri
            case 4:
                osc1_out = osc1->getTriWave();
                break;
            // Noise
            case 5:
                osc1_out = osc1->getNoise();
                break;
        }

        // Sampler moves regardless
        samplePos = samplePos + powf(2.0, voct); // 3 octaves higher because i made the sample wrong
        if (floor(samplePos) >= totalSampleCount){
            samplePos = 0;
        }

        // OSC2
        float wave2 = params[WAVE_2_PARAM].value * clamp(inputs[WAVE_2_INPUT].normalize(10.0f) / 10.0f, 0.0f, 1.0f);;
        osc2->setInputs(voct2, 0.0, 0.0, 0.f, -2.f);
        osc2->process();
        float osc2_out;
        switch(int(wave2)){

            // Sin
            case 0:
                osc2_out = osc2->getSineWave();
                break;
            // Saw
            case 1:
                osc2_out = osc2->getSawWave();
                break;
            // Pulse
            case 2:
                osc2_out = osc2->getPulseWave();
                break;
            // Tri
            case 3:
                osc2_out = osc2->getTriWave();
                break;
            // Noise
            case 4:
                osc2_out = osc2->getNoise();
                break;
        }

        // Mix
        float wave_mixed = ((1 - params[WAVE_MIX_PARAM].value * clamp(inputs[WAVE_MIX_INPUT].normalize(10.0f) / 10.0f, 0.0f, 1.0f)) * osc1_out) + ((params[WAVE_MIX_PARAM].value * clamp(inputs[WAVE_MIX_INPUT].normalize(10.0f) / 10.0f, 0.0f, 1.0f)) * osc2_out);

        /*
            Envelope
        */
        float shape = params[ENV_SHAPE_PARAM].value * clamp(inputs[ENV_SHAPE_INPUT].normalize(10.0f) / 10.0f, 0.0f, 1.0f);
        float minTime = 1e-2;
        env_in = 0.0f;

        // Trigger
        if (env_trigger.process(inputs[TRIG_INPUT].value)) {
            env_gate = true;
        }
        if (env_gate) {
            env_in = 10.0;
        } else{
        }

        bool rising = false;
        bool falling = false;
        float delta = env_in - env_out;
        if (delta > 0) {
            float riseCv = 0.0;
            float rise = minTime * powf(2.0, riseCv * 10.0);
            env_out += shapeDelta(delta, rise, shape) / engineGetSampleRate();
            rising = (env_in - env_out > 1e-3);
            if (!rising) {
                env_gate = false;
            }
        }
        else if (delta < 0) {
            float fallCv = params[ENV_REL_PARAM].value * clamp(inputs[ENV_REL_INPUT].normalize(10.0f) / 10.0f, 0.0f, 1.0f) + 0.0 / 10.0;
            fallCv = clamp(fallCv, 0.0, 1.0);
            float fall = minTime * powf(2.0, fallCv * 10.0);
            env_out += shapeDelta(delta, fall, shape) / engineGetSampleRate();
            falling = (env_in - env_out < -1e-3);
        }
        else {
            env_gate = false;
        }
        if (!rising && !falling) {
            env_out = env_in;
        }

        /*
            VCA
            via https://github.com/VCVRack/AudibleInstruments/blob/dd25b1785c2e67f19824fad97527c97c5d779685/src/Veils.cpp
        */
        float vca_out = env_out * params[ENV_AMT_PARAM].value * clamp(inputs[ENV_AMT_INPUT].normalize(10.0f) / 10.0f, 0.0f, 1.0f);
        outputs[ENV_OUTPUT].value = vca_out;

        /*
            Filter
        */

        // Stage 1
        //float cutoff = pow(2.0f, rescale(clamp(params[FILTER_CUT_PARAM].value + quadraticBipolar(params[FILTER_FM_2_PARAM].value) * 0.1f * inputs[CUTOFF_INPUT2].value + quadraticBipolar(params[FILTER_FM_PARAM].value) * 0.1f * inputs[CUTOFF_INPUT].value / 5.0f, 0.0f , 1.0f), 0.0f, 1.0f, 4.5f, 13.0f));
        float cutoff = pow(2.0f, rescale(clamp(params[FILTER_CUT_PARAM].value * clamp(inputs[FILTER_CUT_INPUT].normalize(10.0f) / 10.0f, 0.0f, 1.0f) + quadraticBipolar(params[FILTER_FM_2_PARAM].value * clamp(inputs[FILTER_FM_2_INPUT].normalize(10.0f) / 10.0f, 0.0f, 1.0f)  ) * 0.1f * vca_out + quadraticBipolar(params[FILTER_FM_1_PARAM].value * clamp(inputs[FILTER_FM_1_INPUT].normalize(10.0f) / 10.0f, 0.0f, 1.0f) ) * 0.1f * vca_out / 5.0f, 0.0f , 1.0f), 0.0f, 1.0f, 4.5f, 13.0f)) ;
        //float q = 10.0f * clamp(params[FILTER_Q_PARAM].value + inputs[Q_INPUT].value / 5.0f, 0.1f, 1.0f);

        // I don't love this, but okay..
        //cutoff = cutoff + (2000 * clamp(inputs[FILTER_CUT_INPUT].normalize(10.0f) / 10.0f, 0.0f, 1.0f));

        // TODO: Find best values for these
        float q = 40.0f * clamp(params[FILTER_Q_PARAM].value * clamp(inputs[FILTER_Q_INPUT].normalize(10.0f) / 10.0f, 0.0f, 1.0f) / 5.0f, 0.1f, 1.0f);
        filter.setParams(cutoff, q, engineGetSampleRate());
        float in = wave_mixed / 5.0f;

        // Stage 2
        in = clamp(in, -5.0f, 5.0f) * 0.2f;
        float a_shape = params[FILTER_DRIVE_PARAM].value * clamp(inputs[FILTER_DRIVE_INPUT].normalize(10.0f) / 10.0f, 0.0f, 1.0f);
        a_shape = clamp(a_shape, -5.0f, 5.0f) * 0.2f;
        a_shape *= 0.99f;
        const float a_shapeB = (1.0 - a_shape) / (1.0 + a_shape);
        const float a_shapeA = (4.0 * a_shape) / ((1.0 - a_shape) * (1.0 + a_shape));
        float a_outputd = in * (a_shapeA + a_shapeB);
        a_outputd = a_outputd / ((std::abs(in) * a_shapeA) + a_shapeB);
        filter.calcOutput(a_outputd);
        float filter_out = filter.lp * 3.0f;

        /*
            Fold!
            from Lindenberg
        */
        float x = clamp(filter_out * 0.1f, -1.f, 1.f);
        float cv = inputs[FOLD_INPUT].value;
        float a = clamp(params[FOLD_PARAM].value + cv, 1.f, 50.f);
        // do the Drumpler!
        float fold_out = x * (fabs(x) + a) / (x * x + (a - 1) * fabs(x) + 1);
        fold_out = fold_out * 5.0f;

        /*
            Pluck
        */
        float pluck_out = fold_out;

        if(inputs[TRIG_INPUT].active){

            /* ADSR */
            float attack = 0.005f;
            float decay = 10.0f;
            float sustain = 10.0f;
            float release = clamp(params[PLUCK_REL_PARAM].value + inputs[PLUCK_REL_INPUT].value / 10.0f, 0.0f, 1.0f);

            // Gate and trigger
            bool gated = inputs[TRIG_INPUT].value >= 1.0f;
            if (trigger.process(inputs[TRIG_INPUT].value)){
                decaying = false;
                gated =true;
            } else{
                gated=false;
            }

            const float base = 20000.0f;
            const float maxTime = 20.0f;
            if (gated) {
                if (decaying) {
                    // Decay
                    if (decay < 1e-4) {
                        env = sustain;
                    }
                    else {
                        env += powf(base, 1 - decay) / maxTime * (sustain - env) * engineGetSampleTime();
                    }
                }
                else {
                    // Attack
                    // Skip ahead if attack is all the way down (infinitely fast)
                    if (attack < 1e-4) {
                        env = 1.0f;
                    }
                    else {
                        env += powf(base, 1 - attack) / maxTime * (1.01f - env) * engineGetSampleTime();
                    }
                    if (env >= 1.0f) {
                        env = 1.0f;
                        decaying = true;
                    }
                }
            }
            else {
                // Release
                if (release < 1e-4) {
                    env = 0.0f;
                }
                else {
                    env += powf(base, 1 - release) / maxTime * (0.0f - env) * engineGetSampleTime();
                }
                decaying = false;
            }

            bool sustaining = isNear(env, sustain, 1e-3);
            bool resting = isNear(env, 0.0f, 1e-3);
            float env_output = 10.0f * env;

            /* VCA */
            float vca_cv = fmaxf(env_output / 10.f, 0.f);
            float exp_val =  clamp(params[PLUCK_EXP_PARAM].value + inputs[PLUCK_EXP_INPUT].value / 10.0f, 0.0f, 1.0f);
            vca_cv = powf(vca_cv, exp_val);
            lastCv = vca_cv;
            pluck_out = fold_out * vca_cv;

        }

        /*
            Outputs
        */
        outputs[OUT_OUTPUT].value = pluck_out;

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

        int TOP_BUFFER = 6;
        int BOTTOM_OFFSET = 45;
        int LEFT_BUFFER = 2;
        int RIGHT_BUFFER = 50;

        int CV_SIZE = 2;
        int CV_DIST = 15;
        int CV_NEG_DIST = 10;

        /*
            Secret
        */
        addParam(createParam<DrumplerRoundLargeHappyKnob>(mm2px(Vec(18 + LEFT_BUFFER + RIGHT_BUFFER, 2)), module, Drumpler::FOLD_PARAM));

        /*
            Left Side
        */

        // Wave
        addParam(createParam<DrumplerRoundLargeBlackSnapKnob>(mm2px(Vec(5 + LEFT_BUFFER, 20 + TOP_BUFFER)), module, Drumpler::WAVE_1_PARAM));
        addParam(createParam<DrumplerRoundLargeBlackSnapKnob>(mm2px(Vec(30 + LEFT_BUFFER, 20 + TOP_BUFFER)), module, Drumpler::WAVE_2_PARAM));
        addParam(createParam<DrumplerRoundLargeBlackKnob>(mm2px(Vec(17.5 + LEFT_BUFFER, 35 + TOP_BUFFER)), module, Drumpler::WAVE_MIX_PARAM));

        addInput(createPort<PJ301MPort>(mm2px(Vec(5 + LEFT_BUFFER + CV_SIZE, 20 + TOP_BUFFER + CV_DIST)), PortWidget::INPUT, module, Drumpler::WAVE_1_INPUT));
        addInput(createPort<PJ301MPort>(mm2px(Vec(30 + LEFT_BUFFER+ CV_SIZE, 20 + TOP_BUFFER + CV_DIST)), PortWidget::INPUT, module, Drumpler::WAVE_2_INPUT));
        addInput(createPort<PJ301MPort>(mm2px(Vec(17.5 + LEFT_BUFFER + CV_SIZE, 35 + TOP_BUFFER + CV_DIST)), PortWidget::INPUT, module, Drumpler::WAVE_MIX_INPUT));

        // Envelope
        addParam(createParam<DrumplerRoundLargeBlackKnob>(mm2px(Vec(5 + LEFT_BUFFER, 20 + BOTTOM_OFFSET + TOP_BUFFER)), module, Drumpler::ENV_REL_PARAM));
        addParam(createParam<DrumplerRoundLargeBlackKnob>(mm2px(Vec(30 + LEFT_BUFFER, 20 + BOTTOM_OFFSET + TOP_BUFFER)), module, Drumpler::ENV_AMT_PARAM));
        addParam(createParam<DrumplerRoundLargeBlackKnob>(mm2px(Vec(17.5 + LEFT_BUFFER, 35 + BOTTOM_OFFSET + TOP_BUFFER)), module, Drumpler::ENV_SHAPE_PARAM));

        addInput(createPort<PJ301MPort>(mm2px(Vec(5 + LEFT_BUFFER + CV_SIZE, 20 + TOP_BUFFER + BOTTOM_OFFSET + CV_DIST)), PortWidget::INPUT, module, Drumpler::ENV_REL_INPUT));
        addInput(createPort<PJ301MPort>(mm2px(Vec(30 + LEFT_BUFFER+ CV_SIZE, 20 + TOP_BUFFER + BOTTOM_OFFSET + CV_DIST)), PortWidget::INPUT, module, Drumpler::ENV_AMT_INPUT));
        addInput(createPort<PJ301MPort>(mm2px(Vec(17.5 + LEFT_BUFFER + CV_SIZE, 35 + TOP_BUFFER + BOTTOM_OFFSET + CV_DIST)), PortWidget::INPUT, module, Drumpler::ENV_SHAPE_INPUT));

        /*
            Right Side
        */

        // Filter
        addParam(createParam<DrumplerRoundLargeBlackKnob>(mm2px(Vec(17.5 + LEFT_BUFFER + RIGHT_BUFFER, 20 + TOP_BUFFER)), module, Drumpler::FILTER_CUT_PARAM));
        addParam(createParam<DrumplerRoundLargeBlackKnob>(mm2px(Vec(5 + LEFT_BUFFER + RIGHT_BUFFER, 35 + TOP_BUFFER)), module, Drumpler::FILTER_FM_1_PARAM));
        addParam(createParam<DrumplerRoundLargeBlackKnob>(mm2px(Vec(30 + LEFT_BUFFER + RIGHT_BUFFER, 35 + TOP_BUFFER)), module, Drumpler::FILTER_Q_PARAM));
        addParam(createParam<DrumplerRoundLargeBlackKnob>(mm2px(Vec(5 + LEFT_BUFFER + RIGHT_BUFFER, 50 + TOP_BUFFER)), module, Drumpler::FILTER_FM_2_PARAM));
        addParam(createParam<DrumplerRoundLargeBlackKnob>(mm2px(Vec(30 + LEFT_BUFFER + RIGHT_BUFFER, 50 + TOP_BUFFER)), module, Drumpler::FILTER_DRIVE_PARAM));

        addInput(createPort<PJ301MPort>(mm2px(Vec(17.5 + LEFT_BUFFER + RIGHT_BUFFER + CV_SIZE, 20 + TOP_BUFFER + CV_DIST)), PortWidget::INPUT, module, Drumpler::FILTER_CUT_INPUT));
        addInput(createPort<PJ301MPort>(mm2px(Vec(5 + LEFT_BUFFER + RIGHT_BUFFER + CV_SIZE, 35 + TOP_BUFFER - CV_NEG_DIST)), PortWidget::INPUT, module, Drumpler::FILTER_FM_1_INPUT));
        addInput(createPort<PJ301MPort>(mm2px(Vec(30 + LEFT_BUFFER + RIGHT_BUFFER + CV_SIZE, 35 + TOP_BUFFER - CV_NEG_DIST)), PortWidget::INPUT, module, Drumpler::FILTER_Q_INPUT));
        addInput(createPort<PJ301MPort>(mm2px(Vec(5 + LEFT_BUFFER + RIGHT_BUFFER + CV_SIZE, 50 + TOP_BUFFER + CV_DIST)), PortWidget::INPUT, module, Drumpler::FILTER_FM_2_INPUT));
        addInput(createPort<PJ301MPort>(mm2px(Vec(30 + LEFT_BUFFER + RIGHT_BUFFER + CV_SIZE, 50 + TOP_BUFFER + CV_DIST)), PortWidget::INPUT, module, Drumpler::FILTER_DRIVE_INPUT));


        // Pluck
        addParam(createParam<DrumplerRoundLargeBlackKnob>(mm2px(Vec(5 + LEFT_BUFFER + RIGHT_BUFFER, 35 + BOTTOM_OFFSET + TOP_BUFFER)), module, Drumpler::PLUCK_REL_PARAM));
        addParam(createParam<DrumplerRoundLargeBlackKnob>(mm2px(Vec(30 + LEFT_BUFFER + RIGHT_BUFFER, 35 + BOTTOM_OFFSET + TOP_BUFFER)), module, Drumpler::PLUCK_EXP_PARAM));
        // addParam(createParam<DrumplerRoundLargeBlackKnob>(mm2px(Vec(17.5 + LEFT_BUFFER + RIGHT_BUFFER, 20 + BOTTOM_OFFSET + TOP_BUFFER)), module, Drumpler::PLUCK_ATTACK_PARAM));

        addInput(createPort<PJ301MPort>(mm2px(Vec(5 + LEFT_BUFFER + RIGHT_BUFFER + CV_SIZE, 35 + TOP_BUFFER + BOTTOM_OFFSET + CV_DIST)), PortWidget::INPUT, module, Drumpler::PLUCK_REL_INPUT));
        addInput(createPort<PJ301MPort>(mm2px(Vec(30 + LEFT_BUFFER + RIGHT_BUFFER + CV_SIZE, 35 + TOP_BUFFER + BOTTOM_OFFSET + CV_DIST)), PortWidget::INPUT, module, Drumpler::PLUCK_EXP_INPUT));

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