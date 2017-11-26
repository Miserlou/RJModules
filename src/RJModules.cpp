#include "RJModules.hpp"

Plugin *plugin;

void init(rack::Plugin *p) {
	plugin = p;
	p->slug = "RJModules";
#ifdef VERSION
	p->version = TOSTRING(VERSION);
#endif
	p->website = "https://github.com/Miserlou/RJModules";

	p->addModel(createModel<BitCrushWidget>("RJModules", "BitCrush", "BitCrush", FILTER_TAG));
    p->addModel(createModel<IntegersWidget>("RJModules", "Integers", "Integers", UTILITY_TAG));
    p->addModel(createModel<FloatsWidget>("RJModules", "Floats", "Floats", UTILITY_TAG));
    p->addModel(createModel<RandomsWidget>("RJModules", "Randoms", "Randoms", UTILITY_TAG));

}
