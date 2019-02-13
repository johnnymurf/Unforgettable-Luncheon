/*
* Purpose of this module is to allow for timed triggers based on musical timings
* Musical timings will be based on Phrase, Bar, Beat, and One-Shot
*
*/
//test
#include "UL.hpp"
#include "dsp/digital.hpp"



const int NUM_ARM_MODULUES = 2;
struct PhraseTrigger : Module {
	enum ParamIds {
			ENUMS(ARM_PARAM, 2),
			NUM_PARAMS
	 };
	enum InputIds {
		CLOCK_INPUT,
		RESET_INPUT,
		ENUMS(ARM_INPUT,2),
		NUM_INPUTS
	};
	enum OutputIds {
		ENUMS(TRIGGER_OUTPUT,2),
		NUM_OUTPUTS
	};
	enum LightIds {
		ENUMS(ARM_LIGHT,2),
		NUM_LIGHTS
	};


	PhraseTrigger() : Module(NUM_PARAMS,NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {}
	void step() override;


	SchmittTrigger clockTrigger;
    bool isBeat = false ; //will be true for every clock pulse input
	// Counts will be used for internal logic, displays will be for graphics
	int beatCount = 1;
	int beatDisplay = 1;
	int barCount = 1; 
	int barDisplay = 1; 
	int PhraseCount = 1;
	int PhraseDisplay
	int beatsPerBar = 4;
	int barsPerPhrase = 8;
	float deltaTime = 0;

	struct armModule{
		SchmittTrigger armButtonTrigger;
		SchmittTrigger armInputTrigger;
		PulseGenerator pulseOut;
		bool isArmed = false;
		bool armButton = false;
		bool armInput = false;
	};

	armModule armModules[NUM_ARM_MODULUES];	
};

void PhraseTrigger::step() {

		deltaTime = engineGetSampleTime();

		//Check if user as armed button or sent armed input externally
		for(int i = 0; i < NUM_ARM_MODULUES; i++){
			armModules[i].armButton = armModules[i].armButtonTrigger.process(params[ARM_PARAM+i].value);
			armModules[i].armInput = armModules[i].armInputTrigger.process(inputs[ARM_INPUT+i].value);
		}
		for(int i = 0; i < NUM_ARM_MODULUES; i++){
			if((armModules[i].armButton || armModules[i].armInput) && !armModules[i].isArmed){
		 		armModules[i].isArmed = true;
		 	}
		}
	

		//True if input to clock is high (receiving input from clock source) 			
		isBeat = clockTrigger.process(inputs[CLOCK_INPUT].value);
		
		for(int i = 0; i < NUM_ARM_MODULUES; i++){
			if((beatCount == 1 && isBeat) && armModules[i].isArmed ){
				armModules[i].pulseOut.trigger(1e-3); //pulseOut will be true for 1mss
				armModules[i].isArmed = false;
			}
		}


// 		TODO - User will determine beatsperBar and barsPerPhrase - default is 4 beats and 8 bars
		if(isBeat){
			printf("%d\n",beatCount);
			beatCount++;
			beatDisplay = beatCount - 1; //prevents display from being off by one as it gets updated AFTER beat count incremement 
			if (beatCount > beatsPerBar){
				beatCount = 1;
				barCount++;
			}
				if (barCount > barsPerPhrase ){
					barCount = 1;
					PhraseCount++;
				}
		}

		// outputs - will pulse if on the beat or show light if it is armed
		for(int i = 0; i < NUM_ARM_MODULUES; i++){
			outputs[TRIGGER_OUTPUT+i].value = armModules[i].pulseOut.process(deltaTime) ? 10.f : 0.f;
			lights[ARM_LIGHT+ i].value = armModules[i].isArmed;
		}
	
	};


// Used to Display Beat Number to the User
struct BeatsDisplayWidget : TransparentWidget{
	//PhraseTrigger *module;
	
	int *beat;
	std::shared_ptr<Font> font;

	BeatsDisplayWidget(){
		font = Font::load(assetPlugin(plugin,"res/DSEG7Classic-Bold.ttf"));
	}
	void draw(NVGcontext *vg) override{
		nvgFontSize(vg, 30);
		nvgFontFaceId(vg, font->handle);
		nvgTextLetterSpacing(vg, -2);

		nvgFillColor(vg, nvgRGBA(0xff, 0x18, 0x00, 0xff));
		char text[100];
		snprintf(text, sizeof(text), " %u",((unsigned) *beat));
		nvgText(vg, 33, 320, text, NULL);
	}
};
struct PhraseTriggerWidget : ModuleWidget {
	PhraseTriggerWidget(PhraseTrigger *module) : ModuleWidget(module) {
		setPanel(SVG::load(assetPlugin(plugin, "res/PhraseTrigger2.svg")));

		addChild(Widget::create<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(Widget::create<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(Widget::create<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(Widget::create<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		//Used for CLock input 
		addInput(Port::create<PJ301MPort>(Vec(33, 30), Port::INPUT, module, PhraseTrigger::CLOCK_INPUT));

		//Adds graphics to module. Takes values from PhaseTrigger and passes to displayWidgets
		{
			BeatsDisplayWidget *beatDisplay = new BeatsDisplayWidget();
				beatDisplay->beat = (&module->beatDisplay);
				addChild(beatDisplay);
				}

		//LED button, Light must be x+4, y+4 to be centered.
		static const float portY[2] = {100, 200};
		for(int i = 0; i < NUM_ARM_MODULUES; i++){
			addParam(ParamWidget::create<LEDButton>(Vec(36,portY[i]), module, PhraseTrigger::ARM_PARAM + i, 0.0, 1.0, 0.0));
			addChild(ModuleLightWidget::create<MediumLight<RedLight>>(Vec(40.0f, portY[i] + 4.0f), module, PhraseTrigger::ARM_LIGHT + i));

			//External input and Trigger Outputs
			addInput(Port::create<PJ301MPort>(Vec(33, portY[i] + 20), Port::INPUT, module, PhraseTrigger::ARM_INPUT + i));
			addOutput(Port::create<PJ301MPort>(Vec(33, portY[i] + 46), Port::OUTPUT, module, PhraseTrigger::TRIGGER_OUTPUT + i));
		}
		//addChild(ModuleLightWidget::create<MediumLight<RedLight>>(Vec(41, 59), module, PhraseTrigger::BLINK_LIGHT));
	}
};


// Specify the Module and ModuleWidget subclass, human-readable
// author name for categorization per plugin, module slug (should never
// change), human-readable module name, and any number of tags
// (found in `include/tags.hpp`) separated by commas.
Model *modelPhraseTrigger = Model::create<PhraseTrigger, PhraseTriggerWidget>("Unforgettable Luncheon", "PhraseTrigger", "Phrase Trigger", SEQUENCER_TAG);