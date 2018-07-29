/*   Tascam Serial Control for Midistudios and devices that use the Accessory 2 port.

//ff time 1:23 
//rw time 1:16 
//pin 13 on the controller handles chaseMode 
//pin 5 is direction-rw is low ff/play/stop is high 
//pin 6 is tach pulse  3.22 hz is play, rw is between 25 and ~78hz ends low, ff is 44 to 30 hz,play is 2.92hz 

   The goal of this project is to create an open device that can replace the obsolete and
   all but impossible-to-find sync devices for various Tascam tape machines.
  See the Readme.md for more details
   2018 Ben Gencarelle
*/
#include <MIDI.h>
#include <Tone.h>
#include <Wire.h>
#include "SSD1306Ascii.h"
#include "SSD1306AsciiWire.h"
SSD1306AsciiWire oled;

volatile bool writeLTCOut;
volatile bool writeMTCOut;
Tone playSpeed; 

#define TIME_SYNC 1 //comment this out for just MIDI control over serial port
//#define EXTERNAL_CONTROL 1 // to use for generalized control
#define MIDI_CONTROL 1
#define OLED_MODE 1
#if !defined(TIME_SYNC) 
  #define STRIPE_MODE 1 
#endif 

#if defined(__AVR_ATmega328__) || defined(__AVR_ATmega328P__) || defined(__AVR_ATmega168__)
//UNO like
#define UNO 1 //for interrupt stuff
#define icpPin 8 // ICP input pin on arduino
#define MOTOR_SPEED_LOW 6400 
#define MOTOR_SPEED_RUN 9600 
#define MOTOR_SPEED_HIGH 12800
#include <SoftwareSerialParity.h>// Not needed for devices with multiple UARTS
SoftwareSerialParity SerialOne(6, 7); // RX, TX
MIDI_CREATE_DEFAULT_INSTANCE();

#elif (__AVR_ATmega2560__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega640__) 
#define MEGA 1 //for interrupt stuff
#define icpPin  48// ICP input pin on arduino -not needed as handled by interrupt
#define EVEN SERIAL_8E1
HardwareSerial & SerialOne = Serial1;
MIDI_CREATE_DEFAULT_INSTANCE();
#define MOTOR_SPEED_LOW 6400 
#define MOTOR_SPEED_RUN 9600 
#define MOTOR_SPEED_HIGH 12800 
#endif

int chaseStateControlPin= 9; 
int chaseFrequencyPin=11;  

void setup()
{

  SerialOne.begin(9600, EVEN);
  SerialOne.println('S');
  pinMode(chaseFrequencyPin,OUTPUT); 
  pinMode(chaseStateControlPin,OUTPUT); 
  digitalWrite(chaseStateControlPin, LOW); 
  delay(100); 
  digitalWrite(chaseStateControlPin, HIGH); 
  playSpeed.begin(chaseFrequencyPin);// 
  playSpeed.play(MOTOR_SPEED_RUN);// 
  
#if defined (STRIPE_MODE) 
  stripeSetup();//  
#endif 
#if defined (MIDI_CONTROL)
  midiSetup();
#endif

#if defined (OLED_MODE)
oledSetup(); 
#endif

#if defined (TIME_SYNC)
 smpteSetup();
#endif

}

void loop()
{
#if defined (MIDI_CONTROL) && !defined (STRIPE_MODE) 
  MIDI.read();
#endif

}


