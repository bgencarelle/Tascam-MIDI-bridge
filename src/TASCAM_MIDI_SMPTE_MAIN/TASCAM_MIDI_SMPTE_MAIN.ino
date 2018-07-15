
/*   Tascam Serial Control for Midistudios and devices that use the Accessory 2 port.
//ff time 1:23
//rw time 1:16
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

  See the Readme.md for more details
   2018 Ben Gencarelle
*/
#define TIME_SYNC 1 //comment this out for just MIDI control over serial port
//#define EXTERNAL_CONTROL 1 // to use for generalized control
#define MIDI_CONTROL 1
#include <MIDI.h>

#if defined(__AVR_ATmega328__) || defined(__AVR_ATmega328P__) || defined(__AVR_ATmega168__)
//UNO like
#define UNO 1 //for interrupt stuff
#define icpPin 8 // ICP input pin on arduino
#include <SoftwareSerialParity.h>// Not needed for devices with multiple UARTS
SoftwareSerialParity SerialOne(10, 11); // RX, TX
MIDI_CREATE_DEFAULT_INSTANCE();

#elif (__AVR_ATmega2560__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega640__)
//Arduino Mega like
#define MEGA 1 //for interrupt stuff
#define icpPin  48// ICP input pin on arduino -not needed as handled by interrupt
#define EVEN SERIAL_8E1
HardwareSerial & SerialOne = Serial1;
MIDI_CREATE_DEFAULT_INSTANCE();

#endif

void setup()
{
  SerialOne.begin(9600, EVEN);
 // SerialOne.println('S');
 // SerialOne.println();
 
#if defined (MIDI_CONTROL)
  midiSetup();
#endif

#if defined (TIME_SYNC)
 smpteSetup();
#endif

}

void loop()
{
#if defined (MIDI_CONTROL)
  MIDI.read();
#endif

}


