PARTY VERSION!

Credits where credits are due:
==============================

The following 3rd party resources were used:

pmf_player - https://github.com/JarkkoPFC/arduino-music-player/
exomizer   - https://bitbucket.org/magli143/exomizer

Colors for the screen border during the C64/Amiga-ish part were sampled from
"A1000" by J.O.E, see http://artcity.bitfellas.org/index.php?a=show&id=11210

3x5 by memesbruh03 - https://www.dafont.com/3x5-2.font
Perfect DOS VGA by Zeh Fernando - https://www.dafont.com/perfect-dos-vga-437.font
A500 Topaz font, TTF file of unknown origin


How to compile the thing
========================

The easiest way is to get Arduino IDE.
See https://www.arduino.cc/en/Main/Software

Load democode.ino in the Arduino IDE, compile it, flash it, done.
Possibly read the comments in democode.ino in case the compiler is too old
or does not like the code and refuses to compile it.

Settings:

Board: "Arduino/Genuino Mega or Mega 2560"
Processor: ATmega2560
(5V 16 MHz if that's a choice)

Note that the code in the source archive is the party version;
it's horrible and hacky and fixed to work just before the deadline,
and it's likely i'll clean it up a bit later.

See https://github.com/fgenesis/hwmf for up-to-date code.


Getting audio out
==================

Audio is output to OCR0A (Pin #13) and OCR0B (Pin #4).
This demo has mono audio so either one works.
Stereo is a future possibility so in case you're building an add-on board
you might plan ahead for that.

Connect it like this:

Pin 4  o-----||-----> left audio channel
Pin 13 o-----||-----> right audio channel
Any GND pin directly to the audio GND.

As insulating capacitors ||, 1.0 µF works fine. Something close to that probably too.
Don't connect the pins directly to the audio, they are at 0-5V level
(instead of approx. -1..+1V that analog audio usually has) and the receiving device
might be unhappy about that (magic smoke worst case). The caps make this a lot safer,
and it also sounds much better as they center the signal around GND.

The on-board LED is also connected to Pin #13 so if you see it pulse a little after a
few seconds after reset, that's an indication it's playing audio correctly.


Changing the display controller
===============================

The binaries in this release archive are compiled for the ILI9481 display controller.
There is (untested) support for ILI9486 and HX8357C as well, so if you have those,
go to democode/src/demolib/cfg-demo.h, look for this:

typedef fglcd::preset::ILI9481_Mega_Shield LCD;

and change it to

typedef fglcd::preset::ILI9486_Mega_Shield LCD;
-or-
typedef fglcd::preset::HX8357C_Mega_Shield LCD;

In case your display controller is 480x320 but none of the above,
feel free to contact me and I might add support for your controller.
Alternatively create a new type describing the controller and add the init code
required to start up the display. In case you do this, please send me a pull request
so the display library can support more controllers!

Those MIPI displays are pretty compatible so it should work just fine after
it's set up properly with the correct initcode.


-- fgenesis,
for Revision 2020
