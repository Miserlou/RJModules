/*

The fun is here: http://cgm.cs.mcgill.ca/~godfried/publications/banff.pdf

*/

#include "RJModules.hpp"
#include "common.hpp"
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

struct EuclidianSmallStringDisplayWidget : TransparentWidget {

  std::string *value;
  std::shared_ptr<Font> font;

  EuclidianSmallStringDisplayWidget() {
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

    Vec textPos = Vec(6.0f, 24.0f);
    NVGcolor textColor = nvgRGB(0x00, 0x00, 0x00);
    nvgFillColor(vg, textColor);
    nvgText(vg, textPos.x, textPos.y, to_display.str().c_str(), NULL);
  }
};

struct EuclidianRoundLargeBlackKnob : RoundLargeBlackKnob
{
    EuclidianRoundLargeBlackKnob()
    {
        setSVG(SVG::load(assetPlugin(pluginInstance, "res/KTFRoundHugeBlackKnob.svg")));
    }
};

struct EuclidianRoundLargeBlackSnapKnob : RoundLargeBlackKnob
{
    EuclidianRoundLargeBlackSnapKnob()
    {
        setSVG(SVG::load(assetPlugin(pluginInstance, "res/KTFRoundHugeBlackKnob.svg")));
        minAngle = -0.83 * M_PI;
        maxAngle = 0.83 * M_PI;
        snap = true;
    }
};

struct EuclidianRoundBlackSnapKnob : RoundBlackKnob
{
    EuclidianRoundBlackSnapKnob()
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

struct Euclidian : Module {
    enum ParamIds {
        K_PARAM,
        N_PARAM,
        I_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        CLOCK_INPUT,
        RESET_INPUT,

        NUM_INPUTS
    };
    enum OutputIds {
        OUTPUT,

        NUM_OUTPUTS
    };
    enum LightIds {
        NUM_LIGHTS
    };

    // Display
    std::string k_display = "4";
    std::string n_display = "12";

    // From AS + Koral
    // BPM detector variables
    bool inMemory = false;
    bool beatLock = false;
    float beatTime = 0.0f;
    int beatCount = 0;
    int beatCountMemory = 0;
    float beatOld = 0.0f;
    std::string tempo = "---";
    dsp::SchmittTrigger clockTrigger;
    dsp::PulseGenerator LightPulse;
    bool pulse = false;

    // Calculator variables
    float bpm = 120;
    float last_bpm = 0;
    float millisecs = 60000;
    float mult = 1000;
    float millisecondsPerBeat;
    float millisecondsPerMeasure;
    float bar = 1.0f;
    float secondsPerBeat = 0.0f;
    float secondsPerMeasure = 0.0f;

    //ms variables
    float half_note_d = 1.0f;
    float half_note = 1.0f;
    float half_note_t =1.0f;

    float qt_note_d = 1.0f;
    float qt_note = 1.0f;
    float qt_note_t = 1.0f;

    float eight_note_d = 1.0f;
    float eight_note =1.0f;
    float eight_note_t = 1.0f;

    float sixth_note_d =1.0f;
    float sixth_note = 1.0f;
    float sixth_note_t = 1.0f;

    float trth_note_d = 1.0f;
    float trth_note = 1.0f;
    float trth_note_t = 1.0f;

    /* Delays - Left*/
    dsp::RCFilter lowpassFilter;
    dsp::RCFilter highpassFilter;

    dsp::DoubleRingBuffer<float, HISTORY_SIZE> historyBuffer;
    dsp::DoubleRingBuffer<float, 16> outBuffer;

    dsp::SampleRateConverter<1> src;

    dsp::SchmittTrigger bypass_button_trig;
    dsp::SchmittTrigger bypass_cv_trig;

    int lcd_tempo = 0;
    bool fx_bypass = false;
    float lastWet = 0.0f;

    float fade_in_fx = 0.0f;
    float fade_in_dry = 0.0f;
    float fade_out_fx = 1.0f;
    float fade_out_dry = 1.0f;
    const float fade_speed = 0.001f;

    /* Menu Settings*/
    int feedback_mode_index = 0;
    int poly_mode_index = 0;
    int last_poly_mode_index = 0;

    /* Input Caching */
    int param_counter = 7;
    float FEEDBACK_PARAM_value;
    float MIX_PARAM_value;
    float RATE_PARAM_value;
    float NUDGE_PARAM_value;
    float COLOR_PARAM_value;

