#include "RJModules.hpp"
#include "VAStateVariableFilter.h"

Plugin *plugin;

void init(rack::Plugin *p) {
	plugin = p;
	p->slug = "RJModules";
#ifdef VERSION
	p->version = TOSTRING(VERSION);
#endif
	p->website = "https://github.com/Miserlou/RJModules";

    // Generators
    p->addModel(createModel<SupersawWidget>("RJModules", "Supersaw", "[GEN] Supersaw", LFO_TAG));
    p->addModel(createModel<TwinLFOWidget>("RJModules", "TwinLFO", "[GEN] TwinLFO", LFO_TAG));
    p->addModel(createModel<NoiseWidget>("RJModules", "Noise", "[GEN] Noise", LFO_TAG));
    p->addModel(createModel<RangeLFOWidget>("RJModules", "RangeLFO", "[GEN] RangeLFO", LFO_TAG));

    // FX
	p->addModel(createModel<BitCrushWidget>("RJModules", "BitCrush", "[FX] BitCrush", DISTORTION_TAG));
    p->addModel(createModel<WidenerWidget>("RJModules", "Widener", "[FX] Widener", UTILITY_TAG));
    p->addModel(createModel<FilterDelayWidget>("RJModules", "FilterDelay", "[FX] FilterDelay", DELAY_TAG));
    p->addModel(createModel<SidechainWidget>("RJModules", "Sidechain", "[FX] Sidechain", UTILITY_TAG));
    p->addModel(createModel<StutterWidget>("RJModules", "Stutter", "[FX] Stutter", DELAY_TAG));

    // Filters
    p->addModel(createModel<FilterWidget>("RJModules", "Filter", "[FILT] Filter", UTILITY_TAG));
    p->addModel(createModel<FiltersWidget>("RJModules", "Filters", "[FILT] Filters", UTILITY_TAG));
    p->addModel(createModel<NotchWidget>("RJModules", "Notch", "[FILT] Notch", UTILITY_TAG));

    // Numerical
    p->addModel(createModel<IntegersWidget>("RJModules", "Integers", "[NUM] Integers", UTILITY_TAG));
    p->addModel(createModel<FloatsWidget>("RJModules", "Floats", "[NUM] Floats", UTILITY_TAG));
    p->addModel(createModel<RandomsWidget>("RJModules", "Randoms", "[NUM] Randoms", UTILITY_TAG));

    // Mix
    p->addModel(createModel<LRMixerWidget>("RJModules", "LRMixer", "[MIX] LRMixer", UTILITY_TAG));
    p->addModel(createModel<MonoWidget>("RJModules", "Mono", "[MIX] Mono", UTILITY_TAG));
    p->addModel(createModel<VolumesWidget>("RJModules", "Volumes", "[MIX] Volumes", UTILITY_TAG));
    p->addModel(createModel<PannerWidget>("RJModules", "Panner", "[MIX] Panner", UTILITY_TAG));
    p->addModel(createModel<PannersWidget>("RJModules", "Panners", "[MIX] Panners", UTILITY_TAG));

    // Live
    p->addModel(createModel<BPMWidget>("RJModules", "BPM", "[LIVE] BPM", UTILITY_TAG));
    p->addModel(createModel<ButtonWidget>("RJModules", "Button", "[LIVE] Button", UTILITY_TAG));
    p->addModel(createModel<ButtonsWidget>("RJModules", "Buttons", "[LIVE] Buttons", UTILITY_TAG));

    // Util
    p->addModel(createModel<SplitterWidget>("RJModules", "Splitter", "[UTIL] Splitter", UTILITY_TAG));
    p->addModel(createModel<SplittersWidget>("RJModules", "Splitters", "[UTIL] Splitters", UTILITY_TAG));
    p->addModel(createModel<DisplaysWidget>("RJModules", "Displays", "[UTIL] Displays", UTILITY_TAG));
    p->addModel(createModel<RangeWidget>("RJModules", "Range", "[UTIL] Range", UTILITY_TAG));


}
