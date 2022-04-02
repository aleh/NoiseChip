# NoiseChip

Turns an ATtiny85 into a "noise chip" suitable for an analogue drum machine such as 
[LDB-1](http://mickeydelp.com/blog/anatomy-of-a-drum-machine).

One of the pins outputs digital "white" noise using a linear feedback shift register, 
4 other pins output different square waves out of tune with each other.

The noise is used by the snare drum, clap and hi-hat modules, while sqaure waves are 
used to add a bit of metallic tembre for the hi-hat.

Note that the frequencies of the square waves are not described in the article, 
but there is a reference to a schematics of Boss DR-110 where each of these square 
wave modules is made using 2 inverters with a 4.7nF capacitor being charged via 
82K, 120K, 330K, and 220K resistors. My crude calculation shows that the corresponding 
frequencies are: 1049Hz, 717Hz, 261Hz, 392Hz. 

Measuring the frequncies out of the ATTINY85 you get 
Pin 0: 960 Hz
Pin 1: 673.5 Hz
Pin 2: 248 Hz
Pin 4: 369 Hz
Pin 5: Noise 
