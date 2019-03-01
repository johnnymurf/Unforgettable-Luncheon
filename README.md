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
    *Current bug means a reset is sent out on number 00:00:00.


# Bugs / weird behaviour
Some sequencers don't reset to the start of their sequence missing the first beat, which can lead to annoying behaviour.
Piano roll from RCM, Impromptu Modular's, and Cf's sequencers seem to work fine. 

On starting the module things can get a small bit weird. 
Trigger out buttons are intended for reseting on the start clock out to sequencers, so they should be sent to a sequencer reset input.
Annoyingly a reset triggers on 00:00:00 as well as on 00:00:01. This is actually fine for must sequencers but for those that don't
take a clock (those that just play), they'll be turned on and then off. The temporary solution to this is
just wait until the beginning of the next phrase before arming. This will be fixed soon, but requires a bit of a refactoring of code.