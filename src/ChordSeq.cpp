#include "RJModules.hpp"
#include "dsp/digital.hpp"
#include <iostream>
#include <cmath>
#include <sstream>
#include <iomanip>

// Displays
struct SmallStringDisplayWidget : TransparentWidget {

  std::string *value;
  std::shared_ptr<Font> font;

  SmallStringDisplayWidget() {
    font = Font::load(assetPlugin(plugin, "res/Pokemon.ttf"));
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
    nvgFontSize(vg, 13);
    nvgFontFaceId(vg, font->handle);
    nvgTextLetterSpacing(vg, 2);

    std::stringstream to_display;
    to_display << std::setw(3) << *value;

    Vec textPos = Vec(4.0f, 20.0f);
    NVGcolor textColor = nvgRGB(0x00, 0x00, 0x00);
    nvgFillColor(vg, textColor);
    nvgText(vg, textPos.x, textPos.y, to_display.str().c_str(), NULL);
  }
};

struct TinySnapKnob : RoundSmallBlackKnob
{
    TinySnapKnob()
    {
        minAngle = -0.83 * M_PI;
        maxAngle = 0.83 * M_PI;
        snap = true;
    }
};

struct ChordSeq : Module {
	enum ParamIds {
		CLOCK_PARAM,
		RUN_PARAM,
		RESET_PARAM,
		STEPS_PARAM,
		ENUMS(ROW1_PARAM, 8),
		ENUMS(ROW2_PARAM, 8),
		ENUMS(ROW3_PARAM, 8),
		ENUMS(GATE_PARAM, 8),
		NUM_PARAMS
	};
	enum InputIds {
		CLOCK_INPUT,
		EXT_CLOCK_INPUT,
		RESET_INPUT,
		STEPS_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		GATES_OUTPUT,
		ROW1_OUTPUT,
		ROW2_OUTPUT,
		ROW3_OUTPUT,
		ENUMS(GATE_OUTPUT, 8),
		NUM_OUTPUTS
	};
	enum LightIds {
		RUNNING_LIGHT,
		RESET_LIGHT,
		GATES_LIGHT,
		ENUMS(ROW_LIGHTS, 3),
		ENUMS(GATE_LIGHTS, 8),
		NUM_LIGHTS
	};

	bool running = true;
	SchmittTrigger clockTrigger;
	SchmittTrigger runningTrigger;
	SchmittTrigger resetTrigger;
	SchmittTrigger gateTriggers[8];
	/** Phase of internal LFO */
	float phase = 0.f;
	int index = 0;
	bool gates[8] = {};
	std::string chord_values[8] = {"A#4M", "A#4M", "A#4M", "A#4M", "A#4M", "A#4M", "A#4M", "A#4M"};

	ChordSeq() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {
		onReset();
	}

	void onReset() override {
		for (int i = 0; i < 8; i++) {
			gates[i] = true;
		}
	}

	void onRandomize() override {
		for (int i = 0; i < 8; i++) {
			gates[i] = (randomUniform() > 0.5f);
		}
	}

	json_t *toJson() override {
		json_t *rootJ = json_object();

		// running
		json_object_set_new(rootJ, "running", json_boolean(running));

		// gates
		json_t *gatesJ = json_array();
		for (int i = 0; i < 8; i++) {
			json_array_insert_new(gatesJ, i, json_integer((int) gates[i]));
		}
		json_object_set_new(rootJ, "gates", gatesJ);

		return rootJ;
	}

	void fromJson(json_t *rootJ) override {
		// running
		json_t *runningJ = json_object_get(rootJ, "running");
		if (runningJ)
			running = json_is_true(runningJ);

		// gates
		json_t *gatesJ = json_object_get(rootJ, "gates");
		if (gatesJ) {
			for (int i = 0; i < 8; i++) {
				json_t *gateJ = json_array_get(gatesJ, i);
				if (gateJ)
					gates[i] = !!json_integer_value(gateJ);
			}
		}
	}

