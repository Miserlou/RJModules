#pragma once

#include <iostream>

#include "AH.hpp"
#include "componentlibrary.hpp"

struct ParamEvent {

	ParamEvent(int t, int i, float v) : pType(t), pId(i), value(v) {}

	int pType;
	int pId;
	float value;

};

struct AHModule : Module {

	float delta;
	float rho;

	AHModule(int numParams, int numInputs, int numOutputs, int numLights = 0) : Module(numParams, numInputs, numOutputs, numLights) {
		delta = engineGetSampleTime();
		rho = engineGetSampleRate();
	}

	void onSampleRateChange() override {
		delta = engineGetSampleTime();
		rho = engineGetSampleRate();
	}

	int stepX = 0;

	bool debugFlag = false;

	inline bool debugEnabled() {
		return debugFlag;
	}

	bool receiveEvents = false;
	int keepStateDisplay = 0;
	std::string paramState = ">";

	virtual void receiveEvent(ParamEvent e) {
		paramState = ">";
		keepStateDisplay = 0;
	}

	void step() override {

		stepX++;

		// Once we start stepping, we can process events
		receiveEvents = true;
		// Timeout for display
		keepStateDisplay++;
		if (keepStateDisplay > 50000) {
			paramState = ">";
		}

	}

};

struct StateDisplay : TransparentWidget {

	AHModule *module;
	int frame = 0;
	std::shared_ptr<Font> font;

	StateDisplay() {
		font = Font::load(assetPlugin(pluginInstance, "res/EurostileBold.ttf"));
	}

	void draw(NVGcontext *vg) override {

		Vec pos = Vec(0, 15);

		nvgFontSize(vg, 16);
		nvgFontFaceId(vg, font->handle);
		nvgTextLetterSpacing(vg, -1);

		nvgFillColor(vg, nvgRGBA(255, 0, 0, 0xff));

		char text[128];
		snprintf(text, sizeof(text), "%s", module->paramState.c_str());
		nvgText(vg, pos.x + 10, pos.y + 5, text, NULL);

	}

};

struct AHParamWidget { // it's a mix-in

	int pType = -1; // Should be set by ste<>(), but if not this allows us to catch pwidgers we are not interested in
	int pId;
	AHModule *mod = NULL;

	virtual ParamEvent generateEvent(float value) {
		return ParamEvent(pType,pId,value);
	};

	template <typename T = AHParamWidget>
	static void set(T *param, int pType, int pId) {
		param->pType = pType;
		param->pId = pId;
	}

};

// Not going to monitor buttons
struct AHButton : SVGSwitch {
	AHButton() {
		addFrame(SVG::load(assetPlugin(pluginInstance,"res/ComponentLibrary/AHButton.svg")));
	}
};

struct AHKnob : RoundKnob, AHParamWidget {
	void onChange(const event::Change &e) override {
		// One off cast, don't want to subclass from ParamWidget, so have to grab it here
		// if (!mod) {
		// 	mod = static_cast<AHModule *>(this->module);
		// }
		// mod->receiveEvent(generateEvent(value));
		RoundKnob::onChange(e);
	}
};

struct AHKnobSnap : AHKnob {
	AHKnobSnap() {
		snap = true;
		setSVG(SVG::load(assetPlugin(pluginInstance,"res/ComponentLibrary/AHKnob.svg")));
	}
};

struct AHKnobNoSnap : AHKnob {
	AHKnobNoSnap() {
		snap = false;
		setSVG(SVG::load(assetPlugin(pluginInstance,"res/ComponentLibrary/AHKnob.svg")));
	}
};

struct AHBigKnobNoSnap : AHKnob {
	AHBigKnobNoSnap() {
		snap = false;
		setSVG(SVG::load(assetPlugin(pluginInstance,"res/ComponentLibrary/AHBigKnob.svg")));
	}
};

struct AHBigKnobSnap : AHKnob {
	AHBigKnobSnap() {
		snap = true;
		setSVG(SVG::load(assetPlugin(pluginInstance,"res/ComponentLibrary/AHBigKnob.svg")));
	}
};

