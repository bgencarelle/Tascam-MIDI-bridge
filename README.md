Tascam Serial Control for Midistudios and devices that use the Accessory 2 port.

   The goal of this project is to create an open device that can replace the obsolete and
   all but impossible-to-find sync devices for various Tascam tape machines.

   Midi control over all of the stuff controlled by the serial port for these devices has already been finished.
   You may want to change the CC#s to something more useful by you.

   There are two sketches-one solely for external control and one for
   external control plus SMPTE sync. The timing sync is not be finished as of June 2018.  
   I do plan on adding pitch control, whether or not I get the timing working.
   I also plan on generalizing this to make for a control thing for non-serial enabled tape machines.

   This is written for the Atmega328 as it is cheap and easily sourced, but if not using the
   LTC reader, virtually anything Arduino compatible will work.

   Use it, fork it, do whatever, but make sure to keep it freely available and respect the licenses of the people
   who made these libraries.

   insert CC license text here

   The MIDI and LTC portions of this code are lifted from various Arduino.cc forums
   (links go here)

   SoftwareSerialParity comes from:

   And has been converted into an Arduino compatible library in the same directory as these sketches

   All related documents can be found in this repository.

   Hardware roughly required:

   Arduino compatible microcontroller
   some sort of inverter/buffer to convert the UART data to reasonable voltages for RS-232
   Opto-isolator for standard MIDI
   Obviously USB midi and Bluetooth can be used, but are not currently implemented.

   Hardware specifically used:
   Arduino Uno with Midi Shield
   CD 4049 inverter for converting LTC from audio to digital and two other pins for UART inversion.
   You may want to use better hardware. I had no problems.
   various resistors and caps.
   USB FTDI uart thing for debugging

  There is a pinout available in the documents, just remember to connect your transmit to the device's receive
  and to both protect and invert the inputs from the higher and lower voltages that RS-232 uses.


  
