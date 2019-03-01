/*
* Purpose of this module is to allow for timed triggers based on musical timings
* Musical timings will be based on Phrase, Bar, Beat.
*
*/

#include "UL.hpp"
#include "dsp/digital.hpp"
#include "rack.hpp"


//Module works best with plugins which don't play first note on reset. 



struct Seeqwensah : Module {
const int MAX_PHRASE = 999;
const int MAX_BAR = 99;
const int MAX_BEAT = 99; 
TextField* textField [8];

	enum ParamIds {
		RUNNING_PARAM,
		RESET_PARAM,
		PHRASE_LENGTH_UP_PARAM,
		PHRASE_LENGTH_DOWN_PARAM,
		BAR_LENGTH_UP_PARAM,
		BAR_LENGTH_DOWN_PARAM,
		ENUMS(ARM_PARAM, 8),
		ENUMS(ARM_BAR, 8),
		ENUMS(ARM_PHRASE, 8),
		NUM_PARAMS
	 };
	enum InputIds {
		MASTER_CLOCK,
		RUN_INPUT,
		RESET_INPUT,
		ENUMS(ARM_INPUT, 8),
		ENUMS(CLOCKS_IN, 8),
		NUM_INPUTS
	};
	enum OutputIds {
		ENUMS(CLOCKS_OUT, 8),
		ENUMS(RESETS_OUT, 8),
		NUM_OUTPUTS
	};
	enum LightIds {
		RUN_LIGHT,
		RESET_LIGHT,
		ENUMS(ARM_LIGHT, 8),
		ENUMS(ARM_BAR_LIGHTS, 8),
		ENUMS(ARM_PHRASE_LIGHTS, 8),
		ENUMS(COMPONENT_ACTIVE_LIGHTS,8),
		NUM_LIGHTS
	};


	Seeqwensah() : Module(NUM_PARAMS,NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {}
	void step() override;


	//used for master clock input
	SchmittTrigger clockTrigger;
	// resetTrigger used for CV input, resetButton used for mouse presses
	SchmittTrigger resetTrigger;
	SchmittTrigger resetButton;
	SchmittTrigger runButton;
	SchmittTrigger runTrigger;

	SchmittTrigger barLengthUpButton;
	SchmittTrigger barLengthDownButton;
	SchmittTrigger phraseLengthUpButton;
	SchmittTrigger phraseLengthDownButton;


	
    bool isBeat = false ; //will be true for every clock pulse input
	bool isReset = false;

	bool isRunning = false;

	bool veryFirst = true; //used to skip first beat, matches sequencers that skip the first clock cycle
	// Counts will be used for internal logic, displays will be for graphics
	int beatCount = 1;
	int beatDisplay = 0;
	int barCount = 1; 
	int barDisplay = 1; 
	int phraseCount = 1;
	int phraseDisplay = 1;
	int beatsPerBar = 4; //default
	int barsPerPhrase = 8; //default
	float deltaTime = 0;
	int totalBeats = 0;



	struct Component{
		SchmittTrigger armButtonTrigger;
		SchmittTrigger armInputTrigger;
		SchmittTrigger clockTrigger;
		SchmittTrigger armBar;
		SchmittTrigger armPhrase;
		SchmittTrigger resetOutOnOff;

		PulseGenerator pulseOut;
		PulseGenerator resetPulseOut;


		bool isArmed = false;
		bool armButton = false;
		bool armInput = false;
		bool hasChosenBar = true; //default will be trigger on bar  
		bool hasChosenPhrase = false;
		bool openTrigger = false;
		int count = 0;
			
	};
	
	Component components [8]; 
	



	json_t *toJson() override {
		json_t *rootJ = json_object();
		
		json_object_set_new(rootJ, "beatsPerBar",json_integer(beatsPerBar));
		json_object_set_new(rootJ, "barsPerPhrase",json_integer(barsPerPhrase));

		//Save Texts
		
			json_object_set_new(rootJ, "text0", json_string(textField[0]->text.c_str()));
			json_object_set_new(rootJ, "text1", json_string(textField[1]->text.c_str()));
			json_object_set_new(rootJ, "text2", json_string(textField[2]->text.c_str()));
			json_object_set_new(rootJ, "text3", json_string(textField[3]->text.c_str()));
			json_object_set_new(rootJ, "text4", json_string(textField[4]->text.c_str()));
			json_object_set_new(rootJ, "text5", json_string(textField[5]->text.c_str()));
			json_object_set_new(rootJ, "text6", json_string(textField[6]->text.c_str()));
			json_object_set_new(rootJ, "text7", json_string(textField[7]->text.c_str()));
			
			return rootJ;
	}

void fromJson(json_t *rootJ) override {

		json_t *beatsPerBarJ = json_object_get(rootJ,"beatsPerBar");
		if(beatsPerBarJ){
			beatsPerBar = json_integer_value(beatsPerBarJ);
		}

		json_t *barsPerPhraseJ = json_object_get(rootJ,"barsPerPhrase");
		if(barsPerPhraseJ){
			barsPerPhrase = json_integer_value(barsPerPhraseJ);
		}
// wish i could make this more elegant
		json_t *textJ0 = json_object_get(rootJ, "text0");
		textField[0]->text = json_string_value(textJ0);
		json_t *textJ1 = json_object_get(rootJ, "text1");
		textField[1]->text = json_string_value(textJ1);
		json_t *textJ2 = json_object_get(rootJ, "text2");
		textField[2]->text = json_string_value(textJ2);
		json_t *textJ3 = json_object_get(rootJ, "text3");
		textField[3]->text = json_string_value(textJ3);
		json_t *textJ4 = json_object_get(rootJ, "text4");
		textField[4]->text = json_string_value(textJ4);
		json_t *textJ5 = json_object_get(rootJ, "text5");
		textField[5]->text = json_string_value(textJ5);
		json_t *textJ6 = json_object_get(rootJ, "text6");
		textField[6]->text = json_string_value(textJ6);
		json_t *textJ7 = json_object_get(rootJ, "text7");
		textField[7]->text = json_string_value(textJ7);

	}
	


	void onReset() override{
		deltaTime = engineGetSampleTime();

		resetModule();
		for(int i = 0 ; i < 8; i++){
			textField[i]->text = "";
		}

	}




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
		components[i].pulseOut.trigger(2e-3f); ///*********************

	}
	
