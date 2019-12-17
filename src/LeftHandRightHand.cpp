#include "plugin.hpp"
#include <algorithm>
#include "RJModules.hpp"
#include "common.hpp"
#include "dsp/digital.hpp"
#include <iostream>
#include <cmath>
#include <sstream>
#include <iomanip>
#include <unistd.h>

/*
Display
*/

struct LeftHandRightHandSmallStringDisplayWidget : TransparentWidget {

  std::string *value;
  std::shared_ptr<Font> font;

  LeftHandRightHandSmallStringDisplayWidget() {
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

struct LeftHandRightHandRoundLargeBlackKnob : RoundLargeBlackKnob
{
    LeftHandRightHandRoundLargeBlackKnob()
    {
        setSVG(SVG::load(assetPlugin(pluginInstance, "res/KTFRoundHugeBlackKnob.svg")));
    }
};

struct LeftHandRightHandRoundLargeBlackSnapKnob : RoundLargeBlackKnob
{
    LeftHandRightHandRoundLargeBlackSnapKnob()
    {
        setSVG(SVG::load(assetPlugin(pluginInstance, "res/KTFRoundLargeBlackKnob.svg")));
        minAngle = -0.83 * M_PI;
        maxAngle = 0.83 * M_PI;
        snap = true;
    }
};

struct LeftHandRightHandRoundBlackSnapKnob : RoundBlackKnob
{
    LeftHandRightHandRoundBlackSnapKnob()
    {
        setSVG(SVG::load(assetPlugin(pluginInstance, "res/KTFRoundLargeBlackKnob.svg")));
        minAngle = -0.83 * M_PI;
        maxAngle = 0.83 * M_PI;
        snap = true;
    }
};

struct LeftHandRightHand : Module {
    enum ParamIds {
        SPLIT_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        NUM_INPUTS
    };
    enum OutputIds {

        CV_OUTPUT,
        GATE_OUTPUT,
        VELOCITY_OUTPUT,
        AFTERTOUCH_OUTPUT,
        PITCH_OUTPUT,
        MOD_OUTPUT,
        RETRIGGER_OUTPUT,
        CLOCK_OUTPUT,
        CLOCK_DIV_OUTPUT,
        START_OUTPUT,
        STOP_OUTPUT,
        CONTINUE_OUTPUT,

        CV_OUTPUT_2,
        GATE_OUTPUT_2,
        VELOCITY_OUTPUT_2,
        AFTERTOUCH_OUTPUT_2,
        PITCH_OUTPUT_2,
        MOD_OUTPUT_2,
        RETRIGGER_OUTPUT_2,
        CLOCK_OUTPUT_2,
        CLOCK_DIV_OUTPUT_2,
        START_OUTPUT_2,
        STOP_OUTPUT_2,
        CONTINUE_OUTPUT_2,

        NUM_OUTPUTS
    };
    enum LightIds {
        NUM_LIGHTS
    };

    midi::InputQueue midiInput;

    int channels;
    enum PolyMode {
        ROTATE_MODE,
        REUSE_MODE,
        RESET_MODE,
        MPE_MODE,
        NUM_POLY_MODES
    };
    PolyMode polyMode;

    uint32_t clock = 0;
    int clockDivision;

    bool pedal;
    // Indexed by channel
    uint8_t notes[16];
    bool gates[16];
    uint8_t velocities[16];
    uint8_t aftertouches[16];
    std::vector<uint8_t> heldNotes;
    bool leftHandMessage = false;
    bool rightHandMessage = false;

    int rotateIndex;

    // 16 channels for MPE. When MPE is disabled, only the first channel is used.
    uint16_t pitches[16];
    uint8_t mods[16];
    dsp::ExponentialFilter pitchFilters[16];
    dsp::ExponentialFilter modFilters[16];

    dsp::PulseGenerator clockPulse;
    dsp::PulseGenerator clockDividerPulse;
    dsp::PulseGenerator retriggerPulses[16];
    dsp::PulseGenerator startPulse;
    dsp::PulseGenerator stopPulse;
    dsp::PulseGenerator continuePulse;

    // Display
    std::string k_display = "60";

    LeftHandRightHand() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(LeftHandRightHand::SPLIT_PARAM, 0, 127, 60, "Split Note");
        heldNotes.reserve(128);
        for (int c = 0; c < 16; c++) {
            pitchFilters[c].setTau(1 / 30.f);
            modFilters[c].setTau(1 / 30.f);
        }
        onReset();
    }

    void onReset() override {
        channels = 1;
        polyMode = ROTATE_MODE;
        clockDivision = 24;
        panic();
        midiInput.reset();
    }

    /** Resets performance state */
    void panic() {
        pedal = false;
        for (int c = 0; c < 16; c++) {
            notes[c] = 60;
            gates[c] = false;
            velocities[c] = 0;
            aftertouches[c] = 0;
            pitches[c] = 8192;
            mods[c] = 0;
            pitchFilters[c].reset();
            modFilters[c].reset();
        }
        pedal = false;
        rotateIndex = -1;
        heldNotes.clear();
    }

    void process(const ProcessArgs& args) override {
        midi::Message msg;
        while (midiInput.shift(&msg)) {
            processMessage(msg);
        }

        outputs[CV_OUTPUT].setChannels(channels);
        outputs[GATE_OUTPUT].setChannels(channels);
        outputs[VELOCITY_OUTPUT].setChannels(channels);

        outputs[CV_OUTPUT_2].setChannels(channels);
        outputs[GATE_OUTPUT_2].setChannels(channels);
        outputs[VELOCITY_OUTPUT_2].setChannels(channels);

        outputs[AFTERTOUCH_OUTPUT].setChannels(channels);
        outputs[RETRIGGER_OUTPUT].setChannels(channels);

        int split = (int)params[SPLIT_PARAM].getValue();
        k_display = std::to_string(split);

        if(leftHandMessage){
            for (int c = 0; c < channels; c++) {
                outputs[CV_OUTPUT].setVoltage((notes[c] - 60.f) / 12.f, c);
                outputs[GATE_OUTPUT].setVoltage(gates[c] ? 10.f : 0.f, c);
                outputs[VELOCITY_OUTPUT].setVoltage(rescale(velocities[c], 0, 127, 0.f, 10.f), c);
                outputs[RETRIGGER_OUTPUT].setVoltage(retriggerPulses[c].process(args.sampleTime) ? 10.f : 0.f, c);
            }
        }
        else{
            for (int c = 0; c < channels; c++) {
                outputs[CV_OUTPUT_2].setVoltage((notes[c] - 60.f) / 12.f, c);
                outputs[GATE_OUTPUT_2].setVoltage(gates[c] ? 10.f : 0.f, c);
                outputs[VELOCITY_OUTPUT_2].setVoltage(rescale(velocities[c], 0, 127, 0.f, 10.f), c);
                outputs[RETRIGGER_OUTPUT_2].setVoltage(retriggerPulses[c].process(args.sampleTime) ? 10.f : 0.f, c);
            }
        }
    }

    void processMessage(midi::Message msg) {
        // DEBUG("MIDI: %01x %01x %02x %02x", msg.getStatus(), msg.getChannel(), msg.getNote(), msg.getValue());
        // DEBUG("NOTE: %03x", msg.getNote());

        if(msg.getNote() < 60){
            leftHandMessage = true;
            rightHandMessage = false;
        }
        else{
            leftHandMessage = false;
            rightHandMessage = true;
        }

        switch (msg.getStatus()) {
            // note off
            case 0x8: {
                releaseNote(msg.getNote());
            } break;
            // note on
            case 0x9: {
                if (msg.getValue() > 0) {
                    int c = msg.getChannel();
                    pressNote(msg.getNote(), &c);
                    velocities[c] = msg.getValue();
                }
                else {
                    // For some reason, some keyboards send a "note on" event with a velocity of 0 to signal that the key has been released.
                    releaseNote(msg.getNote());
                }
            } break;
            // key pressure
            case 0xa: {
                // Set the aftertouches with the same note
                // TODO Should we handle the MPE case differently?
                for (int c = 0; c < 16; c++) {
                    if (notes[c] == msg.getNote())
                        aftertouches[c] = msg.getValue();
                }
            } break;
            // cc
            case 0xb: {
                processCC(msg);
            } break;
            // channel pressure
            case 0xd: {
                if (polyMode == MPE_MODE) {
                    // Set the channel aftertouch
                    aftertouches[msg.getChannel()] = msg.getNote();
                }
                else {
                    // Set all aftertouches
                    for (int c = 0; c < 16; c++) {
                        aftertouches[c] = msg.getNote();
                    }
                }
            } break;
            // pitch wheel
            case 0xe: {
                int c = (polyMode == MPE_MODE) ? msg.getChannel() : 0;
                pitches[c] = ((uint16_t) msg.getValue() << 7) | msg.getNote();
            } break;
            case 0xf: {
                processSystem(msg);
            } break;
            default: break;
        }
    }

    void processCC(midi::Message msg) {
        switch (msg.getNote()) {
            // mod
            case 0x01: {
                int c = (polyMode == MPE_MODE) ? msg.getChannel() : 0;
                mods[c] = msg.getValue();
            } break;
            // sustain
            case 0x40: {
                if (msg.getValue() >= 64)
                    pressPedal();
                else
                    releasePedal();
            } break;
            default: break;
        }
    }

    void processSystem(midi::Message msg) {
        switch (msg.getChannel()) {
            // Timing
            case 0x8: {
                clockPulse.trigger(1e-3);
                if (clock % clockDivision == 0) {
                    clockDividerPulse.trigger(1e-3);
                }
                clock++;
            } break;
            // Start
            case 0xa: {
                startPulse.trigger(1e-3);
                clock = 0;
            } break;
            // Continue
            case 0xb: {
                continuePulse.trigger(1e-3);
            } break;
            // Stop
            case 0xc: {
                stopPulse.trigger(1e-3);
                clock = 0;
            } break;
            default: break;
        }
    }

    int assignChannel(uint8_t note) {
        if (channels == 1)
            return 0;

        switch (polyMode) {
            case REUSE_MODE: {
                // Find channel with the same note
                for (int c = 0; c < channels; c++) {
                    if (notes[c] == note)
                        return c;
                }
            } // fallthrough

            case ROTATE_MODE: {
                // Find next available channel
                for (int i = 0; i < channels; i++) {
                    rotateIndex++;
                    if (rotateIndex >= channels)
                        rotateIndex = 0;
                    if (!gates[rotateIndex])
                        return rotateIndex;
                }
                // No notes are available. Advance rotateIndex once more.
                rotateIndex++;
                if (rotateIndex >= channels)
                    rotateIndex = 0;
                return rotateIndex;
            } break;

            case RESET_MODE: {
                for (int c = 0; c < channels; c++) {
                    if (!gates[c])
                        return c;
                }
                return channels - 1;
            } break;

            case MPE_MODE: {
                // This case is handled by querying the MIDI message channel.
                return 0;
            } break;

            default: return 0;
        }
    }

    void pressNote(uint8_t note, int* channel) {
        // Remove existing similar note
        auto it = std::find(heldNotes.begin(), heldNotes.end(), note);
        if (it != heldNotes.end())
            heldNotes.erase(it);
        // Push note
        heldNotes.push_back(note);
        // Determine actual channel
        if (polyMode == MPE_MODE) {
            // Channel is already decided for us
        }
        else {
            *channel = assignChannel(note);
        }
        // Set note
        notes[*channel] = note;
        gates[*channel] = true;
        retriggerPulses[*channel].trigger(1e-3);
    }

    void releaseNote(uint8_t note) {
        // Remove the note
        auto it = std::find(heldNotes.begin(), heldNotes.end(), note);
        if (it != heldNotes.end())
            heldNotes.erase(it);
        // Hold note if pedal is pressed
        if (pedal)
            return;
        // Turn off gate of all channels with note
        for (int c = 0; c < channels; c++) {
            if (notes[c] == note) {
                gates[c] = false;
            }
        }
        // Set last note if monophonic
        if (channels == 1) {
            if (note == notes[0] && !heldNotes.empty()) {
                uint8_t lastNote = heldNotes.back();
                notes[0] = lastNote;
                gates[0] = true;
                return;
            }
        }
    }

    void pressPedal() {
        if (pedal)
            return;
        pedal = true;
    }

    void releasePedal() {
        if (!pedal)
            return;
        pedal = false;
        // Set last note if monophonic
        if (channels == 1) {
            if (!heldNotes.empty()) {
                uint8_t lastNote = heldNotes.back();
                notes[0] = lastNote;
            }
        }
        // Clear notes that are not held if polyphonic
        else {
            for (int c = 0; c < channels; c++) {
                if (!gates[c])
                    continue;
                gates[c] = false;
                for (uint8_t note : heldNotes) {
                    if (notes[c] == note) {
                        gates[c] = true;
                        break;
                    }
                }
            }
        }
    }

    void setChannels(int channels) {
        if (channels == this->channels)
            return;
        this->channels = channels;
        panic();
    }

    void setPolyMode(PolyMode polyMode) {
        if (polyMode == this->polyMode)
            return;
        this->polyMode = polyMode;
        panic();
    }

    json_t* dataToJson() override {
        json_t* rootJ = json_object();
        json_object_set_new(rootJ, "channels", json_integer(channels));
        json_object_set_new(rootJ, "polyMode", json_integer(polyMode));
        json_object_set_new(rootJ, "clockDivision", json_integer(clockDivision));
        // Saving/restoring pitch and mod doesn't make much sense for MPE.
        if (polyMode != MPE_MODE) {
            json_object_set_new(rootJ, "lastPitch", json_integer(pitches[0]));
            json_object_set_new(rootJ, "lastMod", json_integer(mods[0]));
        }
        json_object_set_new(rootJ, "midi", midiInput.toJson());
        return rootJ;
    }

    void dataFromJson(json_t* rootJ) override {
        json_t* channelsJ = json_object_get(rootJ, "channels");
        if (channelsJ)
            setChannels(json_integer_value(channelsJ));

        json_t* polyModeJ = json_object_get(rootJ, "polyMode");
        if (polyModeJ)
            polyMode = (PolyMode) json_integer_value(polyModeJ);

        json_t* clockDivisionJ = json_object_get(rootJ, "clockDivision");
        if (clockDivisionJ)
            clockDivision = json_integer_value(clockDivisionJ);

        json_t* lastPitchJ = json_object_get(rootJ, "lastPitch");
        if (lastPitchJ)
            pitches[0] = json_integer_value(lastPitchJ);

        json_t* lastModJ = json_object_get(rootJ, "lastMod");
        if (lastModJ)
            mods[0] = json_integer_value(lastModJ);

        json_t* midiJ = json_object_get(rootJ, "midi");
        if (midiJ)
            midiInput.fromJson(midiJ);
    }
};


struct ClockDivisionValueItem : MenuItem {
    LeftHandRightHand* module;
    int clockDivision;
    void onAction(const event::Action& e) override {
        module->clockDivision = clockDivision;
    }
};


struct ClockDivisionItem : MenuItem {
    LeftHandRightHand* module;
    Menu* createChildMenu() override {
        Menu* menu = new Menu;
        std::vector<int> divisions = {24 * 4, 24 * 2, 24, 24 / 2, 24 / 4, 24 / 8, 2, 1};
        std::vector<std::string> divisionNames = {"Whole", "Half", "Quarter", "8th", "16th", "32nd", "12 PPQN", "24 PPQN"};
        for (size_t i = 0; i < divisions.size(); i++) {
            ClockDivisionValueItem* item = new ClockDivisionValueItem;
            item->text = divisionNames[i];
            item->rightText = CHECKMARK(module->clockDivision == divisions[i]);
            item->module = module;
            item->clockDivision = divisions[i];
            menu->addChild(item);
        }
        return menu;
    }
};


struct ChannelValueItem : MenuItem {
    LeftHandRightHand* module;
    int channels;
    void onAction(const event::Action& e) override {
        module->setChannels(channels);
    }
};


struct ChannelItem : MenuItem {
    LeftHandRightHand* module;
    Menu* createChildMenu() override {
        Menu* menu = new Menu;
        for (int channels = 1; channels <= 16; channels++) {
            ChannelValueItem* item = new ChannelValueItem;
            if (channels == 1)
                item->text = "Monophonic";
            else
                item->text = string::f("%d", channels);
            item->rightText = CHECKMARK(module->channels == channels);
            item->module = module;
            item->channels = channels;
            menu->addChild(item);
        }
        return menu;
    }
};


struct PolyModeValueItem : MenuItem {
    LeftHandRightHand* module;
    LeftHandRightHand::PolyMode polyMode;
    void onAction(const event::Action& e) override {
        module->setPolyMode(polyMode);
    }
};


struct PolyModeItem : MenuItem {
    LeftHandRightHand* module;
    Menu* createChildMenu() override {
        Menu* menu = new Menu;
        std::vector<std::string> polyModeNames = {
            "Rotate",
            "Reuse",
            "Reset",
            "MPE",
        };
        for (int i = 0; i < LeftHandRightHand::NUM_POLY_MODES; i++) {
            LeftHandRightHand::PolyMode polyMode = (LeftHandRightHand::PolyMode) i;
            PolyModeValueItem* item = new PolyModeValueItem;
            item->text = polyModeNames[i];
            item->rightText = CHECKMARK(module->polyMode == polyMode);
            item->module = module;
            item->polyMode = polyMode;
            menu->addChild(item);
        }
        return menu;
    }
};


struct LeftHandRightHandPanicItem : MenuItem {
    LeftHandRightHand* module;
    void onAction(const event::Action& e) override {
        module->panic();
    }
};


struct LeftHandRightHandWidget : ModuleWidget {
    LeftHandRightHandWidget(LeftHandRightHand* module) {
        setModule(module);

        box.size = Vec(15*8, 380);
        SVGPanel *panel = new SVGPanel();
        panel->box.size = box.size;
        panel->setBackground(SVG::load(assetPlugin(pluginInstance, "res/LeftHandRightHand.svg")));
        addChild(panel);

        // Displays
        if(module != NULL){
            LeftHandRightHandSmallStringDisplayWidget *k_Display = new LeftHandRightHandSmallStringDisplayWidget();
            k_Display->box.pos = Vec(20, 140);
            k_Display->box.size = Vec(35, 35);
            k_Display->value = &module->k_display;
            addChild(k_Display);
        }
        addParam(createParam<LeftHandRightHandRoundLargeBlackSnapKnob>(Vec(60, 140), module, LeftHandRightHand::SPLIT_PARAM));

        // addOutput(createOutput<PJ301MPort>(mm2px(Vec(4.61505, 60.1445)), module, LeftHandRightHand::CV_OUTPUT));
        // addOutput(createOutput<PJ301MPort>(mm2px(Vec(16.214, 60.1445)), module,  LeftHandRightHand::GATE_OUTPUT));
        // addOutput(createOutput<PJ301MPort>(mm2px(Vec(27.8143, 60.1445)), module, LeftHandRightHand::VELOCITY_OUTPUT));

        addOutput(createOutput<PJ301MPort>(mm2px(Vec(4.61505, 76.1449)), module, LeftHandRightHand::CV_OUTPUT));
        addOutput(createOutput<PJ301MPort>(mm2px(Vec(16.214, 76.1449)), module,  LeftHandRightHand::GATE_OUTPUT));
        addOutput(createOutput<PJ301MPort>(mm2px(Vec(16.214, 86.1449)), module, LeftHandRightHand::RETRIGGER_OUTPUT));
        addOutput(createOutput<PJ301MPort>(mm2px(Vec(27.8143, 76.1449)), module, LeftHandRightHand::VELOCITY_OUTPUT));

        // addOutput(createOutput<PJ301MPort>(mm2px(Vec(4.61505, 92.1439)), module, LeftHandRightHand::CLOCK_OUTPUT));
        // addOutput(createOutput<PJ301MPort>(mm2px(Vec(16.214, 92.1439)), module, LeftHandRightHand::CLOCK_DIV_OUTPUT));
        // addOutput(createOutput<PJ301MPort>(mm2px(Vec(27.8143, 92.1439)), module, LeftHandRightHand::RETRIGGER_OUTPUT));

        addOutput(createOutput<PJ301MPort>(mm2px(Vec(4.61505, 108.144)), module, LeftHandRightHand::CV_OUTPUT_2));
        addOutput(createOutput<PJ301MPort>(mm2px(Vec(16.214, 108.144)), module,  LeftHandRightHand::GATE_OUTPUT_2));
        addOutput(createOutput<PJ301MPort>(mm2px(Vec(16.214, 118.1449)), module, LeftHandRightHand::RETRIGGER_OUTPUT_2));
        addOutput(createOutput<PJ301MPort>(mm2px(Vec(27.8143, 108.144)), module, LeftHandRightHand::VELOCITY_OUTPUT_2));

        MidiWidget* midiWidget = createWidget<MidiWidget>(mm2px(Vec(3.41891, 14.8373)));
        midiWidget->box.size = mm2px(Vec(33.840, 28));
        midiWidget->setMidiPort(module ? &module->midiInput : NULL);
        addChild(midiWidget);
    }

