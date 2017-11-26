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

struct RandomsWidget: ModuleWidget {
    RandomsWidget();
};

struct ButtonWidget: ModuleWidget {
    ButtonWidget();
};

struct SplitterWidget: ModuleWidget {
    SplitterWidget();
};

struct PannerWidget: ModuleWidget {
    PannerWidget();
};

struct FilterDelayWidget: ModuleWidget {
    FilterDelayWidget();
};

struct LRMixerWidget: ModuleWidget {
    LRMixerWidget();
};

struct SupersawWidget: ModuleWidget {
    SupersawWidget();
};

// struct FFTunerWidget: ModuleWidget {
//     FFTunerWidget();
// };