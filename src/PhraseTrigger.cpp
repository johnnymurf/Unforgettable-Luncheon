/*
* Purpose of this module is to allow for timed triggers based on musical timings
* Musical timings will be based on Phrase, Bar, Beat, and One-Shot
*
*/
//test
#include "UL.hpp"
#include "dsp/digital.hpp"
#include "cmath"



const int NUM_ARM_MODULUES = 9;
const int NUM_ROWS = 3;
struct PhraseTrigger : Module {
	enum ParamIds {
		RESET_PARAM,
		ENUMS(ARM_PARAM, NUM_ARM_MODULUES),
		ENUMS(ARM_BAR, NUM_ARM_MODULUES),
		ENUMS(ARM_PHRASE, NUM_ARM_MODULUES),
//TO DO		RESET_PARAM,
		NUM_PARAMS
	 };
	enum InputIds {
		CLOCK_INPUT,
		RESET_INPUT,
		ENUMS(ARM_INPUT,NUM_ARM_MODULUES),
		ENUMS(CLOCK_INPUTS,NUM_ARM_MODULUES),
		NUM_INPUTS
	};
	enum OutputIds {
		ENUMS(TRIGGER_OUTPUT,NUM_ARM_MODULUES),
		NUM_OUTPUTS
	};
	enum LightIds {
		RESET_LIGHT,
		ENUMS(ARM_LIGHT,NUM_ARM_MODULUES),
		ENUMS(ARM_BAR_LIGHTS, NUM_ARM_MODULUES),
		ENUMS(ARM_PHRASE_LIGHTS, NUM_ARM_MODULUES),
		NUM_LIGHTS
	};


