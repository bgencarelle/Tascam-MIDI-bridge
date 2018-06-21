Tascam Serial Control for Midistudios and devices that use the Accessory 2 port.

  The goal of this project is to create an open device that can replace the obsolete and
  all but impossible-to-find sync devices for various Tascam tape machines.
  There are two sketches-one solely for external control and one for
  external control plus SMPTE sync. The timing sync will probably not be finished.

  This is written for the Atmega328 as it is cheap and easily sourced, but if not using the
  LTC reader, virtually anything Arduino compatible will work.

  insert CC license text here

  The MIDI and LTC portions of this code are lifted from various Arduino.cc forums
  (links go here)

  SoftwareSerialParity comes from:

  And has been converted into an Arduino compatible library in the same directory as these sketches

  All related documents can be found in this repository.

  Hardware required:

  Arduino compatible microcontroller
  some sort of inverter/buffer to convert the UART data to reasonable voltages for RS-232
  Opto-isolator for standard MIDI
  Obviously USB midi and Bluetooth can be used, but are not currently implemented.

  Hardware used:
  Arduino Uno with Midi Shield
  CD 4049 inverter for LTC from tape and for UART inversion.
  resistors and caps.
  USB UART for debugging
  2018 Ben Gencarelle