struct AHTrimpotSnap : AHKnob {
	AHTrimpotSnap() {
		snap = true;
		setSVG(SVG::load(assetPlugin(pluginInstance,"res/ComponentLibrary/AHTrimpot.svg")));
	}
};

struct AHTrimpotNoSnap : AHKnob {
	AHTrimpotNoSnap() {
		snap = false;
		setSVG(SVG::load(assetPlugin(pluginInstance,"res/ComponentLibrary/AHTrimpot.svg")));
	}
};


struct UI {

	enum UIElement {
		KNOB = 0,
		PORT,
		BUTTON,
		LIGHT,
		TRIMPOT
	};

	float Y_KNOB[2]    = {50.8, 56.0}; // w.r.t 22 = 28.8 from bottom
	float Y_PORT[2]    = {49.7, 56.0}; // 27.7
	float Y_BUTTON[2]  = {53.3, 56.0}; // 31.3
	float Y_LIGHT[2]   = {57.7, 56.0}; // 35.7
	float Y_TRIMPOT[2] = {52.8, 56.0}; // 30.8

	float Y_KNOB_COMPACT[2]     = {30.1, 35.0}; // Calculated relative to  PORT=29 and the deltas above
	float Y_PORT_COMPACT[2]     = {29.0, 35.0};
	float Y_BUTTON_COMPACT[2]   = {32.6, 35.0};
	float Y_LIGHT_COMPACT[2]    = {37.0, 35.0};
	float Y_TRIMPOT_COMPACT[2]  = {32.1, 35.0};

	float X_KNOB[2]     = {12.5, 48.0}; // w.r.t 6.5 = 6 from left
	float X_PORT[2]     = {11.5, 48.0}; // 5
	float X_BUTTON[2]   = {14.7, 48.0}; // 8.2
	float X_LIGHT[2]    = {19.1, 48.0}; // 12.6
	float X_TRIMPOT[2]  = {14.7, 48.0}; // 8.2

	float X_KNOB_COMPACT[2]     = {21.0, 35.0}; // 15 + 6, see calc above
	float X_PORT_COMPACT[2]     = {20.0, 35.0}; // 15 + 5
	float X_BUTTON_COMPACT[2]   = {23.2, 35.0}; // 15 + 8.2
	float X_LIGHT_COMPACT[2]    = {27.6, 35.0}; // 15 + 12.6
	float X_TRIMPOT_COMPACT[2]  = {23.2, 35.0}; // 15 + 8.2


	Vec getPosition(int type, int xSlot, int ySlot, bool xDense, bool yDense);

	/* From the numerical key on a keyboard (0 = C, 11 = B), spacing in px between white keys and a starting x and Y coordinate for the C key (in px)
	* calculate the actual X and Y coordinate for a key, and the scale note to which that key belongs (see Midi note mapping)
	* http://www.grantmuller.com/MidiReference/doc/midiReference/ScaleReference.html */
	void calculateKeyboard(int inKey, float spacing, float xOff, float yOff, float *x, float *y, int *scale);

};

/*
These are derived from https://github.com/j4s0n-c/trowaSoft-VCV/blob/master/src/trowaSoftComponents.hpp

shouts TrowaSoft, best sequencer!
*/

struct ColorValueLight : ModuleLightWidget {
	NVGcolor baseColor;
	// Pixels to add for outer radius (either px or relative %).
	float outerRadiusHalo = 0.35;
	bool outerRadiusRelative = true;
	ColorValueLight() : ModuleLightWidget()
	{
		bgColor = nvgRGBA(0x20, 0x20, 0x20, 0xFF);
		borderColor = nvgRGBA(0, 0, 0, 0);
		return;
	};
	virtual ~ColorValueLight(){};
	// Set a single color
	void setColor(NVGcolor bColor)
	{
		color = bColor;
		baseColor = bColor;
		if (baseColors.size() < 1)
		{
			baseColors.push_back(bColor);			
		}
		else
		{
			baseColors[0] = bColor;
		}
	}
	void drawHalo(const DrawArgs &args) override
	{
		float radius = box.size.x / 2.0;
		float oradius = radius + ((outerRadiusRelative) ? (radius*outerRadiusHalo) : outerRadiusHalo);

		nvgBeginPath(args.vg);
		nvgRect(args.vg, radius - oradius, radius - oradius, 2 * oradius, 2 * oradius);

		NVGpaint paint;
		NVGcolor icol = color::mult(color, 0.10);//colorMult(color, 0.10);
		NVGcolor ocol = nvgRGB(0, 0, 0);
		paint = nvgRadialGradient(args.vg, radius, radius, radius, oradius, icol, ocol);
		nvgFillPaint(args.vg, paint);
		nvgGlobalCompositeOperation(args.vg, NVG_LIGHTER);
		nvgFill(args.vg);
	}
};

