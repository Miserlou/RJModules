#include "RJModules.hpp"
#include "VAStateVariableFilter.h"

Plugin *pluginInstance;

void init(rack::Plugin *p) {
	pluginInstance = p;

    // Generators
    p->addModel(modelSupersaw);
    p->addModel(modelTwinLFO);
    p->addModel(modelNoise);
    p->addModel(modelRangeLFO);
    p->addModel(modelAcid);
    p->addModel(modelEssEff);
    // p->addModel(modelDrumpler);

    // p->addModel(modelRiser);
    // p->addModel(modelThreeXOSC);

    // VCA
    p->addModel(modelPluck);

    // FX
    p->addModel(modelBitCrush);
    p->addModel(modelWidener);
    p->addModel(modelFilterDelay);
    p->addModel(modelSidechain);
    p->addModel(modelStutter);
    p->addModel(modelGlides);
    // p->addModel(modelPolySidechain);
    p->addModel(modelPingPong);
    p->addModel(modelBrickwall);
    p->addModel(modelSlapback);

    // Filters
    p->addModel(modelFilter);
    p->addModel(modelFilters);
    p->addModel(modelNotch);
    p->addModel(modelBPF);
    p->addModel(modelKTF);
    p->addModel(modelRandomFilter);

    // Numerical
    p->addModel(modelIntegers);
    p->addModel(modelFloats);
    p->addModel(modelRandoms);
    p->addModel(modelNotes);

    // Mix
    p->addModel(modelLRMixer);
    p->addModel(modelMono);
    p->addModel(modelVolumes);
    p->addModel(modelPanner);
    p->addModel(modelPanners);
    p->addModel(modelDryWet);

    // Live
    p->addModel(modelBPM);
    p->addModel(modelButton);
    p->addModel(modelButtons);
    p->addModel(modelMetaKnob);
    p->addModel(modelReplayKnob);
    p->addModel(modelTriggerSwitch);

    // Util
    p->addModel(modelSplitter);
    p->addModel(modelSplitters);
    p->addModel(modelDisplays);
    p->addModel(modelRange);
    p->addModel(modelOctaves);
    p->addModel(modelBuffers);
    p->addModel(modelChord);
    p->addModel(modelPlayableChord);
    p->addModel(modelSucculent);
    p->addModel(modelLeftHandRightHand);
    // p->addModel(modelSoundpipe);

    // Sequencer
    p->addModel(modelChordSeq);
    p->addModel(modelMutateSeq);
    p->addModel(modelSequential);
    p->addModel(modelEuclidian);

    // Quantizer
    p->addModel(modeluQuant);

}