	void resetModule(){
		isRunning = false;
		lights[RUN_LIGHT].value = 0.0f;
		isRunning = false;
		beatCount = 1; 
		barCount = 1;
		phraseCount = 1;
		beatDisplay = beatCount - 1;
		barDisplay = barCount;
		phraseDisplay = phraseCount;
		for(int i = 0; i < 8; i++){
			components[i].openTrigger = false;
			components[i].isArmed = false;
			lights[COMPONENT_ACTIVE_LIGHTS+i].value = 0.0f;
		
		}
	}
	
	void incrementBeat(){
		if(veryFirst){
			printf("test\n");
		}

		if(!veryFirst){ // skip clock

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
				if(phraseCount > MAX_PHRASE){
					phraseCount = 1;
				}
			}
		}
		veryFirst = false;
	}

};



void Seeqwensah::step() {

	deltaTime = engineGetSampleTime();
	
	//True if input to clock is high (receiving input from clock source) 			
	isBeat = clockTrigger.process(inputs[MASTER_CLOCK].value);



	for(int i = 0; i < 8 ; i++){
		

		// User selects to output on bar or phrase
		if(components[i].armBar.process(params[ARM_BAR+i].value)){
			setBar(i);
		}
		if(components[i].armPhrase.process(params[ARM_PHRASE+i].value)){
			setPhrase(i);	
		}
		// check if user has armed a component 
		components[i].armButton = components[i].armButtonTrigger.process(params[ARM_PARAM+i].value);
		components[i].armInput = components[i].armInputTrigger.process(inputs[ARM_INPUT+i].value);	
		// Arm or Disarm the component 
		if((components[i].armButton || components[i].armInput) && !components[i].isArmed){
			armModule(i);
		}
		if((components[i].armButton || components[i].armInput) && components[i].isArmed){
			disarmModule(i);
		}

		lights[ARM_LIGHT + i].value = components[i].isArmed;
		lights[ARM_BAR_LIGHTS + i].value = components[i].hasChosenBar;
		lights[ARM_PHRASE_LIGHTS + i].value = components[i].hasChosenPhrase;
	}

	

	if(isRunning){
		// Loop through all components 
		for(int i = 0; i < 8; i++){
	
			if((barCount == 1 && beatCount == 1) &&  isBeat && components[i].hasChosenPhrase && components[i].isArmed){
				outputTriggerEngage(i);
				components[i].resetPulseOut.trigger(1e-3f); 
				
			 }
		
			if((beatCount == 1) && isBeat && components[i].hasChosenBar && components[i].isArmed){
				outputTriggerEngage(i);
				components[i].resetPulseOut.trigger(1e-3f); 

			}


			if(components[i].openTrigger){
				if(inputs[CLOCKS_IN + i].value){
					components[i].pulseOut.trigger(1e-3f);
				}

				if(components[i].pulseOut.process(deltaTime) && !veryFirst){
				//	printf("Test\n\n");
			
					outputs[CLOCKS_OUT+i].value = 10.0f;
				}
				else{
					outputs[CLOCKS_OUT+i].value = 0.0f;
				}
			}

			// Send reset pulse out if compenent begins/ends clock output
			if(components[i].resetPulseOut.process(deltaTime)){
				outputs[RESETS_OUT+i].value = 10.0f;
			}
			if(!components[i].resetPulseOut.process(deltaTime)){
				outputs[RESETS_OUT+i].value = 0.0f;
			}

			lights[COMPONENT_ACTIVE_LIGHTS+i].value = components[i].openTrigger;
		}

		//Incremement beat counters;
		if(isBeat && !resetButton.process(params[RESET_PARAM].value) ){
			incrementBeat();
		}

	}	
	//user runs
	if(runButton.process(params[RUNNING_PARAM].value) || runTrigger.process(inputs[RUN_INPUT].value)){
		isRunning = !isRunning;
		if(isRunning){
			lights[RUN_LIGHT].value = 10.0f;
		}
		else{
			lights[RUN_LIGHT].value = 0.0f;
		}
	}

	//user resets
	if( resetTrigger.process(inputs[RESET_INPUT].value)  || resetButton.process(params[RESET_PARAM].value) ){
			resetModule();
	}
	lights[RESET_LIGHT].setBrightnessSmooth(resetTrigger.isHigh());
	lights[RESET_LIGHT].setBrightnessSmooth(resetButton.isHigh());


	//Check if user change length of bar/phrase
	if(barLengthUpButton.process(params[BAR_LENGTH_UP_PARAM].value)){
		beatsPerBar++;
	}
	if(barLengthDownButton.process(params[BAR_LENGTH_DOWN_PARAM].value)){
		beatsPerBar--;
		if(beatsPerBar < 1){
			beatsPerBar = 1;
		}
	}
	if(phraseLengthUpButton.process(params[PHRASE_LENGTH_UP_PARAM].value) ){
		barsPerPhrase++;
	}
	if(phraseLengthDownButton.process(params[PHRASE_LENGTH_DOWN_PARAM].value)){
		barsPerPhrase--;
		if(barsPerPhrase < 1) {
			barsPerPhrase = 1;
		}
	}
};