//--------------------------------------------------------------
// RJ_PadSwitch
// Empty momentary switch of given size.
//--------------------------------------------------------------
struct RJ_PadSwitch : Switch {
	int btnId = -1;
	// Group id (to match guys that should respond to mouse down drag).
	int groupId = -1;
	
	RJ_PadSwitch() : Switch() {
		momentary = true;

		std::cout << "Tloading RJ_PadSwitch\n";
		return;
	}
	RJ_PadSwitch(Vec size) {
		box.size = size;	
		return;
	}
	void setValue(float val) {
		if (paramQuantity)
		{
			paramQuantity->setValue(val);
		}		
		return;
	}
	// Allow mouse-down & drag to set buttons (i.e. on Sequencer grid where there are many buttons). 
	// Suggestion from @LKHSogpit, Solution from @AndrewBelt.
	// https://github.com/j4s0n-c/trowaSoft-VCV/issues/7
	// https://github.com/VCVRack/Rack/issues/607
	/** Called when a widget responds to `onMouseDown` for a left button press */
	void onDragStart(const event::DragStart &e) override {
		if (e.button != GLFW_MOUSE_BUTTON_LEFT)
			return;		
		if (paramQuantity)
		{
			if (momentary)
			{
				paramQuantity->setValue(paramQuantity->maxValue); // Trigger Value				
			}
			else
			{
				float newVal = (paramQuantity->getValue() < paramQuantity->maxValue) ? paramQuantity->maxValue : paramQuantity->minValue;
				paramQuantity->setValue(newVal); // Toggle Value				
			}
		}
		return;
	}
	/** Called when the left button is released and this widget is being dragged */
	// https://github.com/j4s0n-c/trowaSoft-VCV/issues/12
	// Last button keeps pressed down.
	// void onDragEnd(const event::DragEnd &e) override {
		// if (paramQuantity) {
		// }
		// return;
	// }
	/** Called when a widget responds to `onMouseUp` for a left button release and a widget is being dragged */
	void onDragEnter(const event::DragEnter &e) override 
	{	
		if (e.button != GLFW_MOUSE_BUTTON_LEFT)
			return;	
		// Set these no matter what because if you drag back onto your starting square, you want to toggle it again.
		RJ_PadSwitch *origin = dynamic_cast<RJ_PadSwitch*>(e.origin);
		if (origin && origin != this && origin->groupId == this->groupId && paramQuantity) 
		{
			float newVal = (paramQuantity->getValue() < paramQuantity->maxValue) ? paramQuantity->maxValue : paramQuantity->minValue;
			//DEBUG("onDragEnter(%d) - Set Value to %3.1f.", btnId, newVal);				
			paramQuantity->setValue(newVal); // Toggle Value
		}	
	}
	void onDragLeave(const event::DragLeave &e) override 
	{
		if (e.button != GLFW_MOUSE_BUTTON_LEFT)
			return;				
		RJ_PadSwitch *origin = dynamic_cast<RJ_PadSwitch*>(e.origin);
		if (origin && origin->groupId == this->groupId && paramQuantity) 
		{
			if (momentary)
			{
				//DEBUG("onDragLeave(%d) (momentary) - Set Value to %3.1f.", btnId, paramQuantity->minValue);
				paramQuantity->setValue(paramQuantity->minValue); // Turn Off				
			}
		}		
		return;
	}
	void onButton(const event::Button &e) override 
	{
		ParamWidget::onButton(e);
		return;
	}
};