	void setIndex(int index) {
		int numSteps = (int) clamp(roundf(params[STEPS_PARAM].value + inputs[STEPS_INPUT].value), 1.0f, 8.0f);
		phase = 0.f;
		this->index = index;
		if (this->index >= numSteps)
			this->index = 0;
	}

	void step() override {
		// Run
		if (runningTrigger.process(params[RUN_PARAM].value)) {
			running = !running;
		}

		bool gateIn = false;
		if (running) {
			if (inputs[EXT_CLOCK_INPUT].active) {
				// External clock
				if (clockTrigger.process(inputs[EXT_CLOCK_INPUT].value)) {
					setIndex(index + 1);
				}
				gateIn = clockTrigger.isHigh();
			}
			else {
				// Internal clock
				float clockTime = powf(2.0f, params[CLOCK_PARAM].value + inputs[CLOCK_INPUT].value);
				phase += clockTime * engineGetSampleTime();
				if (phase >= 1.0f) {
					setIndex(index + 1);
				}
				gateIn = (phase < 0.5f);
			}
		}

		// Reset
		if (resetTrigger.process(params[RESET_PARAM].value + inputs[RESET_INPUT].value)) {
			setIndex(0);
		}

		// Gate buttons
		for (int i = 0; i < 8; i++) {
			if (gateTriggers[i].process(params[GATE_PARAM + i].value)) {
				gates[i] = !gates[i];
			}
			outputs[GATE_OUTPUT + i].value = (running && gateIn && i == index && gates[i]) ? 10.0f : 0.0f;
			lights[GATE_LIGHTS + i].setBrightnessSmooth((gateIn && i == index) ? (gates[i] ? 1.f : 0.33) : (gates[i] ? 0.66 : 0.0));
		}

		// Outputs
		outputs[ROW1_OUTPUT].value = params[ROW1_PARAM + index].value;
		outputs[ROW2_OUTPUT].value = params[ROW2_PARAM + index].value;
		outputs[ROW3_OUTPUT].value = params[ROW3_PARAM + index].value;
		outputs[GATES_OUTPUT].value = (gateIn && gates[index]) ? 10.0f : 0.0f;
		lights[RUNNING_LIGHT].value = (running);
		lights[RESET_LIGHT].setBrightnessSmooth(resetTrigger.isHigh());
		lights[GATES_LIGHT].setBrightnessSmooth(gateIn);
		lights[ROW_LIGHTS].value = outputs[ROW1_OUTPUT].value / 10.0f;
		lights[ROW_LIGHTS + 1].value = outputs[ROW2_OUTPUT].value / 10.0f;
		lights[ROW_LIGHTS + 2].value = outputs[ROW3_OUTPUT].value / 10.0f;
	}
};


