#include "RJModules.hpp"
#include "dsp/digital.hpp"
#include "UI.hpp"
#include <iostream>
#include <cmath>

struct GuitarSnapKnob : RoundSmallBlackKnob
{
    GuitarSnapKnob()
    {
        setSVG(SVG::load(assetPlugin(pluginInstance, "res/KTFRoundSmallBlackKnob.svg")));
        minAngle = -0.83 * M_PI;
        maxAngle = 0.83 * M_PI;
        snap = true;
    }
};

struct GuitarSnapKnobLg : RoundLargeBlackKnob
{
    GuitarSnapKnobLg()
    {
        setSVG(SVG::load(assetPlugin(pluginInstance, "res/KTFRoundLargeBlackKnob.svg")));
        minAngle = -0.83 * M_PI;
        maxAngle = 0.83 * M_PI;
        snap = true;
    }
};

struct GuitarLEDButton : SVGSwitch {
        GuitarLEDButton() {
                addFrame(SVG::load(assetPlugin(pluginInstance, "res/LilLEDButton.svg")));
                momentary = true;
        }
};

struct GuitarNeck: Module {
    enum ParamIds {
        ENUMS(FRET, 64),
        OCT_PARAM,
        ROOT_PARAM,
        RETURN_PARAM,
        HOLD_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        OCT_INPUT,
        ROOT_INPUT,
        NUM_INPUTS
    };
    enum OutputIds {
        NOTE_OUTPUT,
        GATE_OUTPUT,
        NUM_OUTPUTS
    };
    enum LightIds {
        ENUMS(LIGHT, 64),
        RETURN_LIGHT,
        HOLD_LIGHT,
        NUM_LIGHTS
    };
    
    SchmittTrigger returnTrigger;
    SchmittTrigger holdTrigger;
    bool RETURN = true;
    bool HOLD = false;

    int currentSquare;
    bool gateOpen;
    float lightValues[64];
    const float lightLambda = 0.06;

    GuitarNeck() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

        for (int i = 0; i < 64; i++) {
            configParam(GuitarNeck::FRET, 0.0, 1.0, 0.0, "");
            configParam(GuitarNeck::LIGHT, 0.0, 1.0, 0.0, "");
        }

        configParam(GuitarNeck::OCT_PARAM, 0.0, 6.0, 1.0, string::f("Octave", 0));
        configParam(GuitarNeck::ROOT_PARAM, 0.0, 11.0, 0.0, string::f("Root", 0));

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


    // Buttons
    if (holdTrigger.process(params[HOLD_PARAM].value)){
        HOLD = !HOLD;
    }
    if (returnTrigger.process(params[RETURN_PARAM].value)){
        RETURN = !RETURN;
    }
    if(HOLD){
        lights[HOLD_LIGHT].value = 10.0;
    }
    else{
        lights[HOLD_LIGHT].value = -10.0;
    }
    if(RETURN){
        lights[RETURN_LIGHT].value = 10.0;
    }
    else{
        lights[RETURN_LIGHT].value = -10.0;
    }
    
    //
    for(int i=0; i<64; i++){
        if(paramQuantities[ParamIds::FRET + i]->getValue() > 0){
            lightValues[i] = 1.0;
        }
        lights[LIGHT+i].value = lightValues[i];
        if(lightValues[LIGHT+i] > 0){
            lightValues[LIGHT+i] -= lightValues[LIGHT+i] / lightLambda / engineGetSampleRate();
        }
    }

    // 

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

    // Knobs
    addParam(createParam<GuitarSnapKnobLg>(Vec(368, 40), module, GuitarNeck::ROOT_PARAM));
    addParam(createParam<GuitarSnapKnobLg>(Vec(368, 120), module, GuitarNeck::OCT_PARAM));

    // CV
    addInput(createPort<PJ301MPort>(Vec(375, 85), PortWidget::INPUT, module, GuitarNeck::ROOT_INPUT));
    addInput(createPort<PJ301MPort>(Vec(375, 165), PortWidget::INPUT, module, GuitarNeck::OCT_INPUT));


    // Buttons
    addParam(createParam<GuitarLEDButton>(Vec(378, 210), module, GuitarNeck::RETURN_PARAM));
    addChild(createLight<MediumLight<GreenLight>>(Vec(378+4.4, 210+4.4), module, GuitarNeck::RETURN_LIGHT));
    
    addParam(createParam<GuitarLEDButton>(Vec(378, 240), module, GuitarNeck::HOLD_PARAM));
    addChild(createLight<MediumLight<GreenLight>>(Vec(378+4.4, 240+4.4), module, GuitarNeck::HOLD_LIGHT));

    // Outputs
    addOutput(createPort<PJ301MPort>(Vec(375, 280), PortWidget::OUTPUT, module, GuitarNeck::NOTE_OUTPUT));
    addOutput(createPort<PJ301MPort>(Vec(375, 330), PortWidget::OUTPUT, module, GuitarNeck::GATE_OUTPUT));

    // Pads
    int BASE_X = 37;
    int y = 32;
    int x = BASE_X;
    int dx = 1;
    Vec padSize = Vec(37, 37);
    Vec lSize = Vec(padSize.x - 2*dx, padSize.y - 2*dx);
    int spacing = padSize.x + 3;
    NVGcolor lightColor = nvgRGB(0xFF, 0xFF, 0xFF);
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
            pad->box.size = padSize;
            pad->momentary = false;
            pad->box.pos = Vec(x, y);
            pad->btnId = id;
            pad->groupId = groupId;
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

}

Model *modelGuitarNeck = createModel<GuitarNeck, GuitarNeckWidget>("GuitarNeck");
