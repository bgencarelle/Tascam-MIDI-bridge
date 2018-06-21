/*   Tascam Serial Control for Midistudios and devices that use the Accessory 2 port.

   See the Readme.md for more details 
   
   The goal of this project is to create an open device that can replace the obsolete and 
   all but impossible-to-find sync devices for various Tascam tape machines.
   
   Hardware roughly required:

   Arduino compatible microcontroller
   some sort of inverter/buffer to convert the UART data to reasonable voltages for RS-232
   Opto-isolator for standard MIDI
   Obviously USB midi and Bluetooth can be used, but are not currently implemented.

   Hardware specifically used by me:
   Arduino Uno with Midi Shield
   CD 4049 inverter for converting LTC from audio to digital and two other pins for UART inversion.
   You may want to use better hardware. I had no problems.
   various resistors and caps.
   USB FTDI uart thing for debugging

  There is a pinout available in the documents, just remember to connect your transmit to the device's receive
  and to both protect and invert the inputs from the higher and lower voltages that RS-232 uses. I had great luck with 
  the 4049s and 5v, but I am not sure if 3.3v stuff would play nice.
  
   2018 Ben Gencarelle
*/
//#define TIME_SYNC 1 //comment this out for just MIDI control over serial port
//#define EXTERNAL_CONTROL 1 // to use for generalized control

#define CC_CHANNEL 1//everything on one CC makes easier
#include <MIDI.h>

#include <SoftwareSerialParity.h>// Not needed for devices with multiple UARTS
SoftwareSerialParity SerialOne(10, 11); // RX, TX
MIDI_CREATE_DEFAULT_INSTANCE();

void setup()
{
  SerialOne.begin(9600, EVEN);
  SerialOne.println('S');
  SerialOne.println();
  //Midi section//
  MIDI.setHandleNoteOn(handleNoteOn);  // Put only the name of the function
  MIDI.setHandleNoteOff(handleNoteOff);
  MIDI.setHandleControlChange(handleControlChange);
  //MIDI.setHandleStop(handleStop);
  //  MIDI.setHandleStart(handleStart);
  //  MIDI.setHandleContinue(handleContinue);
  //  MIDI.setHandleSystemExclusive(handleSystemExclusive);

  // Initiate MIDI communications, listen to all channels
  MIDI.begin(MIDI_CHANNEL_OMNI);


#if defined (TIME_SYNC)
  smpteSetup();
#endif

}

void loop()
{
  MIDI.read();
  
#if defined (TIME_SYNC)
  {
    timeCodeCall();
  }
#endif
}



