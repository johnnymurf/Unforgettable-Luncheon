/*
* Purpose of this module is to allow for timed triggers based on musical timings
* Musical timings will be based on Phrase, Bar, Beat, and One-Shot
*
*/

#include "UL.hpp"
#include "dsp/digital.hpp"

struct PhraseTrigger : Module {
		enum ParamIds {
		ARM_PARAM,
		NUM_PARAMS
	 };
	enum InputIds {
		CLOCK_INPUT,
		RESET_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		TRIGGER_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		ARM_LIGHT,
		NUM_LIGHTS
	};


	PhraseTrigger() : Module(NUM_PARAMS,NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {}
	void step() override;

	// For more advanced Module features, read Rack's engine.hpp header file
	//- toJson, fromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - onReset, onRandomize, onCreate, onDelete: implements special behavior when user clicks these from the context menu
};

	SchmittTrigger clockTrigger;
	SchmittTrigger armButtonTrigger;
	PulseGenerator pulseOut; //Used for triggering output pulse
	bool isArmed = false; //for detecting if output is armed
	bool armButton = false;
    bool isBeat = false ; //will be true for every clock pulse input
	int beatCount = 1;
	float deltaTime = 0;

void PhraseTrigger::step() {

		deltaTime = engineGetSampleTime();
	
		armButton = armButtonTrigger.process(params[ARM_PARAM].value);
		
		if(armButton){
			printf("armButton\n");
		}
		if(armButton && !isArmed){
			isArmed = true;
		}


		isBeat = clockTrigger.process(inputs[CLOCK_INPUT].value);

		if((beatCount == 1 && isBeat) && isArmed ){
			pulseOut.trigger(1e-3);
			isArmed = false;
		}


		if(isBeat){
			printf("%d\n",beatCount);
			beatCount++;
		}
		// Test to make a Bar 4 beats long, will be extended to user defined length in future
		if(beatCount > 4){
			outputs[TRIGGER_OUTPUT].value = 0.0;
			beatCount = 1;
		}

		// outputs
		outputs[TRIGGER_OUTPUT].value = pulseOut.process(deltaTime) ? 10.f : 0.f;
		lights[ARM_LIGHT].value = isArmed;
}


struct PhraseTriggerWidget : ModuleWidget {
	PhraseTriggerWidget(PhraseTrigger *module) : ModuleWidget(module) {
		setPanel(SVG::load(assetPlugin(plugin, "res/PhraseTrigger2.svg")));

	//	addChild(Widget::create<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
	//	addChild(Widget::create<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
	//	addChild(Widget::create<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
	//	addChild(Widget::create<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

	// Create arm button with light
	// Light must be x+4, y+4 to be centered.
		addParam(ParamWidget::create<LEDButton>(Vec(36,250), module, PhraseTrigger::ARM_PARAM, 0.0, 1.0, 0.0));
		addChild(ModuleLightWidget::create<MediumLight<RedLight>>(Vec(40.0f, 254.0f), module, PhraseTrigger::ARM_LIGHT));
	//	addParam(ParamWidget::create<Davies1900hBlackKnob>(Vec(28, 87), module, PhraseTrigger::PITCH_PARAM, -3.0, 3.0, 0.0));

		addInput(Port::create<PJ301MPort>(Vec(33, 30), Port::INPUT, module, PhraseTrigger::CLOCK_INPUT));

		addOutput(Port::create<PJ301MPort>(Vec(33, 275), Port::OUTPUT, module, PhraseTrigger::TRIGGER_OUTPUT));

		//addChild(ModuleLightWidget::create<MediumLight<RedLight>>(Vec(41, 59), module, PhraseTrigger::BLINK_LIGHT));
	}
};


// Specify the Module and ModuleWidget subclass, human-readable
// author name for categorization per plugin, module slug (should never
// change), human-readable module name, and any number of tags
// (found in `include/tags.hpp`) separated by commas.
Model *modelPhraseTrigger = Model::create<PhraseTrigger, PhraseTriggerWidget>("Unforgettable Luncheon", "PhraseTrigger", "Phrase Trigger", SEQUENCER_TAG);