struct ChordSeqWidget : ModuleWidget {
	ChordSeqWidget(ChordSeq *module) : ModuleWidget(module) {
		setPanel(SVG::load(assetPlugin(plugin, "res/ChordSeq.svg")));

		addChild(Widget::create<ScrewSilver>(Vec(15, 0)));
		addChild(Widget::create<ScrewSilver>(Vec(box.size.x-30, 0)));
		addChild(Widget::create<ScrewSilver>(Vec(15, 365)));
		addChild(Widget::create<ScrewSilver>(Vec(box.size.x-30, 365)));

		addParam(ParamWidget::create<RoundBlackKnob>(Vec(18, 56), module, ChordSeq::CLOCK_PARAM, -2.0f, 6.0f, 2.0f));
		addParam(ParamWidget::create<LEDButton>(Vec(60, 61-1), module, ChordSeq::RUN_PARAM, 0.0f, 1.0f, 0.0f));
		addChild(ModuleLightWidget::create<MediumLight<GreenLight>>(Vec(64.4f, 64.4f), module, ChordSeq::RUNNING_LIGHT));
		addParam(ParamWidget::create<LEDButton>(Vec(99, 61-1), module, ChordSeq::RESET_PARAM, 0.0f, 1.0f, 0.0f));
		addChild(ModuleLightWidget::create<MediumLight<GreenLight>>(Vec(103.4f, 64.4f), module, ChordSeq::RESET_LIGHT));
		addParam(ParamWidget::create<RoundBlackSnapKnob>(Vec(132, 56), module, ChordSeq::STEPS_PARAM, 1.0f, 8.0f, 8.0f));
		addChild(ModuleLightWidget::create<MediumLight<GreenLight>>(Vec(179.4f, 64.4f), module, ChordSeq::GATES_LIGHT));
		addChild(ModuleLightWidget::create<MediumLight<GreenLight>>(Vec(218.4f, 64.4f), module, ChordSeq::ROW_LIGHTS));
		addChild(ModuleLightWidget::create<MediumLight<GreenLight>>(Vec(256.4f, 64.4f), module, ChordSeq::ROW_LIGHTS + 1));
		addChild(ModuleLightWidget::create<MediumLight<GreenLight>>(Vec(295.4f, 64.4f), module, ChordSeq::ROW_LIGHTS + 2));

		static const float portX[8] = {20, 58, 96, 135, 173, 212, 250, 289};
		addInput(Port::create<PJ301MPort>(Vec(portX[0]-1, 98), Port::INPUT, module, ChordSeq::CLOCK_INPUT));
		addInput(Port::create<PJ301MPort>(Vec(portX[1]-1, 98), Port::INPUT, module, ChordSeq::EXT_CLOCK_INPUT));
		addInput(Port::create<PJ301MPort>(Vec(portX[2]-1, 98), Port::INPUT, module, ChordSeq::RESET_INPUT));
		addInput(Port::create<PJ301MPort>(Vec(portX[3]-1, 98), Port::INPUT, module, ChordSeq::STEPS_INPUT));
		addOutput(Port::create<PJ301MPort>(Vec(portX[4]-1, 98), Port::OUTPUT, module, ChordSeq::GATES_OUTPUT));
		addOutput(Port::create<PJ301MPort>(Vec(portX[5]-1, 98), Port::OUTPUT, module, ChordSeq::ROW1_OUTPUT));
		addOutput(Port::create<PJ301MPort>(Vec(portX[6]-1, 98), Port::OUTPUT, module, ChordSeq::ROW2_OUTPUT));
		addOutput(Port::create<PJ301MPort>(Vec(portX[7]-1, 98), Port::OUTPUT, module, ChordSeq::ROW3_OUTPUT));

		SmallStringDisplayWidget *display = new SmallStringDisplayWidget();
		for (int i = 0; i < 8; i++) {
			//addParam(ParamWidget::create<RoundBlackKnob>(Vec(portX[i]-2, 157), module, ChordSeq::ROW1_PARAM + i, 0.0f, 10.0f, 0.0f));
			
		    display = new SmallStringDisplayWidget();
		    display->box.pos = Vec(portX[i]-10, 157);
		    display->box.size = Vec(45, 30);
		    display->value = &module->chord_values[i];
		    addChild(display);

			addParam(ParamWidget::create<RoundBlackKnob>(Vec(portX[i]-2, 198), module, ChordSeq::ROW2_PARAM + i, 0.0f, 10.0f, 0.0f));
			addParam(ParamWidget::create<RoundBlackKnob>(Vec(portX[i]-2, 240), module, ChordSeq::ROW3_PARAM + i, 0.0f, 10.0f, 0.0f));
			addParam(ParamWidget::create<LEDButton>(Vec(portX[i]+2, 278-1), module, ChordSeq::GATE_PARAM + i, 0.0f, 1.0f, 0.0f));
			addChild(ModuleLightWidget::create<MediumLight<GreenLight>>(Vec(portX[i]+6.4f, 281.4f), module, ChordSeq::GATE_LIGHTS + i));
			addOutput(Port::create<PJ301MPort>(Vec(portX[i]-1, 307), Port::OUTPUT, module, ChordSeq::GATE_OUTPUT + i));
		}
	}
};



Model *modelChordSeq = Model::create<ChordSeq, ChordSeqWidget>("RJModules", "ChordSeq", "[SEQ] ChordSeq - Chord Sequencer", SEQUENCER_TAG);
