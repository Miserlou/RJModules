#include "rack.hpp"

using namespace rack;

extern Plugin *plugin;

////////////////////
// module widgets
////////////////////

struct SupersawWidget: ModuleWidget {
    SupersawWidget();
};

struct TwinLFOWidget: ModuleWidget {
    TwinLFOWidget();
};

struct NoiseWidget: ModuleWidget {
    NoiseWidget();
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

struct ButtonsWidget: ModuleWidget {
    ButtonsWidget();
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

struct PannersWidget: ModuleWidget {
    PannersWidget();
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

struct SidechainWidget: ModuleWidget {
    SidechainWidget();
};

struct FilterWidget: ModuleWidget {
    FilterWidget();
};

struct FiltersWidget: ModuleWidget {
    FiltersWidget();
};

struct NotchWidget: ModuleWidget {
    NotchWidget();
};

struct RangeWidget: ModuleWidget {
    RangeWidget();
};

struct WidenerWidget: ModuleWidget {
    WidenerWidget();
};

struct RangeLFOWidget: ModuleWidget {
    RangeLFOWidget();
};

// struct FFTunerWidget: ModuleWidget {
//     FFTunerWidget();
// };