    void refreshDetector() {
    }

    void onReset() override {
        refreshDetector();
    }

    void onInitialize() {
        refreshDetector();
    }

    Euclidian() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(Euclidian::K_PARAM, 0, 128, 5, "k");
        configParam(Euclidian::N_PARAM, 0, 128, 8, "n");
        configParam(Euclidian::I_PARAM, 0, 16, 0, "i");
    }

    // From https://bitbucket.org/sjcastroe/bjorklunds-algorithm/src/master/
    std::string bjorklund(int beats, int steps)
    {
        //We can only have as many beats as we have steps (0 <= beats <= steps)
        if (beats > steps)
            beats = steps;

        //Each iteration is a process of pairing strings X and Y and the remainder from the pairings
        //X will hold the "dominant" pair (the pair that there are more of)
        std::string x = "1";
        int x_amount = beats;

        std::string y = "0";
        int y_amount = steps - beats;

        do
        {
            //Placeholder variables
            int x_temp = x_amount;
            int y_temp = y_amount;
            std::string y_copy = y;

            //Check which is the dominant pair
            if (x_temp >= y_temp)
            {
                //Set the new number of pairs for X and Y
                x_amount = y_temp;
                y_amount = x_temp - y_temp;

                //The previous dominant pair becomes the new non dominant pair
                y = x;
            }
            else
            {
                x_amount = x_temp;
                y_amount = y_temp - x_temp;
            }

            //Create the new dominant pair by combining the previous pairs
            x = x + y_copy;
        } while (x_amount > 1 && y_amount > 1);//iterate as long as the non dominant pair can be paired (if there is 1 Y left, all we can do is pair it with however many Xs are left, so we're done)

        //By this point, we have strings X and Y formed through a series of pairings of the initial strings "1" and "0"
        //X is the final dominant pair and Y is the second to last dominant pair
        std::string rhythm;
        for (int i = 1; i <= x_amount; i++)
            rhythm += x;
        for (int i = 1; i <= y_amount; i++)
            rhythm += y;
        return rhythm;
    }

    // In-place rotates s towards left by d
    void rotate(std::string &s, int d)
    {
        reverse(s.begin(), s.begin() + d);
        reverse(s.begin()+d, s.end());
        reverse(s.begin(), s.end());
    }

    void process(const ProcessArgs &args) override {

        int K = (int)params[K_PARAM].getValue();
        int N = (int)params[N_PARAM].getValue();
        int I = (int)params[I_PARAM].getValue();

        k_display = std::to_string(K);
        n_display = std::to_string(N);

        std::string bork = bjorklund(K, N);
        std::cout << bork << "\n";

        rotate(bork, I);
        std::cout << bork << "\n";

    }
};

struct EuclidianWidget : ModuleWidget {
  EuclidianWidget(Euclidian *module) {
    setModule(module);
    box.size = Vec(15*10, 380);

    SVGPanel *panel = new SVGPanel();
    panel->box.size = box.size;
    panel->setBackground(SVG::load(assetPlugin(pluginInstance, "res/Euclidian.svg")));
    addChild(panel);

    // Displays
    if(module != NULL){
        EuclidianSmallStringDisplayWidget *k_Display = new EuclidianSmallStringDisplayWidget();
        k_Display->box.pos = Vec(45, 48);
        k_Display->box.size = Vec(35, 35);
        k_Display->value = &module->k_display;
        addChild(k_Display);
    }
    if(module != NULL){
        EuclidianSmallStringDisplayWidget *n_Display = new EuclidianSmallStringDisplayWidget();
        n_Display->box.pos = Vec(100, 48);
        n_Display->box.size = Vec(35, 35);
        n_Display->value = &module->n_display;
        addChild(n_Display);
    }

    // Knobs
    int LEFT = 14;
    int RIGHT = 65;
    int DIST = 82;
    int BASE = 115;
    // addParam(createParam<EuclidianRoundBlackSnapKnob>(Vec(100, 50), module, Euclidian::RATE_PARAM));
    addParam(createParam<EuclidianRoundLargeBlackSnapKnob>(Vec(LEFT, BASE), module, Euclidian::K_PARAM));
    addParam(createParam<EuclidianRoundLargeBlackSnapKnob>(Vec(LEFT + RIGHT, BASE), module, Euclidian::N_PARAM));
    addParam(createParam<EuclidianRoundLargeBlackSnapKnob>(Vec(LEFT, BASE + DIST), module, Euclidian::I_PARAM));
    // addParam(createParam<EuclidianRoundLargeBlackKnob>(Vec(LEFT + RIGHT, BASE + DIST), module, Euclidian::MIX_PARAM));

    // Inputs and Knobs
    // addOutput(createPort<PJ301MPort>(Vec(11, 277), PortWidget::OUTPUT, module, Euclidian::COLOR_SEND));
    // addInput(createPort<PJ301MPort>(Vec(45, 277), PortWidget::INPUT, module, Euclidian::COLOR_RETURN));
    // addOutput(createPort<PJ301MPort>(Vec(80, 277), PortWidget::OUTPUT, module, Euclidian::COLOR_SEND_RIGHT));
    // addInput(createPort<PJ301MPort>(Vec(112.5, 277), PortWidget::INPUT, module, Euclidian::COLOR_RETURN_RIGHT));

    addInput(createPort<PJ301MPort>(Vec(11, 320), PortWidget::INPUT, module, Euclidian::CLOCK_INPUT));
    addInput(createPort<PJ301MPort>(Vec(45, 320), PortWidget::INPUT, module, Euclidian::RESET_INPUT));

    addOutput(createPort<PJ301MPort>(Vec(112.5, 320), PortWidget::OUTPUT, module, Euclidian::OUTPUT));
    }

