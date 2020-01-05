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
        J_PARAM,
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
        ENUMS(PATTERN_LIGHT, 16),
        NUM_LIGHTS
    };

    // Display
    std::string k_display = "4";
    std::string n_display = "12";

    // Player
    int head = -1;
    SchmittTrigger clockTrigger;
    SchmittTrigger resetTrigger;
    int old_K;
    int old_J;
    int old_I;
    int old_N;
    std::string old_final_string;

    Euclidian() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(Euclidian::K_PARAM, 0, 128, 4, "k");
        configParam(Euclidian::N_PARAM, 1, 128, 12, "n");
        configParam(Euclidian::I_PARAM, 0, 16, 0, "i");
        configParam(Euclidian::J_PARAM, 1, 15, 1, "j");
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

    void rotate(std::string &a, int d){
        if(d>=a.length()){
            return;
        }
        std::rotate(a.begin(), a.begin() + d, a.end());
    }

    void multiply(std::string &s, int d)
    {
        std::string new_s = "";
        for(char& c : s) {
            for(int i=0; i<d; i++){
                new_s = new_s + c;
            }
        }
        s = new_s;
    }

    void process(const ProcessArgs &args) override {

        int K = (int)params[K_PARAM].getValue();
        int N = (int)params[N_PARAM].getValue();
        int I = (int)params[I_PARAM].getValue();
        int J = (int)params[J_PARAM].getValue();

        k_display = std::to_string(K);
        n_display = std::to_string(N);
        std::string final_string;

        if( (old_K == K) && (old_J == J) && (old_I == I) && (old_N == N) ){
            final_string = old_final_string;
        } else{
            std::string bork = bjorklund(K, N);
            rotate(bork, I);
            multiply(bork, J);
            final_string = bork;
        }

        old_final_string = final_string;
        old_K = K;
        old_N = N;
        old_I = I;
        old_J = J;

        if (resetTrigger.process(inputs[RESET_INPUT].value)) {
            head = -1;
        }

        if (clockTrigger.process(inputs[CLOCK_INPUT].value)) {
            if (head >= final_string.length() -1){
                head = -1;
            }
            head += 1;
            if(final_string[head] == "1"[0]){
                outputs[OUTPUT].value = 12.0f;
            } else{
                outputs[OUTPUT].value = 0.0f;
            }
        } else{
            outputs[OUTPUT].value = 0.0f;
        }

        for(int li=0;li<16;li++){
            if(li>final_string.length()){
                lights[PATTERN_LIGHT + li].setBrightness(0);
                continue;
            }

            if(li==head){
                continue;
            }

            if(final_string[li] == "1"[0]){
                lights[PATTERN_LIGHT + li].setBrightness(.5);
            } else{
                lights[PATTERN_LIGHT + li].setBrightness(0);
            }
        }

        if(head<16){
            if(final_string[head] == "1"[0]){
                lights[PATTERN_LIGHT + head].setBrightness(1.0f);
            } else{
                lights[PATTERN_LIGHT + head].setBrightness(0.75f);
            }
        }

    }
};

template <typename BASE>
struct EuclidianLight : BASE {
    EuclidianLight() {
        this->box.size = mm2px(Vec(3.0, 3.0));
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
    addParam(createParam<EuclidianRoundLargeBlackSnapKnob>(Vec(LEFT + RIGHT, BASE + DIST), module, Euclidian::J_PARAM));
    addInput(createPort<PJ301MPort>(Vec(11, 320), PortWidget::INPUT, module, Euclidian::CLOCK_INPUT));
    addInput(createPort<PJ301MPort>(Vec(45, 320), PortWidget::INPUT, module, Euclidian::RESET_INPUT));

    for(int j=0;j<8;j++){
        addChild(createLight<EuclidianLight<WhiteLight>>(Vec(j * 16 + 15, 265), module, Euclidian::PATTERN_LIGHT + j));
    }
    for(int k=0;k<8;k++){
        addChild(createLight<EuclidianLight<WhiteLight>>(Vec(k * 16 + 15, 285), module, Euclidian::PATTERN_LIGHT + (k + 8)));
    }

    addOutput(createPort<PJ301MPort>(Vec(112.5, 320), PortWidget::OUTPUT, module, Euclidian::OUTPUT));

    }

