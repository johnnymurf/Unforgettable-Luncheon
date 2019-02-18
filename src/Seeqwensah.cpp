/*
* Purpose of this module is to allow for timed triggers based on musical timings
* Musical timings will be based on Phrase, Bar, Beat, and One-Shot
*
*/
//test
#include "UL.hpp"
#include "dsp/digital.hpp"



const int NUM_COMPONENTS = 9;
const int NUM_ROWS = 3;
struct Seeqwensah : Module {
	enum ParamIds {
		RESET_PARAM,
		ENUMS(ARM_PARAM, NUM_COMPONENTS),
		ENUMS(ARM_BAR, NUM_COMPONENTS),
		ENUMS(ARM_PHRASE, NUM_COMPONENTS),
//TO DO		RESET_PARAM,
		NUM_PARAMS
	 };
	enum InputIds {
		MASTER_CLOCK,
		RESET_INPUT,
		ENUMS(ARM_INPUT, NUM_COMPONENTS),
		ENUMS(COMPONENT_CLOCKS, NUM_COMPONENTS),
		NUM_INPUTS
	};
	enum OutputIds {
		ENUMS(TRIGGER_OUTPUT, NUM_COMPONENTS),
		NUM_OUTPUTS
	};
	enum LightIds {
		RESET_LIGHT,
		ENUMS(ARM_LIGHT, NUM_COMPONENTS),
		ENUMS(ARM_BAR_LIGHTS, NUM_COMPONENTS),
		ENUMS(ARM_PHRASE_LIGHTS, NUM_COMPONENTS),
		NUM_LIGHTS
	};