	PhraseTrigger() : Module(NUM_PARAMS,NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {}
	void step() override;


	SchmittTrigger clockTrigger;
	SchmittTrigger resetTrigger;
	SchmittTrigger resetButton;
    bool isBeat = false ; //will be true for every clock pulse input
	bool isReset = false;
	// Counts will be used for internal logic, displays will be for graphics
	int beatCount = 1;
	int beatDisplay = 0;
	int barCount = 1; 
	int barDisplay = 1; 
	int phraseCount = 1;
	int phraseDisplay = 1;
	int beatsPerBar = 4;
	int barsPerPhrase = 8;
	float deltaTime = 0;
	int totalBeats = 0;

	struct armModule{
		SchmittTrigger armButtonTrigger;
		SchmittTrigger armInputTrigger;
		SchmittTrigger clockTrigger;
		SchmittTrigger armBar;
		SchmittTrigger armPhrase;
		PulseGenerator pulseOut;
 
		bool isArmed = false;
		bool armButton = false;
		bool armInput = false;
		bool hasChosenBar = true; //default will be trigger on bar  
		bool hasChosenPhrase = false;
		bool openTrigger = false;
	};


armModule armModules[NUM_ARM_MODULUES];	
};

void PhraseTrigger::step() {
		deltaTime = engineGetSampleTime();

		//Check if user has armed button or sent armed input externally, and chosen bar or phrase
		for(int i = 0; i < NUM_ARM_MODULUES; i++){
			armModules[i].armButton = armModules[i].armButtonTrigger.process(params[ARM_PARAM+i].value);
			armModules[i].armInput = armModules[i].armInputTrigger.process(inputs[ARM_INPUT+i].value);
			if(armModules[i].armBar.process(params[ARM_BAR+i].value)){
				armModules[i].hasChosenBar = true;
				armModules[i].hasChosenPhrase = false;
			}
			if(armModules[i].armPhrase.process(params[ARM_PHRASE+i].value)){
				armModules[i].hasChosenBar = false;
				armModules[i].hasChosenPhrase = true;
			}
	
		}

		// On receiving arm input and not already armed, arm module.
		for(int i = 0; i < NUM_ARM_MODULUES; i++){
			if((armModules[i].armButton || armModules[i].armInput) && !armModules[i].isArmed){
		 		armModules[i].isArmed = true;
		 	}
		}
		//True if input to clock is high (receiving input from clock source) 			
		isBeat = clockTrigger.process(inputs[CLOCK_INPUT].value);

		if( resetTrigger.process(inputs[RESET_INPUT].value)  || resetButton.process(params[RESET_PARAM].value) ){
			beatCount = 1; 
			barCount = 1;
			phraseCount = 1;
			beatDisplay = beatCount - 1;
			barDisplay = barCount;
			phraseDisplay = phraseCount;
			for(int i = 0; i < NUM_ARM_MODULUES; i++){
				armModules[i].openTrigger = false;
			}
		}

		
		for(int i = 0; i < NUM_ARM_MODULUES; i++){
			if((barCount == 1 && beatCount == 1) &&  isBeat && armModules[i].hasChosenPhrase && armModules[i].isArmed){
				armModules[i].openTrigger = !armModules[i].openTrigger;
			 	armModules[i].isArmed = false;
			 }
			if( (beatCount == 1) && isBeat && armModules[i].hasChosenBar && armModules[i].isArmed){
				armModules[i].openTrigger = !armModules[i].openTrigger;
				armModules[i].isArmed = false;
			}

		}
		
		//Incremement beat counters;
		if(isBeat && !resetButton.process(params[RESET_PARAM].value) ){
			barDisplay = barCount;
			phraseDisplay = phraseCount;
			beatCount++;
			totalBeats++; 
			beatDisplay = beatCount - 1; //prevents display from being off by one as it gets updated AFTER beat count incremement 
			if (beatCount > beatsPerBar){
				beatCount = 1;
				barCount++;
				
			}
				if (barCount > barsPerPhrase){
					barCount = 1;
					phraseCount++;
				}
		}

		// outputs - will pulse if on the beat or show light if it is armed and if its set to output on bar or phrase
		for(int i = 0; i < NUM_ARM_MODULUES; i++){
			if(armModules[i].openTrigger){
				if(armModules[i].clockTrigger.process(inputs[CLOCK_INPUTS+ i].value)){
					outputs[TRIGGER_OUTPUT+i].value = 10.0f;
				}
				else{
					outputs[TRIGGER_OUTPUT+i].value = 0.0f;
				}

			}
			
			lights[ARM_LIGHT + i].value = armModules[i].isArmed;
			lights[ARM_BAR_LIGHTS + i].value = armModules[i].hasChosenBar;
			lights[ARM_PHRASE_LIGHTS + i].value = armModules[i].hasChosenPhrase;
		}
		lights[RESET_LIGHT].setBrightnessSmooth(resetTrigger.isHigh());
		lights[RESET_LIGHT].setBrightnessSmooth(resetButton.isHigh());

	};
// Used to Display Beat Number to the User
struct TimeDisplayWidget : TransparentWidget{

	int *beat;
	int *bar;
	int *phrase;
	std::shared_ptr<Font> font;