    void appendContextMenu(Menu *menu) override
    {
        Euclidian *module = dynamic_cast<Euclidian *>(this->module);

        struct PattIndexItem : MenuItem
        {
            Euclidian *module;
            int index;
            void onAction(const event::Action &e) override
            {
                if(index == 0){
                    module->params[Euclidian::K_PARAM].setValue(4);
                    module->params[Euclidian::N_PARAM].setValue(12);
                    module->params[Euclidian::I_PARAM].setValue(0);
                    module->params[Euclidian::J_PARAM].setValue(1);
                }
                if(index == 1){
                    module->params[Euclidian::K_PARAM].setValue(2);
                    module->params[Euclidian::N_PARAM].setValue(3);
                    module->params[Euclidian::I_PARAM].setValue(0);
                    module->params[Euclidian::J_PARAM].setValue(1);
                }
                if(index == 2){
                    module->params[Euclidian::K_PARAM].setValue(2);
                    module->params[Euclidian::N_PARAM].setValue(5);
                    module->params[Euclidian::I_PARAM].setValue(0);
                    module->params[Euclidian::J_PARAM].setValue(1);
                }
                if(index == 3){
                    module->params[Euclidian::K_PARAM].setValue(3);
                    module->params[Euclidian::N_PARAM].setValue(4);
                    module->params[Euclidian::I_PARAM].setValue(0);
                    module->params[Euclidian::J_PARAM].setValue(1);
                }
                if(index == 4){
                    module->params[Euclidian::K_PARAM].setValue(3);
                    module->params[Euclidian::N_PARAM].setValue(7);
                    module->params[Euclidian::I_PARAM].setValue(0);
                    module->params[Euclidian::J_PARAM].setValue(1);
                }
                if(index == 5){
                    module->params[Euclidian::K_PARAM].setValue(3);
                    module->params[Euclidian::N_PARAM].setValue(8);
                    module->params[Euclidian::I_PARAM].setValue(0);
                    module->params[Euclidian::J_PARAM].setValue(1);
                }
                if(index == 6){
                    module->params[Euclidian::K_PARAM].setValue(4);
                    module->params[Euclidian::N_PARAM].setValue(9);
                    module->params[Euclidian::I_PARAM].setValue(0);
                    module->params[Euclidian::J_PARAM].setValue(1);
                }
                if(index == 7){
                    module->params[Euclidian::K_PARAM].setValue(4);
                    module->params[Euclidian::N_PARAM].setValue(11);
                    module->params[Euclidian::I_PARAM].setValue(0);
                    module->params[Euclidian::J_PARAM].setValue(1);
                }
                if(index == 8){
                    module->params[Euclidian::K_PARAM].setValue(5);
                    module->params[Euclidian::N_PARAM].setValue(6);
                    module->params[Euclidian::I_PARAM].setValue(4);
                    module->params[Euclidian::J_PARAM].setValue(1);
                }
                if(index == 9){
                    module->params[Euclidian::K_PARAM].setValue(5);
                    module->params[Euclidian::N_PARAM].setValue(7);
                    module->params[Euclidian::I_PARAM].setValue(0);
                    module->params[Euclidian::J_PARAM].setValue(1);
                }
                if(index == 10){
                    module->params[Euclidian::K_PARAM].setValue(5);
                    module->params[Euclidian::N_PARAM].setValue(8);
                    module->params[Euclidian::I_PARAM].setValue(4);
                    module->params[Euclidian::J_PARAM].setValue(1);
                }
                if(index == 11){
                    module->params[Euclidian::K_PARAM].setValue(5);
                    module->params[Euclidian::N_PARAM].setValue(9);
                    module->params[Euclidian::I_PARAM].setValue(4);
                    module->params[Euclidian::J_PARAM].setValue(1);
                }
                if(index == 12){
                    module->params[Euclidian::K_PARAM].setValue(5);
                    module->params[Euclidian::N_PARAM].setValue(16);
                    module->params[Euclidian::I_PARAM].setValue(6);
                    module->params[Euclidian::J_PARAM].setValue(1);
                }
                if(index == 13){
                    module->params[Euclidian::K_PARAM].setValue(7);
                    module->params[Euclidian::N_PARAM].setValue(8);
                    module->params[Euclidian::I_PARAM].setValue(0);
                    module->params[Euclidian::J_PARAM].setValue(1);
                }
                if(index == 14){
                    module->params[Euclidian::K_PARAM].setValue(7);
                    module->params[Euclidian::N_PARAM].setValue(12);
                    module->params[Euclidian::I_PARAM].setValue(0);
                    module->params[Euclidian::J_PARAM].setValue(1);
                }
                if(index == 15){
                    module->params[Euclidian::K_PARAM].setValue(7);
                    module->params[Euclidian::N_PARAM].setValue(16);
                    module->params[Euclidian::I_PARAM].setValue(5);
                    module->params[Euclidian::J_PARAM].setValue(1);
                }
                if(index == 16){
                    module->params[Euclidian::K_PARAM].setValue(9);
                    module->params[Euclidian::N_PARAM].setValue(16);
                    module->params[Euclidian::I_PARAM].setValue(4);
                    module->params[Euclidian::J_PARAM].setValue(1);
                }
                if(index == 17){
                    module->params[Euclidian::K_PARAM].setValue(13);
                    module->params[Euclidian::N_PARAM].setValue(24);
                    module->params[Euclidian::I_PARAM].setValue(8);
                    module->params[Euclidian::J_PARAM].setValue(1);
                }
            }
        };

        struct PattItem : MenuItem
        {
            Euclidian *module;
            Menu *createChildMenu() override
            {
                Menu *menu = new Menu();
                const std::string polyLabels[] = {
                    "Flamenco", // E(4, 12)
                    "Swing Tumbao", // E(2, 3)
                    "Khafif-e-ramal", //E(2, 5)
                    "Cumbia / Calypso", // E(3, 4)
                    "Ruchenitza", // E(3, 7)
                    "Tresillo", // E(3, 8)
                    "Aksak", // E(4,9)
                    "Zappa", // E(4,11)
                    "York-Samai", //E(5,6) + 4
                    "Nawakhat", //E(5,7)
                    "Cinquillo / Tango", // E(5,8) + 4
                    "Venda", // E(5,8) + 2
                    "Bossa Nova", // E(5,16) + 6
                    "Tuareg - Bendir", // E(7,8)
                    "Bell - Ashanti", // E(7,12)
                    "Samba", // E(7,16) + 5
                    "West/Central Africa", // E(9,16) + 4
                    "Aka", // E(12,24)
                    "Aka 2", // E(13,24)
                };
                for (int i = 0; i < (int)LENGTHOF(polyLabels); i++)
                {
                    PattIndexItem *item = createMenuItem<PattIndexItem>(polyLabels[i], CHECKMARK(1 == 2));
                    item->module = module;
                    item->index = i;
                    menu->addChild(item);
                }
                return menu;
            }
        };

        menu->addChild(new MenuEntry);

        PattItem *polyItem = createMenuItem<PattItem>("Preset Pattern", ">");
        polyItem->module = module;
        menu->addChild(polyItem);
    }

};

Model *modelEuclidian = createModel<Euclidian, EuclidianWidget>("Euclidian");
