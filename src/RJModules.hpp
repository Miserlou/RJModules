#include "rack.hpp"

using namespace rack;

extern Plugin *plugin;

////////////////////
// module widgets
////////////////////

struct SupersawWidget: ModuleWidget {
    SupersawWidget();
};

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

struct SplittersWidget: ModuleWidget {
    SplittersWidget();
};

struct PannerWidget: ModuleWidget {
    PannerWidget();
};

struct MonoWidget: ModuleWidget {
    MonoWidget();
};

struct VolumesWidget: ModuleWidget {
    VolumesWidget();
};

struct FilterDelayWidget: ModuleWidget {
    FilterDelayWidget();
};

struct LRMixerWidget: ModuleWidget {
    LRMixerWidget();
};

struct BPMWidget: ModuleWidget {
    BPMWidget();
};

struct DisplaysWidget: ModuleWidget {
    DisplaysWidget();
};

// struct FFTunerWidget: ModuleWidget {
//     FFTunerWidget();
// };