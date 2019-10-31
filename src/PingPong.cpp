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

struct PingPongSmallStringDisplayWidget : TransparentWidget {

  std::string *value;
  std::shared_ptr<Font> font;

  PingPongSmallStringDisplayWidget() {
    font = Font::load(assetPlugin(pluginInstance, "res/Pokemon.ttf"));
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
    nvgFontSize(vg, 20);
    nvgFontFaceId(vg, font->handle);
    nvgTextLetterSpacing(vg, 0.4);

    std::stringstream to_display;
    to_display << std::setw(3) << *value;

    Vec textPos = Vec(12.0f, 28.0f);
    NVGcolor textColor = nvgRGB(0x00, 0x00, 0x00);
    nvgFillColor(vg, textColor);
    nvgText(vg, textPos.x, textPos.y, to_display.str().c_str(), NULL);
  }
};

/*
Widget
*/

struct PingPong : Module {
    enum ParamIds {
        NUM_PARAMS
    };
    enum InputIds {
        CLOCK_INPUT,
        NUM_INPUTS
    };
    enum OutputIds {
        LEFT_OUTPUT,
        RIGHT_OUTPUT,
        NUM_OUTPUTS
    };
    enum LightIds {
        NUM_LIGHTS
    };

    // Display
    std::string rate_display = "1";

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

    /* Delays */
    dsp::RCFilter lowpassFilter;
    dsp::RCFilter highpassFilter;

    dsp::DoubleRingBuffer<float, HISTORY_SIZE> historyBuffer;
    dsp::DoubleRingBuffer<float, 16> outBuffer;

    dsp::SampleRateConverter<1> src;

    void calculateValues(float bpm){
        millisecondsPerBeat = millisecs/bpm;
        millisecondsPerMeasure = millisecondsPerBeat * 4;
        secondsPerBeat = 60 / bpm;
        secondsPerMeasure = secondsPerBeat * 4;
        bar = (millisecondsPerMeasure);
        half_note_d = ( millisecondsPerBeat * 3 );
        half_note = ( millisecondsPerBeat * 2 );
        half_note_t = ( half_note * 2 / 3 );
        qt_note_d = ( millisecondsPerBeat / 2 ) * 3;
        qt_note = millisecondsPerBeat;
        qt_note_t =  half_note  / 3;
        eight_note_d = ( millisecondsPerBeat / 4 ) * 3;
        eight_note = millisecondsPerBeat / 2;
        eight_note_t = millisecondsPerBeat / 3;
        sixth_note = millisecondsPerBeat / 4;
        sixth_note_d = ( sixth_note ) * 1.5;
        sixth_note_t = millisecondsPerBeat / 6;
        trth_note = millisecondsPerBeat / 8;
        trth_note_d = ( trth_note ) * 1.5;
        trth_note_t = trth_note * 2 / 3;
        last_bpm = bpm;
    }

    void refreshDetector() {
        inMemory = false;
        beatLock = false;
        beatTime = 0.0f;
        beatCount = 0;
        beatCountMemory = 0;
        beatOld = 0.0f;
        tempo = "---";
        pulse = false;
    }

    void onReset() override {
        refreshDetector();
    }

    void onInitialize() {
        refreshDetector();
    }

