#include "rack.hpp"

using namespace rack;

extern Plugin *plugin;

////////////////////
// module widgets
////////////////////

struct BitCrushWidget: ModuleWidget {
	BitCrushWidget();
};

struct IntegersWidget: ModuleWidget {
    IntegersWidget();
};

struct FloatsWidget: ModuleWidget {
    FloatsWidget();
};