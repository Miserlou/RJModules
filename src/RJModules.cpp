#include "RJModules.hpp"

Plugin *plugin;

void init(rack::Plugin *p) {
	plugin = p;
	p->slug = "RJModules";
#ifdef VERSION
	p->version = TOSTRING(VERSION);
#endif
	p->website = "https://github.com/Miserlou/RJModules";

	p->addModel(createModel<BitCrushWidget>("RJModules", "BitCrush", "BitCrush", DISTORTION_TAG));
    p->addModel(createModel<IntegersWidget>("RJModules", "Integers", "Integers", UTILITY_TAG));
    p->addModel(createModel<FloatsWidget>("RJModules", "Floats", "Floats", UTILITY_TAG));
    p->addModel(createModel<RandomsWidget>("RJModules", "Randoms", "Randoms", UTILITY_TAG));
    p->addModel(createModel<ButtonWidget>("RJModules", "Button", "Button", UTILITY_TAG));
    p->addModel(createModel<ButtonsWidget>("RJModules", "Buttons", "Buttons", UTILITY_TAG));
    p->addModel(createModel<SplitterWidget>("RJModules", "Splitter", "Splitter", UTILITY_TAG));
    p->addModel(createModel<SplittersWidget>("RJModules", "Splitters", "Splitters", UTILITY_TAG));
    p->addModel(createModel<PannerWidget>("RJModules", "Panner", "Panner", UTILITY_TAG));
    p->addModel(createModel<FilterDelayWidget>("RJModules", "FilterDelay", "FilterDelay", DELAY_TAG));
    p->addModel(createModel<LRMixerWidget>("RJModules", "LRMixer", "LRMixer", UTILITY_TAG));
    p->addModel(createModel<SupersawWidget>("RJModules", "Supersaw", "Supersaw", LFO_TAG));
    p->addModel(createModel<MonoWidget>("RJModules", "Mono", "Mono", UTILITY_TAG));
    p->addModel(createModel<VolumesWidget>("RJModules", "Volumes", "Volumes", UTILITY_TAG));
    p->addModel(createModel<BPMWidget>("RJModules", "BPM", "BPM", UTILITY_TAG));
    p->addModel(createModel<DisplaysWidget>("RJModules", "Displays", "Displays", UTILITY_TAG));
    p->addModel(createModel<SidechainWidget>("RJModules", "Sidechain", "Sidechain", UTILITY_TAG));


}