	Seeqwensah() : Module(NUM_PARAMS,NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {}
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

	struct Component{
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

Component components [NUM_COMPONENTS];

void setBar(int i){
	components[i].hasChosenBar = true;
	components[i].hasChosenPhrase = false;
}

void setPhrase(int i){
	components[i].hasChosenBar = false;
	components[i].hasChosenPhrase = true;
}

void armModule(int i){
	components[i].isArmed = true;
	components[i].armButton = 0.0f;
	components[i].armInput = 0.0f;
}

void disarmModule(int i){
	components[i].isArmed = false;
	components[i].armButton = 0.0f;
	components[i].armInput = 0.0f;
}

void outputTriggerEngage(int i){				
	components[i].openTrigger = !components[i].openTrigger;
	components[i].isArmed = false;	 
}

void resetModule(){
	beatCount = 1; 
	barCount = 1;
	phraseCount = 1;
	beatDisplay = beatCount - 1;
	barDisplay = barCount;
	phraseDisplay = phraseCount;
	for(int i = 0; i < NUM_COMPONENTS; i++){
		components[i].openTrigger = false;
		components[i].isArmed = false;
	}
}

void incrementBeat(){
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

};

void Seeqwensah::step() {
		//deltaTime = engineGetSampleTime();
		//True if input to clock is high (receiving input from clock source) 			
		isBeat = clockTrigger.process(inputs[MASTER_CLOCK].value);


		
		// Loop through all components 
		for(int i = 0; i < NUM_COMPONENTS; i++){
			// check if user has armed a component 
			components[i].armButton = components[i].armButtonTrigger.process(params[ARM_PARAM+i].value);
			components[i].armInput = components[i].armInputTrigger.process(inputs[ARM_INPUT+i].value);

			// User selects to output on bar or phrase
			if(components[i].armBar.process(params[ARM_BAR+i].value)){
				setBar(i);
			}
			if(components[i].armPhrase.process(params[ARM_PHRASE+i].value)){
				setPhrase(i);
			}

			// Arm or Disarm the component 
			if((components[i].armButton || components[i].armInput) && !components[i].isArmed){
				armModule(i);
			}
			if((components[i].armButton || components[i].armInput) && components[i].isArmed){
				disarmModule(i);
			}

			// if armed and user selected Phrase
			if((barCount == 1 && beatCount == 1) &&  isBeat && components[i].hasChosenPhrase && components[i].isArmed){
				outputTriggerEngage(i);
			 }
			//  if armed and user selected Bar
			if((beatCount == 1) && isBeat && components[i].hasChosenBar && components[i].isArmed){
				outputTriggerEngage(i);
			}
			

			if(components[i].openTrigger){
				if(components[i].clockTrigger.process(inputs[COMPONENT_CLOCKS+ i].value)){
					outputs[TRIGGER_OUTPUT+i].value = 10.0f;
				}
				else{
					outputs[TRIGGER_OUTPUT+i].value = 0.0f;
				}
			}
		    // outputs - will pulse if on the beat or show light if it is armed and if its set to output on bar or phrase
			lights[ARM_LIGHT + i].value = components[i].isArmed;
			lights[ARM_BAR_LIGHTS + i].value = components[i].hasChosenBar;
			lights[ARM_PHRASE_LIGHTS + i].value = components[i].hasChosenPhrase;
		}
		



		if( resetTrigger.process(inputs[RESET_INPUT].value)  || resetButton.process(params[RESET_PARAM].value) ){
			resetModule();
		}
		//Incremement beat counters;
		if(isBeat && !resetButton.process(params[RESET_PARAM].value) ){
			incrementBeat();
		}


		lights[RESET_LIGHT].setBrightnessSmooth(resetTrigger.isHigh());
		lights[RESET_LIGHT].setBrightnessSmooth(resetButton.isHigh());

	};
// Used to Display Phrase/Bar/Beat Number to the User
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



struct SeeqwensahWidget : ModuleWidget {
	SeeqwensahWidget(Seeqwensah *module) : ModuleWidget(module) {
		setPanel(SVG::load(assetPlugin(plugin, "res/Seeqwensah.svg")));

		addChild(Widget::create<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(Widget::create<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(Widget::create<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(Widget::create<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		//Used for CLock input 
		addInput(Port::create<PJ301MPort>(Vec(33, 20), Port::INPUT, module, Seeqwensah::MASTER_CLOCK));
		addInput(Port::create<PJ301MPort>(Vec(33, 50), Port::INPUT, module, Seeqwensah::RESET_INPUT));
		addParam(ParamWidget::create<LEDButton>(Vec(10,53), module, Seeqwensah::RESET_PARAM, 0.0, 1.0, 0.0));//arm button
		addChild(ModuleLightWidget::create<MediumLight<RedLight>>(Vec(14,57),module, Seeqwensah::RESET_LIGHT));


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
		for(int i = 0; i < NUM_COMPONENTS/NUM_ROWS; i++){
			// left button
			addParam(ParamWidget::create<LEDButton>(Vec(portX[i],row1Y + 20), module, Seeqwensah::ARM_BAR + i,0.0, 1.0, 0.0));//select to trigger on bar
			addChild(ModuleLightWidget::create<MediumLight<GreenLight>>(Vec(portX[i]+4.0f,row1Y+24),module, Seeqwensah::ARM_BAR_LIGHTS + i));
			//top button
			addParam(ParamWidget::create<LEDButton>(Vec(portX[i]+26,row1Y), module, Seeqwensah::ARM_PARAM + i, 0.0, 1.0, 0.0));//arm button
			addChild(ModuleLightWidget::create<MediumLight<RedLight>>(Vec(portX[i]+30.0f, row1Y + 4.0f), module, Seeqwensah::ARM_LIGHT + i));//arm button light
			
			addInput(Port::create<PJ301MPort>(Vec((portX[i] + 23), row1Y + 24), Port::INPUT, module, Seeqwensah::ARM_INPUT + i));// takes input to arm module (useful for MIDI)
			//right button
			addParam(ParamWidget::create<LEDButton>(Vec(portX[i] + 52, row1Y + 20), module, Seeqwensah::ARM_PHRASE + i,0.0, 1.0, 0.0));//select to trigger on phrase
			addChild(ModuleLightWidget::create<MediumLight<BlueLight>>(Vec(portX[i]+56.0f,row1Y+24),module, Seeqwensah::ARM_PHRASE_LIGHTS + i));
			addInput(Port::create<PJ301MPort>(Vec(portX[i] - 2, row1Y + 52), Port::INPUT, module, Seeqwensah::COMPONENT_CLOCKS + i));
			addOutput(Port::create<PJ301MPort>(Vec(portX[i] + 23, row1Y + 52), Port::OUTPUT, module, Seeqwensah::TRIGGER_OUTPUT + i));
			count++;
		}
		static const float row2Y = 190;
		for(int i = 0; i < NUM_COMPONENTS/NUM_ROWS; i++){
			// left button
			addParam(ParamWidget::create<LEDButton>(Vec(portX[i],row2Y + 20), module, Seeqwensah::ARM_BAR + count,0.0, 1.0, 0.0));//select to trigger on bar
			addChild(ModuleLightWidget::create<MediumLight<GreenLight>>(Vec(portX[i]+4.0f,row2Y+24),module, Seeqwensah::ARM_BAR_LIGHTS + count));
			//top button
			addParam(ParamWidget::create<LEDButton>(Vec(portX[i]+26,row2Y), module, Seeqwensah::ARM_PARAM + count, 0.0, 1.0, 0.0));//arm button
			addChild(ModuleLightWidget::create<MediumLight<RedLight>>(Vec(portX[i]+30.0f, row2Y + 4.0f), module, Seeqwensah::ARM_LIGHT + count));//arm button light
			addInput(Port::create<PJ301MPort>(Vec((portX[i] + 23), row2Y + 24), Port::INPUT, module, Seeqwensah::ARM_INPUT + count));// takes input to arm module (useful for MIDI)
			//right button
			addParam(ParamWidget::create<LEDButton>(Vec(portX[i] + 52, row2Y + 20), module, Seeqwensah::ARM_PHRASE + count,0.0, 1.0, 0.0));//select to trigger on phrase
			addChild(ModuleLightWidget::create<MediumLight<BlueLight>>(Vec(portX[i]+56.0f,row2Y+24),module, Seeqwensah::ARM_PHRASE_LIGHTS + count));
			addInput(Port::create<PJ301MPort>(Vec(portX[i] - 2, row2Y + 52), Port::INPUT, module, Seeqwensah::COMPONENT_CLOCKS + count));
			addOutput(Port::create<PJ301MPort>(Vec(portX[i] + 23, row2Y + 52), Port::OUTPUT, module, Seeqwensah::TRIGGER_OUTPUT + count));
			count++;
		}
		static const float row3Y = 280;
		for(int i = 0; i < NUM_COMPONENTS/NUM_ROWS; i++){
			// left button
			addParam(ParamWidget::create<LEDButton>(Vec(portX[i],row3Y + 20), module, Seeqwensah::ARM_BAR + count,0.0, 1.0, 0.0));//select to trigger on bar
			addChild(ModuleLightWidget::create<MediumLight<GreenLight>>(Vec(portX[i]+4.0f,row3Y+24),module, Seeqwensah::ARM_BAR_LIGHTS + count));
			//top button
			addParam(ParamWidget::create<LEDButton>(Vec(portX[i]+26,row3Y), module, Seeqwensah::ARM_PARAM +count, 0.0, 1.0, 0.0));//arm button
			addChild(ModuleLightWidget::create<MediumLight<RedLight>>(Vec(portX[i]+30.0f, row3Y + 4.0f), module, Seeqwensah::ARM_LIGHT + count));//arm button light
			addInput(Port::create<PJ301MPort>(Vec((portX[i] + 23), row3Y + 24), Port::INPUT, module, Seeqwensah::ARM_INPUT + count));// takes input to arm module (useful for MIDI)
			//right button
			addParam(ParamWidget::create<LEDButton>(Vec(portX[i] + 52, row3Y + 20), module, Seeqwensah::ARM_PHRASE + count,0.0, 1.0, 0.0));//select to trigger on phrase
			addChild(ModuleLightWidget::create<MediumLight<BlueLight>>(Vec(portX[i]+56.0f,row3Y+24),module, Seeqwensah::ARM_PHRASE_LIGHTS + count));
			addInput(Port::create<PJ301MPort>(Vec(portX[i] - 2, row3Y + 52), Port::INPUT, module, Seeqwensah::COMPONENT_CLOCKS + count));
			addOutput(Port::create<PJ301MPort>(Vec(portX[i] + 23, row3Y + 52), Port::OUTPUT, module, Seeqwensah::TRIGGER_OUTPUT + count));
			count++;
		}
	}
};


// Specify the Module and ModuleWidget subclass, human-readable
// author name for categorization per plugin, module slug (should never
// change), human-readable module name, and any number of tags
// (found in `include/tags.hpp`) separated by commas.
Model *modelSeeqwensah = Model::create<Seeqwensah, SeeqwensahWidget>("Unforgettable Luncheon", "Seeqwensah", "Seeqwensah", SEQUENCER_TAG);