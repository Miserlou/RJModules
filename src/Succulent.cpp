/*
Succccccccccccccc

*/

#include "Bitmap.hpp"

using namespace rack;

// Forward-declare the Plugin, defined in Template.cpp
extern Plugin *pluginInstance;

// Forward-declare each Model, defined in each module source file
extern Model *modelBlank_1HP;

struct BlankBaseWidget : ModuleWidget {
    static constexpr int LISTSIZE = 2;
    int selected = 0;
    std::string fileName[LISTSIZE];
    BitMap *bmp;
    std::string FileName(std::string tpl, int templateSize) {
        char workingSpace[100];
        snprintf(workingSpace, 100, tpl.c_str(), templateSize);
        return asset::plugin(pluginInstance, workingSpace);
    }

    BlankBaseWidget(Module *module) : ModuleWidget() {
        setModule(module);
    }
    void appendContextMenu(Menu *menu) override;
    void loadBitmap() {
        bmp = createWidget<BitMap>(Vec(0,0));
        bmp->box.size.x = box.size.x;
        bmp->box.size.y = box.size.y;
        bmp->path = fileName[selected];
        addChild(bmp);
    }
    void setBitmap(int sel) {
        if (selected == sel)
            return;
        selected = clamp(sel, 0, LISTSIZE - 1);
        removeChild(bmp);
        delete bmp;
        loadBitmap();
    }
    json_t *toJson() override {
        json_t *rootJ = ModuleWidget::toJson();
        json_object_set_new(rootJ, "style", json_real(selected));
        return rootJ;
    }
    void fromJson(json_t *rootJ) override {
        ModuleWidget::fromJson(rootJ);
        int sel = selected;
        json_t *styleJ = json_object_get(rootJ, "style");
        if (styleJ)
            sel = json_number_value(styleJ);
        setBitmap(sel);
    }

};

struct BitmapMenuItem : MenuItem {
    BlankBaseWidget *w;
    int value;
    void onAction(const event::Action &e) override {
        w->setBitmap(value);
    }
};

void BlankBaseWidget::appendContextMenu(Menu *menu) {
    menu->addChild(new MenuEntry);
    BitmapMenuItem *m = createMenuItem<BitmapMenuItem>("Succccc");
    m->w = this;
    m->value = 0;
    m->rightText = CHECKMARK(selected==m->value);
    menu->addChild(m);
    m = createMenuItem<BitmapMenuItem>("Meow");
    m->w = this;
    m->value = 1;
    m->rightText = CHECKMARK(selected==m->value);
    menu->addChild(m);
}

template<int x>
struct SucculentWidget : BlankBaseWidget {
    SucculentWidget(Module *module) : BlankBaseWidget(module) {
        fileName[0] = FileName("res/Blank_%dHP.png", x);
        fileName[1] = FileName("res/Zen_%dHP.png", x);
        box.size = Vec(RACK_GRID_WIDTH * x, RACK_GRID_HEIGHT);
        loadBitmap();
    }
};

Model *modelSucculent = createModel<Module, SucculentWidget<20>>("Succulent");