 TimeDisplayWidget(){
		font = Font::load(assetPlugin(plugin,"res/DSEG14Classic-Italic.ttf"));
	}
	void draw(NVGcontext *vg) override{
		nvgFontSize(vg, 20);
		nvgFontFaceId(vg, font->handle);
		nvgTextLetterSpacing(vg, 2);
		Vec textPos = Vec(140,40);
		nvgFillColor(vg, nvgRGBA(0xff, 0x18, 0x00, 0xff));
		char text[250];
		snprintf(text, sizeof(text), "%02u  :  %02u  :  %02u",((unsigned) *phrase),((unsigned) *bar),((unsigned) *beat));
		nvgText(vg, textPos.x, textPos.y, text, NULL);
	}
};



struct PhraseTriggerWidget : ModuleWidget {
	PhraseTriggerWidget(PhraseTrigger *module) : ModuleWidget(module) {
		setPanel(SVG::load(assetPlugin(plugin, "res/PhraseTrigger3.svg")));

		addChild(Widget::create<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(Widget::create<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(Widget::create<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(Widget::create<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		//Used for CLock input 
		addInput(Port::create<PJ301MPort>(Vec(33, 20), Port::INPUT, module, PhraseTrigger::CLOCK_INPUT));
		addInput(Port::create<PJ301MPort>(Vec(33, 50), Port::INPUT, module, PhraseTrigger::RESET_INPUT));
		addParam(ParamWidget::create<LEDButton>(Vec(10,53), module, PhraseTrigger::RESET_PARAM, 0.0, 1.0, 0.0));//arm button
		addChild(ModuleLightWidget::create<MediumLight<RedLight>>(Vec(14,57),module, PhraseTrigger::RESET_LIGHT));


		//Adds graphics to module. Takes values from PhaseTrigger and passes to displayWidgets
		{
		 	TimeDisplayWidget *beatDisplay = new TimeDisplayWidget();
			beatDisplay->beat = (&module->beatDisplay);
			beatDisplay->bar = &module->barDisplay;
			beatDisplay->phrase = &module->phraseDisplay;
			addChild(beatDisplay);
		}

		//For LED buttons, child Light must be x+4, y+4 to be centered
		static const  float portX[4] = {20, 140, 260};
		int count = 0;
		static const float row1Y = 100;
		for(int i = 0; i < NUM_ARM_MODULUES/NUM_ROWS; i++){
			// left button
			addParam(ParamWidget::create<LEDButton>(Vec(portX[i],row1Y + 20), module, PhraseTrigger::ARM_BAR + i,0.0, 1.0, 0.0));//select to trigger on bar
			addChild(ModuleLightWidget::create<MediumLight<GreenLight>>(Vec(portX[i]+4.0f,row1Y+24),module, PhraseTrigger::ARM_BAR_LIGHTS + i));
			//top button
			addParam(ParamWidget::create<LEDButton>(Vec(portX[i]+26,row1Y), module, PhraseTrigger::ARM_PARAM + i, 0.0, 1.0, 0.0));//arm button
			addChild(ModuleLightWidget::create<MediumLight<RedLight>>(Vec(portX[i]+30.0f, row1Y + 4.0f), module, PhraseTrigger::ARM_LIGHT + i));//arm button light
			
			addInput(Port::create<PJ301MPort>(Vec((portX[i] + 23), row1Y + 24), Port::INPUT, module, PhraseTrigger::ARM_INPUT + i));// takes input to arm module (useful for MIDI)
			//right button
			addParam(ParamWidget::create<LEDButton>(Vec(portX[i] + 52, row1Y + 20), module, PhraseTrigger::ARM_PHRASE + i,0.0, 1.0, 0.0));//select to trigger on phrase
			addChild(ModuleLightWidget::create<MediumLight<BlueLight>>(Vec(portX[i]+56.0f,row1Y+24),module, PhraseTrigger::ARM_PHRASE_LIGHTS + i));
			addInput(Port::create<PJ301MPort>(Vec(portX[i] - 2, row1Y + 52), Port::INPUT, module, PhraseTrigger::CLOCK_INPUTS + i));
			addOutput(Port::create<PJ301MPort>(Vec(portX[i] + 23, row1Y + 52), Port::OUTPUT, module, PhraseTrigger::TRIGGER_OUTPUT + i));
			count++;
		}
		static const float row2Y = 190;
		for(int i = 0; i < NUM_ARM_MODULUES/NUM_ROWS; i++){
			// left button
			addParam(ParamWidget::create<LEDButton>(Vec(portX[i],row2Y + 20), module, PhraseTrigger::ARM_BAR + count,0.0, 1.0, 0.0));//select to trigger on bar
			addChild(ModuleLightWidget::create<MediumLight<GreenLight>>(Vec(portX[i]+4.0f,row2Y+24),module, PhraseTrigger::ARM_BAR_LIGHTS + count));
			//top button
			addParam(ParamWidget::create<LEDButton>(Vec(portX[i]+26,row2Y), module, PhraseTrigger::ARM_PARAM + count, 0.0, 1.0, 0.0));//arm button
			addChild(ModuleLightWidget::create<MediumLight<RedLight>>(Vec(portX[i]+30.0f, row2Y + 4.0f), module, PhraseTrigger::ARM_LIGHT + count));//arm button light
			addInput(Port::create<PJ301MPort>(Vec((portX[i] + 23), row2Y + 24), Port::INPUT, module, PhraseTrigger::ARM_INPUT + count));// takes input to arm module (useful for MIDI)
			//right button
			addParam(ParamWidget::create<LEDButton>(Vec(portX[i] + 52, row2Y + 20), module, PhraseTrigger::ARM_PHRASE + count,0.0, 1.0, 0.0));//select to trigger on phrase
			addChild(ModuleLightWidget::create<MediumLight<BlueLight>>(Vec(portX[i]+56.0f,row2Y+24),module, PhraseTrigger::ARM_PHRASE_LIGHTS + count));
			addInput(Port::create<PJ301MPort>(Vec(portX[i] - 2, row2Y + 52), Port::INPUT, module, PhraseTrigger::CLOCK_INPUTS + count));
			addOutput(Port::create<PJ301MPort>(Vec(portX[i] + 23, row2Y + 52), Port::OUTPUT, module, PhraseTrigger::TRIGGER_OUTPUT + count));
			count++;
		}
		static const float row3Y = 280;
		for(int i = 0; i < NUM_ARM_MODULUES/NUM_ROWS; i++){
			// left button
			addParam(ParamWidget::create<LEDButton>(Vec(portX[i],row3Y + 20), module, PhraseTrigger::ARM_BAR + count,0.0, 1.0, 0.0));//select to trigger on bar
			addChild(ModuleLightWidget::create<MediumLight<GreenLight>>(Vec(portX[i]+4.0f,row3Y+24),module, PhraseTrigger::ARM_BAR_LIGHTS + count));
			//top button
			addParam(ParamWidget::create<LEDButton>(Vec(portX[i]+26,row3Y), module, PhraseTrigger::ARM_PARAM +count, 0.0, 1.0, 0.0));//arm button
			addChild(ModuleLightWidget::create<MediumLight<RedLight>>(Vec(portX[i]+30.0f, row3Y + 4.0f), module, PhraseTrigger::ARM_LIGHT + count));//arm button light
			addInput(Port::create<PJ301MPort>(Vec((portX[i] + 23), row3Y + 24), Port::INPUT, module, PhraseTrigger::ARM_INPUT + count));// takes input to arm module (useful for MIDI)
			//right button
			addParam(ParamWidget::create<LEDButton>(Vec(portX[i] + 52, row3Y + 20), module, PhraseTrigger::ARM_PHRASE + count,0.0, 1.0, 0.0));//select to trigger on phrase
			addChild(ModuleLightWidget::create<MediumLight<BlueLight>>(Vec(portX[i]+56.0f,row3Y+24),module, PhraseTrigger::ARM_PHRASE_LIGHTS + count));
			addInput(Port::create<PJ301MPort>(Vec(portX[i] - 2, row3Y + 52), Port::INPUT, module, PhraseTrigger::CLOCK_INPUTS + count));
			addOutput(Port::create<PJ301MPort>(Vec(portX[i] + 23, row3Y + 52), Port::OUTPUT, module, PhraseTrigger::TRIGGER_OUTPUT + count));
			count++;
		}
	}
};


// Specify the Module and ModuleWidget subclass, human-readable
// author name for categorization per plugin, module slug (should never
// change), human-readable module name, and any number of tags
// (found in `include/tags.hpp`) separated by commas.
Model *modelPhraseTrigger = Model::create<PhraseTrigger, PhraseTriggerWidget>("Unforgettable Luncheon", "PhraseTrigger", "Phrase Trigger", SEQUENCER_TAG);