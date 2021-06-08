#include "RJModules.hpp"
#include "dsp/digital.hpp"
#include "UI.hpp"
#include <iostream>
#include <cmath>

struct GuitarNeck: Module {
    enum ParamIds {
        ENUMS(FRET, 64),
        NUM_PARAMS
    };
    enum InputIds {
        NUM_INPUTS
    };
    enum OutputIds {
        CH1_OUTPUT,
        CH2_OUTPUT,
        CH3_OUTPUT,
        CH4_OUTPUT,
        CH5_OUTPUT,
        CH6_OUTPUT,
        CH7_OUTPUT,
        CH8_OUTPUT,
        CH9_OUTPUT,
        NUM_OUTPUTS
    };
    enum LightIds {
        ENUMS(LIGHT, 64),
        NUM_LIGHTS
    };
    float resetLight = 0.0;
    float resetLight2 = 0.0;
    float resetLight3 = 0.0;
    float resetLight4 = 0.0;
    float resetLight5 = 0.0;
    float resetLight6 = 0.0;
    float resetLight7 = 0.0;
    float resetLight8 = 0.0;
    float resetLight9 = 0.0;

    GuitarNeck() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

        for (int i = 0; i < 64; i++) {
            configParam(GuitarNeck::FRET, 0.0, 1.0, 0.0, "");
            configParam(GuitarNeck::LIGHT, 0.0, 1.0, 0.0, "");
        }

    }
    void step() override;
};

struct MedLEDButton : SVGSwitch {
        MedLEDButton() {
                addFrame(SVG::load(assetPlugin(pluginInstance, "res/MedLEDButton.svg")));
                momentary = true;
        }
};

template <typename BASE>
struct MedLight : BASE {
        MedLight() {
                this->box.size = mm2px(Vec(10, 10));
        }
};

void GuitarNeck::step() {

    // const float lightLambda = 0.075;
    // float output = 0.0;
    // float output2 = 0.0;
    // float output3 = 0.0;
    // float output4 = 0.0;
    // float output5 = 0.0;
    // float output6 = 0.0;
    // float output7 = 0.0;
    // float output8 = 0.0;
    // float output9 = 0.0;

    // // Reset
    // if (params[RESET_PARAM].value > 0) {
    //     resetLight = 1.0;
    //     output = 12.0;
    // }
    // if (params[RESET_PARAM2].value > 0) {
    //     resetLight2 = 1.0;
    //     output2 = 12.0;
    // }
    // if (params[RESET_PARAM3].value > 0) {
    //     resetLight3 = 1.0;
    //     output3 = 12.0;
    // }
    // if (params[RESET_PARAM4].value > 0) {
    //     resetLight4 = 1.0;
    //     output4 = 12.0;
    // }
    // if (params[RESET_PARAM5].value > 0) {
    //     resetLight5 = 1.0;
    //     output5 = 12.0;
    // }
    // if (params[RESET_PARAM6].value > 0) {
    //     resetLight6 = 1.0;
    //     output6 = 12.0;
    // }
    // if (params[RESET_PARAM7].value > 0) {
    //     resetLight7 = 1.0;
    //     output7 = 12.0;
    // }
    // if (params[RESET_PARAM8].value > 0) {
    //     resetLight8 = 1.0;
    //     output8 = 12.0;
    // }
    // if (params[RESET_PARAM9].value > 0) {
    //     resetLight9 = 1.0;
    //     output9 = 12.0;
    // }

    // resetLight -= resetLight / lightLambda / engineGetSampleRate();
    // resetLight2 -= resetLight2 / lightLambda / engineGetSampleRate();
    // resetLight3 -= resetLight3 / lightLambda / engineGetSampleRate();
    // resetLight4 -= resetLight4 / lightLambda / engineGetSampleRate();
    // resetLight5 -= resetLight5 / lightLambda / engineGetSampleRate();
    // resetLight6 -= resetLight6 / lightLambda / engineGetSampleRate();
    // resetLight7 -= resetLight7 / lightLambda / engineGetSampleRate();
    // resetLight8 -= resetLight8 / lightLambda / engineGetSampleRate();
    // resetLight9 -= resetLight9 / lightLambda / engineGetSampleRate();

    // outputs[CH1_OUTPUT].value = output;
    // outputs[CH2_OUTPUT].value = output2;
    // outputs[CH3_OUTPUT].value = output3;
    // outputs[CH4_OUTPUT].value = output4;
    // outputs[CH5_OUTPUT].value = output5;
    // outputs[CH6_OUTPUT].value = output6;
    // outputs[CH7_OUTPUT].value = output7;
    // outputs[CH8_OUTPUT].value = output8;
    // outputs[CH9_OUTPUT].value = output9;

    // lights[RESET_LIGHT].value = resetLight;
    // lights[RESET_LIGHT2].value = resetLight2;
    // lights[RESET_LIGHT3].value = resetLight3;
    // lights[RESET_LIGHT4].value = resetLight4;
    // lights[RESET_LIGHT5].value = resetLight5;
    // lights[RESET_LIGHT6].value = resetLight6;
    // lights[RESET_LIGHT7].value = resetLight7;
    // lights[RESET_LIGHT8].value = resetLight8;
    // lights[RESET_LIGHT9].value = resetLight9;

}


