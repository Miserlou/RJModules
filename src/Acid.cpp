/*
A C I D

aka BADACID

2x Wave + Envelope/VCA + FM Filter + Acid

Elements pinched from Lindenberg + dBiZ

*/

#include "RJModules.hpp"

#include <iostream>

#include "Oscillator.hpp"
#include "dsp/digital.hpp"

#include <math.h>
#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"
#include <vector>
#include "cmath"
#include <dirent.h>
#include <algorithm> //----added by Joakim Lindbom

using dsp::DSPBLOscillator;
#define pi 3.14159265359

using namespace std;


/*
    UI
*/

struct AcidRoundLargeBlackKnob : RoundLargeBlackKnob {
    AcidRoundLargeBlackKnob() {
        setSVG(SVG::load(assetPlugin(plugin, "res/AcidRoundLargeBlackKnob.svg")));
    }
};

struct AcidRoundLargeHappyKnob : RoundLargeBlackKnob {
    AcidRoundLargeHappyKnob() {
        setSVG(SVG::load(assetPlugin(plugin, "res/AcidRoundLargeHappyKnob.svg")));
    }
};

struct AcidRoundLargeBlackSnapKnob : AcidRoundLargeBlackKnob
{
    AcidRoundLargeBlackSnapKnob()
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

struct MultiFilter
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
struct Acid : Module {
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
    string lastPath = "";
    float samplePos = 0;
    float startPos = 0;
    string fileDesc;
    bool fileLoaded = false;
    vector <string> fichier;
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
    MultiFilter filter;

    // Pluck
    float lastCv = 0.f;
    bool decaying = false;
    float env = 0.0f;
    SchmittTrigger trigger;

    Acid() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {
        playBuffer.resize(2);
        playBuffer[0].resize(0);
        playBuffer[1].resize(0);
    }