//--------------------------------------------------------------
// RJ_PadSwitch
//--------------------------------------------------------------
struct RJ_PadSvgSwitch : SvgSwitch {
	int btnId = -1;
	// Group id (to match guys that should respond to mouse down drag).
	int groupId = -1;
	RJ_PadSvgSwitch() : SvgSwitch() {
		momentary = false;

		this->shadow->opacity = 0.0f; // Turn off the circular shadows that are everywhere.

		return;
	}
	RJ_PadSvgSwitch(Vec size) : RJ_PadSvgSwitch() {
		box.size = size;	
		return;
	}
	void setValue(float val) {
		if (paramQuantity)
		{
			paramQuantity->setValue(val);
		}		
		return;
	}
	
	void toggleVal()
	{
		if (paramQuantity)
		{
			float newVal = (paramQuantity->getValue() < paramQuantity->maxValue) ? paramQuantity->maxValue : paramQuantity->minValue;
			paramQuantity->setValue(newVal); // Toggle Value
		}
		return;
	}
	
	// Allow mouse-down & drag to set buttons (i.e. on Sequencer grid where there are many buttons). 
	// Suggestion from @LKHSogpit, Solution from @AndrewBelt.
	// https://github.com/j4s0n-c/trowaSoft-VCV/issues/7
	// https://github.com/VCVRack/Rack/issues/607
	/** Called when a widget responds to `onMouseDown` for a left button press */
	void onDragStart(const event::DragStart &e) override {
		if (e.button != GLFW_MOUSE_BUTTON_LEFT)
			return;		
		
		if (paramQuantity)
		{
			// if (momentary)
			// {
			// 	DEBUG("RJ_PadSvgSwitch onDragStart(%d) - Momentary - Set Value to %3.1f.", btnId, paramQuantity->maxValue);
			// 	paramQuantity->setValue(paramQuantity->maxValue); // Trigger Value				
			// }
			// else
			// {
			// 	float newVal = (paramQuantity->getValue() < paramQuantity->maxValue) ? paramQuantity->maxValue : paramQuantity->minValue;
			// 	DEBUG("RJ_PadSvgSwitch onDragStart(%d) - Set Value to %3.1f.", btnId, newVal);						
			// 	paramQuantity->setValue(newVal); // Toggle Value
			// }

			float newVal = (paramQuantity->getValue() < paramQuantity->maxValue) ? paramQuantity->maxValue : paramQuantity->minValue;
			DEBUG("RJ_PadSvgSwitch onDragStart(%d) - Set Value to %3.1f.", btnId, newVal);						
			paramQuantity->setValue(1.0); // Toggle Value

		}	
		return;
	}
	/** Called when the left button is released and this widget is being dragged */
	// https://github.com/j4s0n-c/trowaSoft-VCV/issues/12
	// Last button keeps pressed down.
	// void onDragEnd(const event::DragEnd &e) override 
	// {		
		// return;
	// }
	
	/** Called when a widget responds to `onMouseUp` for a left button release and a widget is being dragged */
	void onDragEnter(const event::DragEnter &e) override 
	{	
		if (e.button != GLFW_MOUSE_BUTTON_LEFT)
			return;			
		// Set these no matter what because if you drag back onto your starting square, you want to toggle it again.
		RJ_PadSvgSwitch *origin = dynamic_cast<RJ_PadSvgSwitch*>(e.origin);			
		// XXX THIS
		// if (origin && origin != this && origin->groupId == this->groupId && paramQuantity) 
		// {
		if (origin->groupId == this->groupId && paramQuantity) 
		{
			float newVal = (paramQuantity->getValue() < paramQuantity->maxValue) ? paramQuantity->maxValue : paramQuantity->minValue;
			DEBUG("RJ_PadSvgSwitch onDragEnter(%d) - Set Value to %3.1f.", btnId, newVal);				
			paramQuantity->setValue(1.0); // Toggle Value
		}		
		return;
	}
	void onDragLeave(const event::DragLeave &e) override {
		if (e.button != GLFW_MOUSE_BUTTON_LEFT)
			return;		
		SvgSwitch::onDragLeave(e);
		paramQuantity->setValue(0);
		return;
	}
	void onButton(const event::Button &e) override 
	{
		this->ParamWidget::onButton(e); // Need to call this base method to be set as the touchedParam for MIDI mapping to work.
	}
};


