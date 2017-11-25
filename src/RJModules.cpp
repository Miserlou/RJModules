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
}
