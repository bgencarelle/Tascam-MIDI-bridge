
#define CC_CHANNEL 1
#include <MIDI.h>
#include <SoftwareSerialParity.h>


//#include <SoftwareSerialParity.h>
SoftwareSerialParity SerialOne(2, 3); // RX, TX

struct MySettings : public midi::DefaultSettings
{
//   static const unsigned SysExMaxSize = 128; // Accept SysEx messages up to 1024 bytes long.
//   static const bool Use1ByteParsing = true;
//   static const bool UseRunningStatus = true;
};
MIDI_CREATE_CUSTOM_INSTANCE(HardwareSerial, Serial, MIDI, MySettings);


// -----------------------------------------------------------------------------

// This function will be automatically called when a NoteOn is received.
// It must be a void-returning function with the correct parameters,
// see documentation here:
// http://arduinomidilib.fortyseveneffects.com/a00022.html

void handleStart()
{
      SerialOne.println('P');
}
void handleStop()
{
      SerialOne.println('S');
}
void handleContinue()
{
      SerialOne.println('P');
}

void handleSystemReset()
{
      SerialOne.println('Z');
}
   
void handleSystemExclusive(byte *array, unsigned size)
{
if((array[0]==0xF0) && (array[size-1]==0xf7))
{
  if (array[1] == 0x7f)
  {
  if( array[2] == 0x7f)
  {
   switch (array[4]){
    
    case 0x01://F0  7F  7F  06  01  F7  stop
    SerialOne.println('S');
    break;
    
    case 0x02://F0  7F  7F  06  02  F7 play
    SerialOne.println('P');//play
    break;
    
    case 0x03://F0  7F  7F  06  03  F7 FF
    SerialOne.println('P');//play
    break;

    case 0x06://F0  7F  7F  06  06  F7  |  MMC Record Strobe
    SerialOne.println('V');
    break;
    
    case 0x07://F0  7F  7F  06  06  F7  |  MMC Record Off
    SerialOne.println('W');
    break;
    
    case 0x09://F0  7F  7F  06  09  F7  |  MMC Pause
    SerialOne.println('X');  
    break;
    
    default:
    SerialOne.print(array[0], HEX);
    SerialOne.print(array[1], HEX);
    SerialOne.print(array[2], HEX);
    SerialOne.print(array[3], HEX);
    SerialOne.print(array[4], HEX);
    SerialOne.print(array[5], HEX);
    SerialOne.println("   unknown mmc message ");
 
  }
  }
  }
}
}



void handleControlChange(byte channel, byte number, byte value)
{
if (channel == CC_CHANNEL)
{
   switch (number){
    case 0x33:
    if (value == 0x00)
    {
    SerialOne.println('S');//stop
    }
    break;
    
//    case 0x36://handled by Callback
//    SerialOne.println('P');//play
//    break;

    case 0x7B://EMERGENCY STOP
    SerialOne.println('S');//stop
    SerialOne.println('Z');//stop
    break;
    
    case 0x32: //Record Strobe
    if (value >0)
    {
    SerialOne.println('V');//record on
    }
    else 
    {
    SerialOne.println('W');//record off
    }
    break;
       
    
//    default:
//    SerialOne.print(channel, HEX);
//    SerialOne.println("   unknown cc message ")"
// 
  }
 }
}
void handleNoteOn(byte channel, byte pitch, byte velocity)
{
  // do something different depending on the range value:
  {
   // SerialOne.println(pitch);
    
  }
    // Do whatever you want when a note is pressed.

    // Try to keep your callbacks short (no delays ect)
    // otherwise it would slow down the loop() and have a bad impact
    // on real-time performance.

}
void handleNoteOff(byte channel, byte pitch, byte velocity)
{
    // Do something when the note is released.
    // Note that NoteOn messages with 0 velocity are interpreted as NoteOffs.
}

// -----------------------------------------------------------------------------

void setup()
{

    // Connect the handleNoteOn function to the library,
    // so it is called upon reception of a NoteOn.
//    MIDI.setHandleNoteOn(handleNoteOn);  // Put only the name of the function
//    MIDI.setHandleNoteOff(handleNoteOff);
    MIDI.setHandleControlChange(handleControlChange);
    //MIDI.setHandleStop(handleStop);
    //MIDI.setHandleStart(handleStart);
    MIDI.setHandleContinue(handleContinue);
    MIDI.setHandleSystemExclusive(handleSystemExclusive);
    // Do the same for NoteOffs

    // Initiate MIDI communications, listen to all channels
    MIDI.begin(MIDI_CHANNEL_OMNI);
    
    SerialOne.begin(9600,SERIAL_8E1);
    SerialOne.println('S');
    SerialOne.println("setup complete ");
    SerialOne.println();
}

void loop()
{
    // Call MIDI.read the fastest you can for real-time performance.
 
    MIDI.read();
    // There is no need to check if there are messages incoming
    // if they are bound to a Callback function.
    // The attached method will be called automatically
    // when the corresponding message has been received.
}