    PingPong() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

    }

    void process(const ProcessArgs &args) override {

    /* Clock Detector */
    float deltaTime = args.sampleTime;
    if ( inputs[CLOCK_INPUT].isConnected() ) {

        float clockInput = inputs[CLOCK_INPUT].getVoltage();
        //A rising slope
        if ( ( clockTrigger.process( inputs[CLOCK_INPUT].getVoltage() ) ) && !inMemory ) {
          beatCount++;
          if(!beatLock){
            LightPulse.trigger( 0.1f );
          }
          inMemory = true;

          //BPM is locked
          if ( beatCount == 2 ) {
            beatLock = true;
            beatOld = beatTime;
          }
          //BPM is lost
          if ( beatCount > 2 ) {
            if ( fabs( beatOld - beatTime ) > 0.0005f ) {
              beatLock = false;
              beatCount = 0;
              tempo = "---";
            }
          }
          beatTime = 0;
        }

        //Falling slope
        if ( clockInput <= 0 && inMemory ) {
            inMemory = false;
        }
        //When BPM is locked
        if ( beatLock ) {
          bpm = (int)round( 60 / beatOld );
          tempo = std::to_string( (int)round(bpm) );
          if(bpm!=last_bpm){
            if(bpm<999){
              calculateValues(bpm);
            }else{
              tempo = "OOR";
            }
          }
        } //end of beatLock routine

        beatTime += deltaTime;

      //when beat is lost
      if ( beatTime > 2 ) {
        beatLock = false;
        beatCount = 0;
        tempo = "---";
      }
      beatCountMemory = beatCount;

    } else {
      beatLock = false;
      beatCount = 0;
      //tempo = "OFF";
      //caluculate with knob value instead of bmp detector value
      bpm = 134;
      if (bpm<30){
        bpm = 30;
      }
      bpm = (int)round(bpm);
      tempo = std::to_string( (int)round(bpm) );
      if(bpm!=last_bpm){
          calculateValues(bpm);
      }
    }

    //OUTPUTS: MS to 10V scaled values
    // outputs[MS_OUTPUT+0].setVoltage(rescale(bar,0.0f,10000.0f,0.0f,10.0f));
    // outputs[MS_OUTPUT+1].setVoltage(rescale(half_note_d,0.0f,10000.0f,0.0f,10.0f));
    // outputs[MS_OUTPUT+2].setVoltage(rescale(half_note,0.0f,10000.0f,0.0f,10.0f));
    // outputs[MS_OUTPUT+3].setVoltage(rescale(half_note_t,0.0f,10000.0f,0.0f,10.0f));
    // outputs[MS_OUTPUT+4].setVoltage(rescale(qt_note_d,0.0f,10000.0f,0.0f,10.0f));
    // outputs[MS_OUTPUT+5].setVoltage(rescale(qt_note,0.0f,10000.0f,0.0f,10.0f));
    // outputs[MS_OUTPUT+6].setVoltage(rescale(qt_note_t,0.0f,10000.0f,0.0f,10.0f));
    // outputs[MS_OUTPUT+7].setVoltage(rescale(eight_note_d,0.0f,10000.0f,0.0f,10.0f));
    // outputs[MS_OUTPUT+8].setVoltage(rescale(eight_note,0.0f,10000.0f,0.0f,10.0f));
    // outputs[MS_OUTPUT+9].setVoltage(rescale(eight_note_t,0.0f,10000.0f,0.0f,10.0f));
    // outputs[MS_OUTPUT+10].setVoltage(rescale(sixth_note_d,0.0f,10000.0f,0.0f,10.0f));
    // outputs[MS_OUTPUT+11].setVoltage(rescale(sixth_note,0.0f,10000.0f,0.0f,10.0f));
    // outputs[MS_OUTPUT+12].setVoltage(rescale(sixth_note_t,0.0f,10000.0f,0.0f,10.0f));
    // outputs[MS_OUTPUT+13].setVoltage(rescale(trth_note_d,0.0f,10000.0f,0.0f,10.0f));
    // outputs[MS_OUTPUT+14].setVoltage(rescale(trth_note,0.0f,10000.0f,0.0f,10.0f));
    // outputs[MS_OUTPUT+15].setVoltage(rescale(trth_note_t,0.0f,10000.0f,0.0f,10.0f));

    }
};

struct PingPongWidget : ModuleWidget {
  PingPongWidget(PingPong *module) {
    setModule(module);
    box.size = Vec(15*10, 380);

    SVGPanel *panel = new SVGPanel();
    panel->box.size = box.size;
    panel->setBackground(SVG::load(assetPlugin(pluginInstance, "res/PingPong.svg")));
    addChild(panel);

    addChild(createWidget<ScrewSilver>(Vec(15, 0)));
    addChild(createWidget<ScrewSilver>(Vec(box.size.x-30, 0)));
    addChild(createWidget<ScrewSilver>(Vec(15, 365)));
    addChild(createWidget<ScrewSilver>(Vec(box.size.x-30, 365)));

    // Displays
    if(module != NULL){
        PingPongSmallStringDisplayWidget *fileDisplay = new PingPongSmallStringDisplayWidget();
        fileDisplay->box.pos = Vec(28, 70);
        fileDisplay->box.size = Vec(100, 40);
        fileDisplay->value = &module->rate_display;
        addChild(fileDisplay);
    }

    // Knobs

    // Inputs and Knobs
    addInput(createPort<PJ301MPort>(Vec(16, 320), PortWidget::INPUT, module, PingPong::CLOCK_INPUT));
    addOutput(createPort<PJ301MPort>(Vec(75.5, 320), PortWidget::OUTPUT, module, PingPong::LEFT_OUTPUT));
    addOutput(createPort<PJ301MPort>(Vec(112.5, 320), PortWidget::OUTPUT, module, PingPong::RIGHT_OUTPUT));
    }
};

Model *modelPingPong = createModel<PingPong, PingPongWidget>("PingPong");
