# Unforgettable Lucheon

Modules for use with [VCV Rack](https://vcvrack.com/)

## Seeqwensah:

The idea of the plugin is to sequence other sequencers, triggering them on the 1st beat of a bar or phrase. 

Similar in theory to how some DAWs launch clips.

Users don't have to worry about being perfectly in time, as it releases on the first beat of the next bar/phrase once its armed. 

On default it's 8 bars to a phrase and 4 beats to a bar. This can be altered by the user.


Useful in a live setting or just when you want to jam out.

Please take a moment to read the manual.


* Master Clock - This takes the beat of the master clock.

* Reset - This resets the beat count and all componenets. 

* Run - Turns the beat count on or off. 

### Seeqwensah has 8 components 

* Each component as small a red light. It indicates if its on or off (outputting a signal or not). 

* Has editable text. Useful for keeping track of things.

* The arm button can be pressed. Will light if red. Will also light if armed via arm input. 

* Two buttons (green and blue) indicate if the component will emit on the bar(green) or phrase(blue).

* Arm input takes an in (probably MIDI) to arm the component.

* Clock Input takes whatever clock pulse is sent into it. Clock Output emits it if component is on.

* Trigger Out emits a pulse on the 1st beat of being turned on and last beat of turned off. 
    This is useful for reseting sequences to keep them in time. Also useful if you just want 
    to use it to play a sequencer that doesn't require a clock input.


# Bugs / weird behaviour
Some sequencers don't reset to the start of their sequence missing the first beat, which can lead to annoying behaviour.
Piano roll from RCM and Impromptu Modular's sequencers seem to work fine. 

On restarting rack and reloading a patch things can get weird.
This usually can be fixed by arming and disarming all components. 
rom experience things tend to stay in time once thats done. 