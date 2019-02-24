#include "rack.hpp"


using namespace rack;

// Forward-declare the Plugin, defined in Template.cpp
extern Plugin *plugin;

// Forward-declare each Model, defined in each module source file
extern Model *modelSeeqwensah;


//Custom SVGS

// Maybe in the future....

// struct Small_Knob : SVGKnob {
// 	Small_Knob() {
  
// 		setSVG( SVG::load(assetPlugin(plugin, "res/smallknob.svg")) );
// 	}
//};

struct ULSmallButton : SVGSwitch, MomentarySwitch {
    ULSmallButton(){

        addFrame( SVG::load(assetPlugin(plugin,"res/smallButton.svg")));
    }

};