struct GuitarNeckWidget: ModuleWidget {
    GuitarNeckWidget(GuitarNeck *module);
};

GuitarNeckWidget::GuitarNeckWidget(GuitarNeck *module) {
    setModule(module);
    box.size = Vec(420, 380);

    {
        SVGPanel *panel = new SVGPanel();
        panel->box.size = box.size;
        panel->setBackground(SVG::load(assetPlugin(pluginInstance, "res/GuitarNeck.svg")));
        addChild(panel);
    }

    int BASE_X = 37;
    int y = 32;
    int x = BASE_X;
    int dx = 1;
    Vec padSize = Vec(37, 37);
    Vec lSize = Vec(padSize.x - 2*dx, padSize.y - 2*dx);
    int spacing = padSize.x + 3;
    NVGcolor lightColor = nvgRGB(0x00, 0xFF, 0x00);
    int numCols = 8;
    int numRows = 8;
    int groupId = 0;

    // if (!isPreview)
    // {
    //     numCols = seqModule->numCols;
    //     numRows = seqModule->numRows;
    //     lightColor = seqModule->voiceColors[seqModule->currentChannelEditingIx];
    //     groupId = seqModule->oscId; // Use this id for now since this is unique to each module instance.
    // }

    int id = 0;
    for (int r = 0; r < numRows; r++) //---------THE PADS
    {
        for (int c = 0; c < numCols; c++)
        {           

            std::cout << "Loading Pad!\n";

            // Pad buttons:
            RJ_PadSquare* pad = dynamic_cast<RJ_PadSquare*>(createParam<RJ_PadSquare>(Vec(x, y), module, GuitarNeck::FRET + id));
            // std::cout << pad << "pad Pad!\n";
            pad->box.size = padSize;
            pad->momentary = false;
            pad->box.pos = Vec(x, y);
            pad->btnId = id;
            pad->groupId = groupId;
            // pad->hide();
            if (pad->paramQuantity)
            {
                pad->paramQuantity->minValue = 0;
                pad->paramQuantity->maxValue = 1;
                pad->paramQuantity->defaultValue = 0;
                pad->paramQuantity->setValue(0);
            
            }
            addParam(pad);

            // Lights:
            RJ_LightSquare* padLight = dynamic_cast<RJ_LightSquare*>(RJ_createColorValueLight<RJ_LightSquare>(/*pos */ Vec(x + dx, y + dx),
                /*seqModule*/ module,
                /*lightId*/ GuitarNeck::LIGHT + id, // r * numCols + c
                /* size */ lSize, 
                /* color */ lightColor));
            addChild(padLight);

            // if (!isPreview)
            // {
            //     // Keep a reference to our pad lights so we can change the colors
            //     seqModule->padLightPtrs[r][c] = padLight;
            // }           
            id++;
            x+= spacing;
        }       
        y += spacing; // Next row
        x = BASE_X;
    } // end loop through NxN grid




    // addOutput(createPort<PJ301MPort>(Vec(24, 223), PortWidget::OUTPUT, module, GuitarNeck::CH1_OUTPUT));
    // addOutput(createPort<PJ301MPort>(Vec(65, 223), PortWidget::OUTPUT, module, GuitarNeck::CH2_OUTPUT));
    // addOutput(createPort<PJ301MPort>(Vec(105, 223), PortWidget::OUTPUT, module, GuitarNeck::CH3_OUTPUT));


    // addOutput(createPort<PJ301MPort>(Vec(24, 274), PortWidget::OUTPUT, module, GuitarNeck::CH4_OUTPUT));
    // addOutput(createPort<PJ301MPort>(Vec(65, 274), PortWidget::OUTPUT, module, GuitarNeck::CH5_OUTPUT));
    // addOutput(createPort<PJ301MPort>(Vec(106, 274), PortWidget::OUTPUT, module, GuitarNeck::CH6_OUTPUT));


    // addOutput(createPort<PJ301MPort>(Vec(24, 324), PortWidget::OUTPUT, module, GuitarNeck::CH7_OUTPUT));
    // addOutput(createPort<PJ301MPort>(Vec(65, 324), PortWidget::OUTPUT, module, GuitarNeck::CH8_OUTPUT));
    // addOutput(createPort<PJ301MPort>(Vec(106, 324), PortWidget::OUTPUT, module, GuitarNeck::CH9_OUTPUT));

    // addParam(createParam<MedLEDButton>(Vec(15, 60), module, GuitarNeck::FRET + 0));
    // addChild(createLight<MedLight<GreenLight>>(Vec(20, 65), module, GuitarNeck::LIGHT + 0));

    // addParam(createParam<MedLEDButton>(Vec(55, 60), module, GuitarNeck::FRET + 8));
    // addChild(createLight<MedLight<GreenLight>>(Vec(60, 65), module, GuitarNeck::LIGHT + 16));

    // addParam(createParam<MedLEDButton>(Vec(95, 60), module, GuitarNeck::FRET + 16));
    // addChild(createLight<MedLight<GreenLight>>(Vec(100, 65), module, GuitarNeck::LIGHT + 16));


    // addParam(createParam<MedLEDButton>(Vec(15, 100), module, GuitarNeck::RESET_PARAM4));
    // addChild(createLight<MedLight<GreenLight>>(Vec(20, 105), module, GuitarNeck::RESET_LIGHT4));

    // addParam(createParam<MedLEDButton>(Vec(55, 100), module, GuitarNeck::RESET_PARAM5));
    // addChild(createLight<MedLight<GreenLight>>(Vec(60, 105), module, GuitarNeck::RESET_LIGHT5));

    // addParam(createParam<MedLEDButton>(Vec(95, 100), module, GuitarNeck::RESET_PARAM6));
    // addChild(createLight<MedLight<GreenLight>>(Vec(100, 105), module, GuitarNeck::RESET_LIGHT6));

    // addParam(createParam<MedLEDButton>(Vec(15, 140), module, GuitarNeck::RESET_PARAM7));
    // addChild(createLight<MedLight<GreenLight>>(Vec(20, 145), module, GuitarNeck::RESET_LIGHT7));

    // addParam(createParam<MedLEDButton>(Vec(55, 140), module, GuitarNeck::RESET_PARAM8));
    // addChild(createLight<MedLight<GreenLight>>(Vec(60, 145), module, GuitarNeck::RESET_LIGHT8));

    // addParam(createParam<MedLEDButton>(Vec(95, 140), module, GuitarNeck::RESET_PARAM9));
    // addChild(createLight<MedLight<GreenLight>>(Vec(100, 145), module, GuitarNeck::RESET_LIGHT9));


}

Model *modelGuitarNeck = createModel<GuitarNeck, GuitarNeckWidget>("GuitarNeck");