// sed to Display Phrase/Bar/Beat Number to the User
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
			Vec textPos = Vec(180,43);
			nvgFillColor(vg, nvgRGBA(0xff, 0x18, 0x00, 0xff));
			char text[250];
			snprintf(text, sizeof(text), "%03u  :  %02u  :  %02u",((unsigned) *phrase),((unsigned) *bar),((unsigned) *beat));
			nvgText(vg, textPos.x, textPos.y, text, NULL);
		}
};
struct BarLengthDisplayWidget : TransparentWidget{
	int *beatsPerBar;
	std::shared_ptr<Font> font;

	BarLengthDisplayWidget(){
		font = Font::load(assetPlugin(plugin,"res/DSEG14Classic-Italic.ttf"));
		}
		void draw(NVGcontext *vg) override{
			nvgFontSize(vg,20);
			nvgFontFaceId(vg, font->handle);
			nvgTextLetterSpacing(vg, 2);
			Vec textPos = Vec(485,43);
			nvgFillColor(vg, nvgRGBA(0xff, 0x18, 0x00, 0xff));
			char text[250];
			snprintf(text, sizeof(text),"%02u",((unsigned) *beatsPerBar));
			nvgText(vg, textPos.x, textPos.y, text, NULL);
		}
};
struct PhraseLengthDisplayWidget : TransparentWidget{
	int *barsPerPhrase;
	std::shared_ptr<Font> font;

	PhraseLengthDisplayWidget(){
		font = Font::load(assetPlugin(plugin,"res/DSEG14Classic-Italic.ttf"));
		}
		void draw(NVGcontext *vg) override{
			nvgFontSize(vg,20);
			nvgFontFaceId(vg, font->handle);
			nvgTextLetterSpacing(vg, 2);
			Vec textPos = Vec(410,43);
			nvgFillColor(vg, nvgRGBA(0xff, 0x18, 0x00, 0xff));
			char text[250];
			snprintf(text, sizeof(text),"%02u",((unsigned) *barsPerPhrase));
			nvgText(vg, textPos.x, textPos.y, text, NULL);
		}
};