    void appendContextMenu(Menu* menu) override {
        LeftHandRightHand* module = dynamic_cast<LeftHandRightHand*>(this->module);

        menu->addChild(new MenuEntry);

        ClockDivisionItem* clockDivisionItem = new ClockDivisionItem;
        clockDivisionItem->text = "CLK/N divider";
        clockDivisionItem->rightText = RIGHT_ARROW;
        clockDivisionItem->module = module;
        menu->addChild(clockDivisionItem);

        ChannelItem* channelItem = new ChannelItem;
        channelItem->text = "Polyphony channels";
        channelItem->rightText = string::f("%d", module->channels) + " " + RIGHT_ARROW;
        channelItem->module = module;
        menu->addChild(channelItem);

        PolyModeItem* polyModeItem = new PolyModeItem;
        polyModeItem->text = "Polyphony mode";
        polyModeItem->rightText = RIGHT_ARROW;
        polyModeItem->module = module;
        menu->addChild(polyModeItem);

        LeftHandRightHandPanicItem* panicItem = new LeftHandRightHandPanicItem;
        panicItem->text = "Panic";
        panicItem->module = module;
        menu->addChild(panicItem);
    }
};


// Use legacy slug for compatibility
Model* modelLeftHandRightHand = createModel<LeftHandRightHand, LeftHandRightHandWidget>("LeftHandRightHand");

