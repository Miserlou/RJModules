/*
KTF - Key Tracking Filter
VCF from Fundamental, tuned, plus a PBF.
*/

#include "RJModules.hpp"


using simd::float_4;

struct KTFRoundHugeBlackSnapKnob : RoundHugeBlackKnob
{
    KTFRoundHugeBlackSnapKnob()
    {
        setSVG(SVG::load(assetPlugin(pluginInstance, "res/KTFRoundHugeBlackKnob.svg")));
        minAngle = -0.83 * M_PI;
        maxAngle = 0.83 * M_PI;
        snap = true;
    }
};

struct KTFRoundLargeBlackKnob : RoundLargeBlackKnob
{
    KTFRoundLargeBlackKnob()
    {
        setSVG(SVG::load(assetPlugin(pluginInstance, "res/KTFRoundLargeBlackKnob.svg")));
    }
};


template <typename T>
static T clip(T x) {
    // return std::tanh(x);
    // Pade approximant of tanh
    x = simd::clamp(x, -3.f, 3.f);
    return x * (27 + x * x) / (27 + 9 * x * x);
}


template <typename T>
struct KTFLadderFilter {
    T omega0;
    T resonance = 1;
    T state[4];
    T input;

    KTFLadderFilter() {
        reset();
        setCutoff(0);
    }

    void reset() {
        for (int i = 0; i < 4; i++) {
            state[i] = 0;
        }
    }

    void setCutoff(T cutoff) {
        omega0 = 2 * T(M_PI) * cutoff;
    }

    void process(T input, T dt) {
        dsp::stepRK4(T(0), dt, state, 4, [&](T t, const T x[], T dxdt[]) {
            T inputc = clip(input - resonance * x[3]);
            T yc0 = clip(x[0]);
            T yc1 = clip(x[1]);
            T yc2 = clip(x[2]);
            T yc3 = clip(x[3]);

            dxdt[0] = omega0 * (inputc - yc0);
            dxdt[1] = omega0 * (yc0 - yc1);
            dxdt[2] = omega0 * (yc1 - yc2);
            dxdt[3] = omega0 * (yc2 - yc3);
        });

        this->input = input;
    }

    T lowpass() {
        return state[3];
    }
    T highpass() {
        // TODO This is incorrect when `resonance > 0`. Is the math wrong?
        return clip((input - resonance*state[3]) - 4 * state[0] + 6*state[1] - 4*state[2] + state[3]);
    }
    T bandpass() {
        // TODO This is incorrect when `resonance > 0`. Is the math wrong?
        return (state[3] + clip((input - resonance*state[3]) - 4 * state[0] + 6*state[1] - 4*state[2] + state[3])) / 2;
    }

};


static const int UPSAMPLE = 2;

struct KTF : Module {
    enum ParamIds {
        OCT_PARAM,
        FINE_PARAM,
        RES_PARAM,
        GLIDE_PARAM,
        FREQ_CV_PARAM,
        DRIVE_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        OCT_INPUT,
        FREQ_INPUT,
        RES_INPUT,
        FINE_INPUT,
        DRIVE_INPUT,
        IN_INPUT,
        NUM_INPUTS
    };
    enum OutputIds {
        LPF_OUTPUT,
        HPF_OUTPUT,
        BPF_OUTPUT,
        NUM_OUTPUTS
    };

    KTFLadderFilter<float_4> filters[4];
    float glide_state;