struct SeeqwensahWidget : ModuleWidget {
	SeeqwensahWidget(Seeqwensah *module) : ModuleWidget(module) {
		setPanel(SVG::load(assetPlugin(plugin, "res/SeeqwensahGift.svg")));

		addChild(Widget::create<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(Widget::create<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(Widget::create<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(Widget::create<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		//Used for CLock input 
		addInput(Port::create<PJ301MPort>(Vec(15, 40), Port::INPUT, module, Seeqwensah::MASTER_CLOCK));
		

		//Run
		addParam(ParamWidget::create<LEDBezel>(Vec(120,12), module, Seeqwensah::RUNNING_PARAM, 0.0, 1.0, 0.0));
		addChild(ModuleLightWidget::create<LEDBezelLight<GreenLight>>(Vec(122,14), module, Seeqwensah::RUN_LIGHT));
		addInput(Port::create<PJ301MPort>(Vec(119, 40), Port::INPUT, module, Seeqwensah::RUN_INPUT));

		//Reset
		addParam(ParamWidget::create<LEDButton>(Vec(54,14), module, Seeqwensah::RESET_PARAM, 0.0, 1.0, 0.0));
		addChild(ModuleLightWidget::create<MediumLight<RedLight>>(Vec(58,18),module, Seeqwensah::RESET_LIGHT));
		addInput(Port::create<PJ301MPort>(Vec(51, 40), Port::INPUT, module, Seeqwensah::RESET_INPUT));

		//User selects Phrase / bar length 
		addParam(ParamWidget::create<ULSmallButton>(Vec(390,20),module,Seeqwensah::PHRASE_LENGTH_UP_PARAM, 0.0,10.0f,0.0));
		addParam(ParamWidget::create<ULSmallButton>(Vec(390,35),module,Seeqwensah::PHRASE_LENGTH_DOWN_PARAM, 0.0,10.0f,0.0));
		addParam(ParamWidget::create<ULSmallButton>(Vec(465,20),module,Seeqwensah::BAR_LENGTH_UP_PARAM, 0.0,10.0f,0.0));
		addParam(ParamWidget::create<ULSmallButton>(Vec(465,35),module,Seeqwensah::BAR_LENGTH_DOWN_PARAM, 0.0,10.0f,0.0));
	

		//Adds graphics to module. Takes values from PhaseTrigger and passes to displayWidgets
		TimeDisplayWidget *timeDisplay = new TimeDisplayWidget();
		timeDisplay->beat = (&module->beatDisplay);
		timeDisplay->bar = &module->barDisplay;
		timeDisplay->phrase = &module->phraseDisplay;
		addChild(timeDisplay);

		//phrase and bar length graphics
		BarLengthDisplayWidget *barDisplay = new BarLengthDisplayWidget();
		barDisplay->beatsPerBar = &module->beatsPerBar;
		addChild(barDisplay);
		PhraseLengthDisplayWidget *phraseDisplay = new PhraseLengthDisplayWidget();
		phraseDisplay->barsPerPhrase = &module->barsPerPhrase;
		addChild(phraseDisplay);

		//Didn't use inner loops rows/columns because that would require enums within enums and that could get messy
		static const  float portX[4] = {55, 175, 295, 415};
		int count = 0;
		static const float row1Y = 120;
		for(int i = 0; i < 4; i++){
			module->textField[i] = Widget::create<LedDisplayTextField>(Vec(portX[i]-1, 87));
    		module->textField[i]->box.size = Vec(72, 30);
    		module->textField[i]->multiline = false;
			addChild(module->textField[i]);
			addChild(ModuleLightWidget::create<SmallLight<RedLight>>(Vec(portX[i]+ 32.0f, row1Y - 45), module,Seeqwensah::COMPONENT_ACTIVE_LIGHTS + i));
			// left button
			addParam(ParamWidget::create<LEDButton>(Vec(portX[i],row1Y + 20), module, Seeqwensah::ARM_BAR + i,0.0, 1.0, 0.0));//select to trigger on bar
			addChild(ModuleLightWidget::create<MediumLight<GreenLight>>(Vec(portX[i]+4.0f,row1Y+24),module, Seeqwensah::ARM_BAR_LIGHTS + i));
			//top button
			addParam(ParamWidget::create<LEDBezel>(Vec(portX[i]+24,row1Y), module, Seeqwensah::ARM_PARAM + i, 0.0, 1.0, 0.0));//arm button
			addChild(ModuleLightWidget::create<LEDBezelLight<RedLight>>(Vec(portX[i]+26.0f, row1Y + 2.0f), module, Seeqwensah::ARM_LIGHT + i));//arm button light
			addInput(Port::create<PJ301MPort>(Vec((portX[i] + 23), row1Y + 24), Port::INPUT, module, Seeqwensah::ARM_INPUT + i));// takes input to arm module (useful for MIDI)
			//right button
			addParam(ParamWidget::create<LEDButton>(Vec(portX[i] + 52, row1Y + 20), module, Seeqwensah::ARM_PHRASE + i,0.0, 1.0, 0.0));//select to trigger on phrase
			addChild(ModuleLightWidget::create<MediumLight<BlueLight>>(Vec(portX[i]+56.0f,row1Y+24),module, Seeqwensah::ARM_PHRASE_LIGHTS + i));
			addInput(Port::create<PJ301MPort>(Vec(portX[i] - 2, row1Y + 52), Port::INPUT, module, Seeqwensah::CLOCKS_IN + i));
			addOutput(Port::create<PJ301MPort>(Vec(portX[i] + 23, row1Y + 52), Port::OUTPUT, module, Seeqwensah::CLOCKS_OUT + i));
			addOutput(Port::create<PJ301MPort>(Vec(portX[i] + 48, row1Y + 52), Port::OUTPUT, module, Seeqwensah::RESETS_OUT + i));
			
			count++;
		}
		static const float row2Y = 270;
		for(int i = 0; i  < 4; i++){
			// left button
			module->textField[count] = Widget::create<LedDisplayTextField>(Vec(portX[i]-1, 237));
    		module->textField[count]->box.size = Vec(72, 30);
    		module->textField[count]->multiline = false;
			addChild(module->textField[count]);

			addChild(ModuleLightWidget::create<SmallLight<RedLight>>(Vec(portX[i]+ 32.0f, row2Y - 45), module,Seeqwensah::COMPONENT_ACTIVE_LIGHTS + count));

			addParam(ParamWidget::create<LEDButton>(Vec(portX[i],row2Y + 20), module, Seeqwensah::ARM_BAR + count,0.0, 1.0, 0.0));//select to trigger on bar
			addChild(ModuleLightWidget::create<MediumLight<GreenLight>>(Vec(portX[i]+4.0f,row2Y+24),module, Seeqwensah::ARM_BAR_LIGHTS + count));
			
			addParam(ParamWidget::create<LEDBezel>(Vec(portX[i]+24,row2Y), module, Seeqwensah::ARM_PARAM + count, 0.0, 1.0, 0.0));//arm button
			addChild(ModuleLightWidget::create<LEDBezelLight<RedLight>>(Vec(portX[i]+26.0f, row2Y + 2.0f), module, Seeqwensah::ARM_LIGHT + count));//arm button light
			addInput(Port::create<PJ301MPort>(Vec((portX[i] + 23), row2Y + 24), Port::INPUT, module, Seeqwensah::ARM_INPUT + count));// takes input to arm module (useful for MIDI)
			//right button
			addParam(ParamWidget::create<LEDButton>(Vec(portX[i] + 52, row2Y + 20), module, Seeqwensah::ARM_PHRASE + count,0.0, 1.0, 0.0));//select to trigger on phrase
			addChild(ModuleLightWidget::create<MediumLight<BlueLight>>(Vec(portX[i]+56.0f,row2Y+24),module, Seeqwensah::ARM_PHRASE_LIGHTS + count));
			addInput(Port::create<PJ301MPort>(Vec(portX[i] - 2, row2Y + 52), Port::INPUT, module, Seeqwensah::CLOCKS_IN + count));
			addOutput(Port::create<PJ301MPort>(Vec(portX[i] + 23, row2Y + 52), Port::OUTPUT, module, Seeqwensah::CLOCKS_OUT + count));
			addOutput(Port::create<PJ301MPort>(Vec(portX[i] + 47, row2Y + 52), Port::OUTPUT, module, Seeqwensah::RESETS_OUT + count));
			count++;
		}
	}
};


// Specify the Module and ModuleWidget subclass, human-readable
// author name for categorization per plugin, module slug (should never
// change), human-readable module name, and any number of tags
// (found in `include/tags.hpp`) separated by commas.
Model *modelSeeqwensah = Model::create<Seeqwensah, SeeqwensahWidget>("Unforgettable Luncheon", "Seeqwensah", "Seeqwensah", SEQUENCER_TAG);