    void step() override {

        if(!fileLoaded){
            loadSample("/Users/rjones/Sources/Rack/plugins/RJModules/samples/303_wavetable_c.wav");
            std::cout << "loaded!" << fileLoaded <<" \n";
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
        float wave1 = params[WAVE_1_PARAM].value;
        osc1->setInputs(voct, 0.0, 0.0, 0.f, -2.f);
        osc1->process();
        float osc1_out;
        switch(int(wave1)){

            // Sin
            case 0:
                osc1_out = osc1->getSineWave();
                break;
            // Saw
            case 1:
                osc1_out = osc1->getSawWave();
                break;
            // Pulse
            case 2:
                osc1_out = osc1->getPulseWave();
                break;
            // Tri
            case 3:
                osc1_out = osc1->getTriWave();
                break;
            // Noise
            case 4:
                osc1_out = osc1->getNoise();
                break;
            // Sample
            case 5:
                osc1_out = 5 * playBuffer[0][floor(samplePos)];
                break;
        }

        // Sampler moves regardless
        samplePos = samplePos + powf(2.0, voct); // 3 octaves higher because i made the sample wrong
        if (floor(samplePos) >= totalSampleCount){
            samplePos = 0;
        }

        // OSC2
        float wave2 = params[WAVE_2_PARAM].value;
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
        float wave_mixed = ((1 - params[WAVE_MIX_PARAM].value) * osc1_out) + ((params[WAVE_MIX_PARAM].value) * osc2_out);

        /*
            Envelope
        */
        float shape = params[ENV_SHAPE_PARAM].value;
        float minTime = 1e-2;
        env_in = 0.0f;

        // Trigger
        // if (env_trigger.process(inputs[TRIG_INPUT].value)) {
        if (inputs[TRIG_INPUT].value >= 1.0f) {
            env_gate = true;
            // std::cout << "TRIGGING\n";
        // } else {
        //     env_gate = false;
        //     // std::cout << "NO TRIGGING \n";
        // }
        }
        if (env_gate) {
            env_in = 10.0;
        } else{
            // env_in = 0.0;
        }

        bool rising = false;
        bool falling = false;
        float delta = env_in - env_out;
        if (delta > 0) {
            // std::cout << "RISING!\n" << delta;
            // Rise
            // float riseCv = params[RISE_PARAM].value + inputs[RISE_INPUT].value / 10.0;
            // float riseCv = params[RISE_PARAM].value;
            float riseCv = 0.0;
            //riseCv = clamp(riseCv, 0.0, 1.0);
            // riseCv = 1.0;
            float rise = minTime * powf(2.0, riseCv * 10.0);
            env_out += shapeDelta(delta, rise, shape) / engineGetSampleRate();
            rising = (env_in - env_out > 1e-3);
            if (!rising) {
                env_gate = false;
            }
        }
        else if (delta < 0) {
            //std::cout << "Falling!\n";
            // Fall
            //float fallCv = params[FALL_PARAM].value + inputs[FALL_INPUT].value / 10.0;
            float fallCv = params[ENV_REL_PARAM].value + 0.0 / 10.0;
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

        outputs[ENV_OUTPUT].value = env_out;

        /*
            VCA
            via https://github.com/VCVRack/AudibleInstruments/blob/dd25b1785c2e67f19824fad97527c97c5d779685/src/Veils.cpp
        */
        float vca_out = env_out * params[ENV_AMT_PARAM].value;

        /*
            Filter
        */

        // Stage 1
        //float cutoff = pow(2.0f, rescale(clamp(params[FILTER_CUT_PARAM].value + quadraticBipolar(params[FILTER_FM_2_PARAM].value) * 0.1f * inputs[CUTOFF_INPUT2].value + quadraticBipolar(params[FILTER_FM_PARAM].value) * 0.1f * inputs[CUTOFF_INPUT].value / 5.0f, 0.0f , 1.0f), 0.0f, 1.0f, 4.5f, 13.0f));
        float cutoff = pow(2.0f, rescale(clamp(params[FILTER_CUT_PARAM].value + quadraticBipolar(params[FILTER_FM_2_PARAM].value) * 0.1f * vca_out + quadraticBipolar(params[FILTER_FM_1_PARAM].value) * 0.1f * vca_out / 5.0f, 0.0f , 1.0f), 0.0f, 1.0f, 4.5f, 13.0f));
        //float q = 10.0f * clamp(params[FILTER_Q_PARAM].value + inputs[Q_INPUT].value / 5.0f, 0.1f, 1.0f);

        // TODO: Find best values for these
        float q = 40.0f * clamp(params[FILTER_Q_PARAM].value / 5.0f, 0.1f, 1.0f);
        filter.setParams(cutoff, q, engineGetSampleRate());
        float in = wave_mixed / 5.0f;

        // Stage 2
        in = clamp(in, -5.0f, 5.0f) * 0.2f;
        float a_shape = params[FILTER_DRIVE_PARAM].value;
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
        // do the acid!
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
            if (trigger.process(inputs[TRIG_INPUT].value))
                decaying = false;

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
            std::cout << "hey\n";

            channels = c;
            sampleRate = sr;

            std::cout << "1\n";
            playBuffer[0].clear();
            playBuffer[1].clear();

            std::cout << "2\n";
            for (unsigned int i=0; i < sc; i = i + c) {
                playBuffer[0].push_back(pSampleData[i]);
                if (channels == 2)
                    playBuffer[1].push_back((float)pSampleData[i+1]);

            }

            std::cout << "3\n";
            totalSampleCount = playBuffer[0].size();

            std::cout << "4\n";
            drwav_free(pSampleData);
            loading = false;
            fileLoaded = true;
            std::cout << "loaded\n";
            vector<double>().swap(displayBuff);
            for (int i=0; i < floor(totalSampleCount); i = i + floor(totalSampleCount/130)) {
                displayBuff.push_back(playBuffer[0][i]);
            }
            fileDesc = stringFilename(path)+ "\n";
            fileDesc += std::to_string(sampleRate)+ " Hz" + "\n";
            fileDesc += std::to_string(channels)+ " channel(s)" + "\n";
            if (reload) {
                DIR* rep = NULL;
                struct dirent* dirp = NULL;
                std::string dir = path.empty() ? assetLocal("") : stringDirectory(path);

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
            std::cout << "notloaded\n";
            fileLoaded = false;
        }

        std::cout << "fileLoaded\n";
    }
};

struct AcidWidget : ModuleWidget {
    AcidWidget(Acid *module) : ModuleWidget(module) {
        setPanel(SVG::load(assetPlugin(plugin, "res/Acid.svg")));

        /* Bullshit */
        addChild(Widget::create<ScrewSilver>(Vec(15, 0)));
        addChild(Widget::create<ScrewSilver>(Vec(box.size.x-30, 0)));
        addChild(Widget::create<ScrewSilver>(Vec(15, 365)));
        addChild(Widget::create<ScrewSilver>(Vec(box.size.x-30, 365)));

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
        addParam(ParamWidget::create<AcidRoundLargeHappyKnob>(mm2px(Vec(18 + LEFT_BUFFER + RIGHT_BUFFER, 2)), module, Acid::FOLD_PARAM, 1.f, 50.f, 25.f));

        /*
            Left Side
        */

        // Wave
        addParam(ParamWidget::create<AcidRoundLargeBlackSnapKnob>(mm2px(Vec(5 + LEFT_BUFFER, 20 + TOP_BUFFER)), module, Acid::WAVE_1_PARAM, 0.0, 5.0, 0.0));
        addParam(ParamWidget::create<AcidRoundLargeBlackSnapKnob>(mm2px(Vec(30 + LEFT_BUFFER, 20 + TOP_BUFFER)), module, Acid::WAVE_2_PARAM, 0.0, 4.0, 0.0));
        addParam(ParamWidget::create<AcidRoundLargeBlackKnob>(mm2px(Vec(17.5 + LEFT_BUFFER, 35 + TOP_BUFFER)), module, Acid::WAVE_MIX_PARAM, 0.0, 1.0f, 0.0f));

        addInput(Port::create<PJ301MPort>(mm2px(Vec(5 + LEFT_BUFFER + CV_SIZE, 20 + TOP_BUFFER + CV_DIST)), Port::INPUT, module, Acid::WAVE_1_INPUT));
        addInput(Port::create<PJ301MPort>(mm2px(Vec(30 + LEFT_BUFFER+ CV_SIZE, 20 + TOP_BUFFER + CV_DIST)), Port::INPUT, module, Acid::WAVE_2_INPUT));
        addInput(Port::create<PJ301MPort>(mm2px(Vec(17.5 + LEFT_BUFFER + CV_SIZE, 35 + TOP_BUFFER + CV_DIST)), Port::INPUT, module, Acid::WAVE_MIX_INPUT));

        // Envelope
        addParam(ParamWidget::create<AcidRoundLargeBlackKnob>(mm2px(Vec(5 + LEFT_BUFFER, 20 + BOTTOM_OFFSET + TOP_BUFFER)), module, Acid::ENV_REL_PARAM, 0.0, 1.0, 0.0));
        addParam(ParamWidget::create<AcidRoundLargeBlackKnob>(mm2px(Vec(30 + LEFT_BUFFER, 20 + BOTTOM_OFFSET + TOP_BUFFER)), module, Acid::ENV_AMT_PARAM, 0.0, 1.0, 1.0));
        addParam(ParamWidget::create<AcidRoundLargeBlackKnob>(mm2px(Vec(17.5 + LEFT_BUFFER, 35 + BOTTOM_OFFSET + TOP_BUFFER)), module, Acid::ENV_SHAPE_PARAM, -1.0, 1.0, 0.0));

        addInput(Port::create<PJ301MPort>(mm2px(Vec(5 + LEFT_BUFFER + CV_SIZE, 20 + TOP_BUFFER + BOTTOM_OFFSET + CV_DIST)), Port::INPUT, module, Acid::WAVE_1_INPUT));
        addInput(Port::create<PJ301MPort>(mm2px(Vec(30 + LEFT_BUFFER+ CV_SIZE, 20 + TOP_BUFFER + BOTTOM_OFFSET + CV_DIST)), Port::INPUT, module, Acid::WAVE_2_INPUT));
        addInput(Port::create<PJ301MPort>(mm2px(Vec(17.5 + LEFT_BUFFER + CV_SIZE, 35 + TOP_BUFFER + BOTTOM_OFFSET + CV_DIST)), Port::INPUT, module, Acid::WAVE_MIX_INPUT));

        /*
            Right Side
        */

        // Filter
        addParam(ParamWidget::create<AcidRoundLargeBlackKnob>(mm2px(Vec(17.5 + LEFT_BUFFER + RIGHT_BUFFER, 20 + TOP_BUFFER)), module, Acid::FILTER_CUT_PARAM, 0.0, 1.0f, 0.90f));
        addParam(ParamWidget::create<AcidRoundLargeBlackKnob>(mm2px(Vec(5 + LEFT_BUFFER + RIGHT_BUFFER, 35 + TOP_BUFFER)), module, Acid::FILTER_FM_1_PARAM, -1.0, 1.0f, 0.0f));
        addParam(ParamWidget::create<AcidRoundLargeBlackKnob>(mm2px(Vec(30 + LEFT_BUFFER + RIGHT_BUFFER, 35 + TOP_BUFFER)), module, Acid::FILTER_Q_PARAM, 0.1f, 1.5f, 0.3f));
        addParam(ParamWidget::create<AcidRoundLargeBlackKnob>(mm2px(Vec(5 + LEFT_BUFFER + RIGHT_BUFFER, 50 + TOP_BUFFER)), module, Acid::FILTER_FM_2_PARAM, -1.0, 1.0f, 0.0f));
        addParam(ParamWidget::create<AcidRoundLargeBlackKnob>(mm2px(Vec(30 + LEFT_BUFFER + RIGHT_BUFFER, 50 + TOP_BUFFER)), module, Acid::FILTER_DRIVE_PARAM, -5.0f, 5.0f, 5.0f));

        addInput(Port::create<PJ301MPort>(mm2px(Vec(17.5 + LEFT_BUFFER + RIGHT_BUFFER + CV_SIZE, 20 + TOP_BUFFER + CV_DIST)), Port::INPUT, module, Acid::FILTER_CUT_INPUT));
        addInput(Port::create<PJ301MPort>(mm2px(Vec(5 + LEFT_BUFFER + RIGHT_BUFFER + CV_SIZE, 35 + TOP_BUFFER - CV_NEG_DIST)), Port::INPUT, module, Acid::FILTER_FM_1_INPUT));
        addInput(Port::create<PJ301MPort>(mm2px(Vec(30 + LEFT_BUFFER + RIGHT_BUFFER + CV_SIZE, 35 + TOP_BUFFER - CV_NEG_DIST)), Port::INPUT, module, Acid::FILTER_Q_INPUT));
        addInput(Port::create<PJ301MPort>(mm2px(Vec(5 + LEFT_BUFFER + RIGHT_BUFFER + CV_SIZE, 50 + TOP_BUFFER + CV_DIST)), Port::INPUT, module, Acid::FILTER_FM_2_INPUT));
        addInput(Port::create<PJ301MPort>(mm2px(Vec(30 + LEFT_BUFFER + RIGHT_BUFFER + CV_SIZE, 50 + TOP_BUFFER + CV_DIST)), Port::INPUT, module, Acid::FILTER_DRIVE_INPUT));


        // Pluck
        addParam(ParamWidget::create<AcidRoundLargeBlackKnob>(mm2px(Vec(5 + LEFT_BUFFER + RIGHT_BUFFER, 35 + BOTTOM_OFFSET + TOP_BUFFER)), module, Acid::PLUCK_REL_PARAM, 0.2, 0.4f, 0.50f));
        addParam(ParamWidget::create<AcidRoundLargeBlackKnob>(mm2px(Vec(30 + LEFT_BUFFER + RIGHT_BUFFER, 35 + BOTTOM_OFFSET + TOP_BUFFER)), module, Acid::PLUCK_EXP_PARAM, 0.0001f, .2f, 4.0f));
        // addParam(ParamWidget::create<AcidRoundLargeBlackKnob>(mm2px(Vec(17.5 + LEFT_BUFFER + RIGHT_BUFFER, 20 + BOTTOM_OFFSET + TOP_BUFFER)), module, Acid::PLUCK_ATTACK_PARAM, 0.0, 1.0f, 0.90f));

        addInput(Port::create<PJ301MPort>(mm2px(Vec(5 + LEFT_BUFFER + RIGHT_BUFFER + CV_SIZE, 35 + TOP_BUFFER + BOTTOM_OFFSET + CV_DIST)), Port::INPUT, module, Acid::PLUCK_REL_INPUT));
        addInput(Port::create<PJ301MPort>(mm2px(Vec(30 + LEFT_BUFFER + RIGHT_BUFFER + CV_SIZE, 35 + TOP_BUFFER + BOTTOM_OFFSET + CV_DIST)), Port::INPUT, module, Acid::PLUCK_EXP_INPUT));

        /*
            Bottom
        */
        float OUTLINE = 117.5f;
        addInput(Port::create<PJ301MPort>(mm2px(Vec(5 + LEFT_BUFFER + CV_SIZE, OUTLINE)), Port::INPUT, module, Acid::VOCT_INPUT));
        addInput(Port::create<PJ301MPort>(mm2px(Vec(17.5 + LEFT_BUFFER + CV_SIZE, OUTLINE)), Port::INPUT, module, Acid::VOCT2_INPUT));
        addInput(Port::create<PJ301MPort>(mm2px(Vec(30 + LEFT_BUFFER + CV_SIZE, OUTLINE)), Port::INPUT, module, Acid::TRIG_INPUT));

        addOutput(Port::create<PJ301MPort>(mm2px(Vec(17.5 + LEFT_BUFFER + RIGHT_BUFFER + CV_SIZE, OUTLINE)), Port::OUTPUT, module, Acid::ENV_OUTPUT));
        addOutput(Port::create<PJ301MPort>(mm2px(Vec(30 + LEFT_BUFFER + RIGHT_BUFFER + CV_SIZE, OUTLINE)), Port::OUTPUT, module, Acid::OUT_OUTPUT));
    }
};


Model *modelAcid = Model::create<Acid, AcidWidget>("RJModules", "Acid", "[VCO] Acid", AMPLIFIER_TAG);
