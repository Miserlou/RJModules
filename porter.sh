#! /bin/bash
perl -i -pe 's/Model::create/createModel/g' src/*
perl -i -pe 's/ParamWidget::create/createParam/g' src/*
perl -i -pe 's/ModuleLightWidget::create/createLight/g' src/*
perl -i -pe 's/Port::create/createPort/g' src/*
perl -i -pe 's/Port::OUTPUT/PortWidget::OUTPUT/g' src/*
perl -i -pe 's/Port::INPUT/PortWidget::INPUT/g' src/*
perl -i -pe 's/Widget::create/createWidget/g' src/*
perl -i -pe 's/MenuEntry::create\(\)/new MenuEntry/g' src/*
perl -i -pe 's/MenuLabel::create/createMenuLabel/g' src/*
perl -i -pe 's/MenuItem::create/createMenuItem/g' src/*
perl -i -pe 's/toJson/dataToJson/g' src/*
perl -i -pe 's/fromJson/dataFromJson/g' src/*
perl -i -pe 's/: Module\(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS\) \{/{\n\t\tconfig(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS);/g' src/*
perl -i -pe 's/: Module\(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS\) \{/{\n\t\tconfig(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);/g' src/*
perl -i -pe 's/: ModuleWidget\(module\) \{/{\n\t\tsetModule(module);/g' src/*