//--------------------------------------------------------------
// RJ_PadSquare - A Square Pad button.
//--------------------------------------------------------------
struct RJ_PadSquare : RJ_PadSvgSwitch {
	RJ_PadSquare() 
	{
		std::cout << "Tloading svg\n";
		this->SvgSwitch::addFrame(APP->window->loadSvg(asset::plugin(pluginInstance,"res/TS_pad_0.svg")));
		sw->wrap();
		SvgSwitch::box.size = sw->box.size;
	}
	RJ_PadSquare(Vec size)
	{
		std::cout << "Tloading svg2\n";
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/TS_pad_0.svg")));
		sw->box.size = size;
		SvgSwitch::box.size = size;
	}
};

struct RJ_LightSquare : ColorValueLight 
{
	// Radius on corners
	float cornerRadius = 5.0;
	RJ_LightSquare()
	{
		bgColor = nvgRGBAf(0, 0, 0, /*alpha */ 0.5);
		baseColor = componentlibrary::SCHEME_RED;
	}
	void draw(const DrawArgs &args) override
	{
		float radius = box.size.x / 2.0;
		float oradius = radius*1.1;

		NVGcolor backColor = bgColor;
		NVGcolor outerColor = color;
		// Solid
		nvgBeginPath(args.vg);
		nvgRoundedRect(args.vg, 0.0, 0.0, box.size.x, box.size.y, cornerRadius);
		nvgFillColor(args.vg, backColor);
		nvgFill(args.vg);

		// Border
		nvgStrokeWidth(args.vg, 1.0);
		NVGcolor borderColor = bgColor;
		borderColor.a *= 0.5;
		nvgStrokeColor(args.vg, borderColor);
		nvgStroke(args.vg);

		// Inner glow
		nvgGlobalCompositeOperation(args.vg, NVG_LIGHTER);
		nvgFillColor(args.vg, color);
		nvgFill(args.vg);

		// Outer glow
		nvgBeginPath(args.vg);
		nvgRoundedRect(args.vg, /*x*/ radius - oradius, /*y*/ radius - oradius, /*w*/ 2*oradius, /*h*/ 2*oradius, cornerRadius);
		NVGpaint paint;
		NVGcolor icol = outerColor;// color;
		icol.a *= 0.25;
		NVGcolor ocol = outerColor;// color;
		ocol.a = 0.0;
		float feather = 1;
		// Feather defines how blurry the border of the rectangle is. // Fixed 01/19/2018, made it too tiny before
		paint = nvgBoxGradient(args.vg, /*x*/ radius - oradius, /*y*/ radius - oradius, /*w*/ 2 * oradius, /*h*/ 2 * oradius,  //args.vg, /*x*/ -5, /*y*/ -5, /*w*/ 2*oradius + 10, /*h*/ 2*oradius + 10, 
			/*r: corner radius*/ cornerRadius, /*f: feather*/ feather, 
			/*inner color*/ icol, /*outer color */ ocol);
		nvgFillPaint(args.vg, paint);
		nvgFill(args.vg);
		return;
	}
}; // end RJ_LightSquare

template <class TModuleLightWidget>
ColorValueLight * RJ_createColorValueLight(Vec pos,  Module *module, int lightId, Vec size, NVGcolor lightColor) {
	ColorValueLight *light = new TModuleLightWidget();
	light->box.pos = pos;
	light->module = module;
	light->firstLightId = lightId;	
	//light->value = value;
	light->box.size = size;
	light->setColor(lightColor);
	//light->baseColor = lightColor;
	return light;
}
template <class TModuleLightWidget>
ColorValueLight * RJ_createColorValueLight(Vec pos, Module *module, int lightId, Vec size, NVGcolor lightColor, NVGcolor backColor) {
	ColorValueLight *light = new TModuleLightWidget();
	light->box.pos = pos;
	light->module = module;
	light->firstLightId = lightId;	
	light->box.size = size;
	light->setColor(lightColor);	
	light->bgColor = backColor;
	return light;
}