    // blah needs getters and setters
    // json_t *toJson() override {
    //     json_t *rootJ = ModuleWidget::toJson();
    //     json_object_set_new(rootJ, "feed", json_real(feedback_mode_index));
    //     json_object_set_new(rootJ, "poly", json_real(poly_mode_index));
    //     return rootJ;
    // }

    // void fromJson(json_t *rootJ) override {
    //     ModuleWidget::fromJson(rootJ);
    //     json_t *feedJ = json_object_get(rootJ, "feed");
    //     if (feedJ)
    //         feedback_mode_index = json_number_value(feedJ);
    //     json_t *polyJ = json_object_get(rootJ, "poly");
    //     if (polyJ)
    //         poly_mode_index = json_number_value(polyJ);
    // }

    void appendContextMenu(Menu *menu) override
    {
        Euclidian *module = dynamic_cast<Euclidian *>(this->module);

        struct FeedbackIndexItem : MenuItem
        {
            Euclidian *module;
            int index;
            void onAction(const event::Action &e) override
            {
                module->feedback_mode_index = index;
            }
        };

        struct FeedbackItem : MenuItem
        {
            Euclidian *module;
            Menu *createChildMenu() override
            {
                Menu *menu = new Menu();
                const std::string feedbackLabels[] = {
                    "0\% - 100\%",
                    "0\% - 200\%"
                };
                for (int i = 0; i < (int)LENGTHOF(feedbackLabels); i++)
                {
                    FeedbackIndexItem *item = createMenuItem<FeedbackIndexItem>(feedbackLabels[i], CHECKMARK(module->feedback_mode_index == i));
                    item->module = module;
                    item->index = i;
                    menu->addChild(item);
                }
                return menu;
            }
        };

        struct PolyIndexItem : MenuItem
        {
            Euclidian *module;
            int index;
            void onAction(const event::Action &e) override
            {
                module->poly_mode_index = index;
            }
        };

        struct PolyItem : MenuItem
        {
            Euclidian *module;
            Menu *createChildMenu() override
            {
                Menu *menu = new Menu();
                const std::string polyLabels[] = {
                    "Mono Out",
                    "Poly Out"
                };
                for (int i = 0; i < (int)LENGTHOF(polyLabels); i++)
                {
                    PolyIndexItem *item = createMenuItem<PolyIndexItem>(polyLabels[i], CHECKMARK(module->poly_mode_index == i));
                    item->module = module;
                    item->index = i;
                    menu->addChild(item);
                }
                return menu;
            }
        };

        menu->addChild(new MenuEntry);

        FeedbackItem *feedbackItem = createMenuItem<FeedbackItem>("Feedback Mode", ">");
        feedbackItem->module = module;
        menu->addChild(feedbackItem);

        PolyItem *polyItem = createMenuItem<PolyItem>("Poly Mode", ">");
        polyItem->module = module;
        menu->addChild(polyItem);
    }

};

Model *modelEuclidian = createModel<Euclidian, EuclidianWidget>("Euclidian");