    KTF() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS);
        // Multiply and offset for backward patch compatibility
        configParam(OCT_PARAM, -8.5, 8.5, 0.0, "Octave", " Oct");
        configParam(FINE_PARAM, -0.2f, 0.2f, 0.0f, "Fine frequency");
        configParam(RES_PARAM, 0.f, 1.f, .4f, "Resonance", "%", 0.f, 100.f);
        configParam(GLIDE_PARAM , 0.0f, 10.0f, 0.0001f, "Glide amount");
        configParam(FREQ_CV_PARAM, -1.f, 1.f, 0.f, "Frequency modulation", "%", 0.f, 100.f);
        configParam(DRIVE_PARAM, 0.f, 1.f, 0.f, "Drive", "", 0, 11);
    }

    void onReset() override {
        for (int i = 0; i < 4; i++)
            filters[i].reset();
    }

    void process(const ProcessArgs &args) override {
        //if (!outputs[LPF_OUTPUT].isConnected() && !outputs[HPF_OUTPUT].isConnected() && !outputs[BPF_OUTPUT].isConnected()) {
        if (!outputs[LPF_OUTPUT].isConnected()) {
            return;
        }

        float driveParam = params[DRIVE_PARAM].getValue();
        float resParam = params[RES_PARAM].getValue();
        float fineParam = .711f; // Magic number!
        fineParam = fineParam + params[FINE_PARAM].getValue() + inputs[FINE_INPUT].value / 10.f;
        fineParam = dsp::quadraticBipolar(fineParam * 2.f - 1.f) * 7.f / 12.f;

        float freqParam = 1.f;
        freqParam = freqParam * 10.f - 5.f;
        //freqParam = freqParam + round(params[FREQ_CV_PARAM].getValue()

        int channels = std::max(1, inputs[IN_INPUT].getChannels());

        for (int c = 0; c < channels; c += 4) {
            auto *filter = &filters[c / 4];

            float_4 input = float_4::load(inputs[IN_INPUT].getVoltages(c)) / 5.f;

            // Drive gain
            float_4 drive = driveParam + inputs[DRIVE_INPUT].getPolyVoltageSimd<float_4>(c) / 10.f;
            drive = clamp(drive, 0.f, 1.f);
            float_4 gain = simd::pow(1.f + drive, 5);
            input *= gain;

            // Add -120dB noise to bootstrap self-oscillation
            input += 1e-6f * (2.f * random::uniform() - 1.f);

            // Set resonance
            float_4 resonance = resParam + inputs[RES_INPUT].getPolyVoltageSimd<float_4>(c) / 10.f;
            resonance = clamp(resonance, 0.f, 1.f);
            filter->resonance = simd::pow(resonance, 2) * 10.f;

            // Get pitch
            float_4 pitch = freqParam + fineParam + inputs[FREQ_INPUT].getPolyVoltageSimd<float_4>(c) + round(params[OCT_PARAM].getValue());

            float gp = params[GLIDE_PARAM].getValue();
            if (gp == 0.f){
                pitch = pitch;
            }
            else if (pitch[0] > glide_state){
                pitch[0] = glide_state + (.00001 * (10 - (gp)));
                glide_state = pitch[0];
            } else {
                pitch[0] = glide_state - (.00001 * (10 - (gp)));
                glide_state = pitch[0];
            }

            // Set cutoff
            float_4 cutoff = dsp::FREQ_C4 * simd::pow(2.f, pitch);
            cutoff = clamp(cutoff, 1.f, 21000.f);
            filter->setCutoff(cutoff);

            // Set outputs
            filter->process(input, args.sampleTime);

            float_4 lowpass = 5.f * filter->lowpass();
            lowpass.store(outputs[LPF_OUTPUT].getVoltages(c));

            // float_4 highpass = 5.f * filter->highpass();
            // highpass.store(outputs[HPF_OUTPUT].getVoltages(c));

            // float_4 bandpass = 5.f * filter->bandpass();
            // bandpass.store(outputs[BPF_OUTPUT].getVoltages(c));
        }

        outputs[LPF_OUTPUT].setChannels(channels);
        outputs[HPF_OUTPUT].setChannels(channels);
        outputs[BPF_OUTPUT].setChannels(channels);

    }
};


struct KTFWidget : ModuleWidget {
    KTFWidget(KTF *module) {
        setModule(module);
        setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/KTF.svg")));

        addChild(createWidget<ScrewSilver>(Vec(15, 0)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x - 30, 0)));
        addChild(createWidget<ScrewSilver>(Vec(15, 365)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x - 30, 365)));

        addParam(createParam<KTFRoundHugeBlackSnapKnob>(Vec(33, 61), module, KTF::OCT_PARAM));
        addParam(createParam<KTFRoundLargeBlackKnob>(Vec(12, 143), module, KTF::FINE_PARAM));
        addParam(createParam<KTFRoundLargeBlackKnob>(Vec(71, 143), module, KTF::RES_PARAM));
        addParam(createParam<KTFRoundLargeBlackKnob>(Vec(12, 208), module, KTF::GLIDE_PARAM));
        addParam(createParam<KTFRoundLargeBlackKnob>(Vec(71, 208), module, KTF::DRIVE_PARAM));

        addInput(createInput<PJ301MPort>(Vec(10, 276), module, KTF::OCT_INPUT));
        addInput(createInput<PJ301MPort>(Vec(48, 276), module, KTF::RES_INPUT));
        addInput(createInput<PJ301MPort>(Vec(85, 276), module, KTF::DRIVE_INPUT));

        addInput(createInput<PJ301MPort>(Vec(10, 320), module, KTF::IN_INPUT));
        addInput(createInput<PJ301MPort>(Vec(48, 320), module, KTF::FREQ_INPUT));
        addOutput(createOutput<PJ301MPort>(Vec(85, 320), module, KTF::LPF_OUTPUT));

        // These don't work!
        // addOutput(createOutput<PJ301MPort>(Vec(10, 320), module, KTF::BPF_OUTPUT));
        // addOutput(createOutput<PJ301MPort>(Vec(85, 320), module, KTF::HPF_OUTPUT));
    }
};


Model *modelKTF = createModel<KTF, KTFWidget>("KTF");
