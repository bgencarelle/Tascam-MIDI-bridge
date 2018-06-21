---
name: Midi to Tascam Serial Control
Usage: Midistudios and devices that use the Tascam Accessory 2 port.
---
*Goals*
Goal: The goal of this project is to create an open device that can replace the obsolete and all but impossible-to-find sync devices for various Tascam tape machines.

*The desired functions*

MIDI transport control of all functions available over the serial port.
SMPTE Time Code Chase to external reference.
Speed/pitch control of transport.

*Current Status*

Midi control over all of the stuff controlled by the serial port for these devices has already been finished. You may want to change the CC's to something more useful by you, as they were setup to be most useful with my MIDI controller.

There are two sketches-one solely for external control and one for external control plus SMPTE sync.
The timing sync is not finished as of June 2018, but the decoders both work well.

I do plan on adding DAC based pitch control, whether or not I get the timing working.

I also plan on generalizing this to make for a control thing for non-serial enabled tape machines.

This is written for the Arduino UNO as it is cheap and easily sourced, but, virtually anything Arduino compatible will work.

Obviously USB midi and Bluetooth can be used, but are not currently implemented.

*Hardware roughly required*

Arduino compatible microcontroller.
Some sort of inverter/buffer to convert the UART data to reasonable voltages for RS-232.
Driver/inverter to get the LTC to digital levels.
Opto-isolator for standard MIDI.

*Hardware specifically used*
Arduino Uno with Midi Shield CD 4049 inverter for both converting LTC from audio to digital levels and two other pins for the UART inversion and voltage conversion.
Various resistors and caps.
USB FTDI UART thing for debugging.

You may want to use better hardware. I had no problems.

*Notes on the Serial and MIDI format*

There is a pinout available in the documents as both a GIF and a PDF.
Basically just remember to connect your transmit to the device's receive.
Do not forget to both protect and invert the inputs to save your device from the higher and lower voltages that RS-232 uses.

The RS-232 format is 9600 baud, 8 bits, even parity, no handshaking.

Each message is sent as a byte, or a pair of bytes followed by usual end of line characters.

Make sure to wait 100ms between messages. This won't be a problem usually.

*Attribution and licenses*
Use it, fork it, do whatever, but make sure to keep it freely available, give me a bit of credit and respect the licenses of the people who made these libraries.

https://creativecommons.org/licenses/by-sa/4.0/

The MIDI and LTC portions of this code are lifted from various Arduino.cc forums
<http://arduino.cc>

SoftwareSerialParity comes from:

<https://github.com/ledongthuc/CustomSoftwareSerial>

And has been converted into an Arduino compatible library in the same directory as these sketches

All related documents used for research can be found in this repository.
