/*
Succccccccccccccc

*/

#include "RJModules.hpp"

/*
    UI
*/

// struct SucculentRoundLargeBlackKnob : RoundLargeBlackKnob {
//     SucculentRoundLargeBlackKnob() {
//         setSVG(SVG::load(assetPlugin(pluginInstance, "res/SucculentRoundLargeBlackKnob.svg")));
//     }
// };

// struct SucculentRoundLargeHappyKnob : RoundLargeBlackKnob {
//     SucculentRoundLargeHappyKnob() {
//         setSVG(SVG::load(assetPlugin(pluginInstance, "res/SucculentRoundLargeHappyKnob.svg")));
//     }
// };

// struct SucculentRoundLargeBlackSnapKnob : SucculentRoundLargeBlackKnob
// {
//     SucculentRoundLargeBlackSnapKnob()
//     {
//         minAngle = -0.83 * M_PI;
//         maxAngle = 0.83 * M_PI;
//         snap = true;
//     }
// };

/*
    Modules
*/
struct Succulent : Module {
    enum ParamIds {
        NUM_PARAMS
    };
    enum InputIds {
        NUM_INPUTS
    };
    enum OutputIds {
        NUM_OUTPUTS
    };
    enum LightIds {
        NUM_LIGHTS
    };

    Succulent() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
    }

    void step() override {

    }
};

struct SvgPanelNoBg : SvgPanel{
    void setBackground(std::shared_ptr<Svg> svg) {
        widget::SvgWidget* sw = new widget::SvgWidget;
        sw->setSvg(svg);
        addChild(sw);

        // Set size
        box.size = sw->box.size.div(RACK_GRID_SIZE).round().mult(RACK_GRID_SIZE);
    }
};

struct SucculentWidget : ModuleWidget {
    SucculentWidget(Succulent *module) {
		setModule(module);
        setPanel(SVG::load(assetPlugin(pluginInstance, "res/Succulent.svg")));
    }

    void setPanel(std::shared_ptr<Svg> svg) {
        // Remove existing panel
        if (panel) {
            removeChild(panel);
            delete panel;
            panel = NULL;
        }

        // Create SvgPanel
        SvgPanelNoBg* svgPanel = new SvgPanelNoBg;
        svgPanel->setBackground(svg);
        panel = svgPanel;
        addChildBottom(panel);

        // Set ModuleWidget size based on panel
        box.size.x = std::round(panel->box.size.x / RACK_GRID_WIDTH) * RACK_GRID_WIDTH;
    }

    void drawShadow(const DrawArgs& args) {
        return;
    }
};

Model *modelSucculent = createModel<Succulent, SucculentWidget>("Succulent");