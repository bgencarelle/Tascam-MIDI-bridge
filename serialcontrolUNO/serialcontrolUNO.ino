#include <MIDI.h>

#include <SoftwareSerialParity.h>
SoftwareSerialParity mySerial(2, 3); // RX, TX
MIDI_CREATE_DEFAULT_INSTANCE();

// -----------------------------------------------------------------------------

// This function will be automatically called when a NoteOn is received.
// It must be a void-returning function with the correct parameters,
// see documentation here:
// http://arduinomidilib.fortyseveneffects.com/a00022.html

void handleStart()
{
      mySerial.println('P');
}
void handleStop()
{
      mySerial.println('S');
}

void handleSystemReset()

{
      mySerial.println('Z');
}
    // Do whatever you want when a note is pressed.

    // Try to keep your callbacks short (no delays ect)
    // otherwise it would slow down the loop() and have a bad impact
    // on real-time performance.
void HandleSystemExclusive(byte *array, unsigned size)
{
  if( array[2] == 0x7f)
  {
   switch (array[4]){
    case 0x01:
    mySerial.println('S');//stop
    break;
    
    case 0x02:
    mySerial.println('P');//play
    break;

    case 0x06:
    mySerial.println('V');
    break;
    
    case 0x07:
    mySerial.println('W');
    break;
    
    case 0x09:
    mySerial.println('X');  
    
    default:
    mySerial.print(array[2], HEX);
    mySerial.println("   unknown message ");
   }
  }

//F0  7F  7F  06  01  F7  stop
//F0  7F  7F  06  02  F7 play
//F0  7F  7F  06  09  F7  |  MMC Pause
//F0  7F  7F  06  06  F7  |  MMC Record Strobe

}
void handleNoteOn(byte channel, byte pitch, byte velocity)
{
  // do something different depending on the range value:
  {
    mySerial.println(channel);
    mySerial.println(pitch);
    mySerial.println(velocity);
    
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
    mySerial.begin(9600,EVEN);
    
      mySerial.println('P');
    // Connect the handleNoteOn function to the library,
    // so it is called upon reception of a NoteOn.
    MIDI.setHandleNoteOn(handleNoteOn);  // Put only the name of the function
  
    MIDI.setHandleNoteOn(handleNoteOff);  // Put only the name of the function
   // MIDI.setHandleStop(handleStop);
    MIDI.setHandleStart(handleStart);
    MIDI.setHandleSystemExclusive(HandleSystemExclusive);
    // Do the same for NoteOffs
    MIDI.setHandleNoteOff(handleNoteOff);

    // Initiate MIDI communications, listen to all channels
    MIDI.begin(MIDI_CHANNEL_OMNI);
    mySerial.println("setup complete ");
    mySerial.println();
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
