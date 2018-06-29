#define Tas688 1 //default 
//define Tas644 1
#define CC_CHANNEL 16//everything on one CC makes easier
#define CHAN_MIN 36
#define CHAN_MAX 45
//transport control
#define AllStop 0x00
#define RecStrobe 0x01
#define PlayOn 0x02
#define FFRew 0x04
//record toggle
#define RecTog1 0x14
#define RecTog2 0x15
#define RecTog3 0x16
#define RecTog4 0x17
#define RecTog5 0x18
#define RecTog6 0x19
#define RecTog7 0x1a
#define RecTog8 0x1b
//control stuff
#define RepToggle 0x1c
#define SetMem1  0x1d
#define SetMem2  0x1e
#define MonModeIns 0x1f
#define MonModeMix 0x34
#define DispChange 0x36
#define RTZero 0x37
#define EMGStop 0x7b
//end of defines
volatile byte lastPlayStatus = 0;//needed for record toggle, global for reasons

void midiSetup() {
  //Midi section//
  MIDI.setHandleNoteOn(handleNoteOn);  // Put only the name of the function
  MIDI.setHandleNoteOff(handleNoteOff);
  MIDI.setHandleControlChange(handleControlChange);
  MIDI.setHandleProgramChange(handleProgramChange);
  MIDI.setHandlePitchBend(handlePitchBend);
  MIDI.begin(MIDI_CHANNEL_OMNI);
  MIDI.turnThruOff();//no need for midi thru
}

//MIDI callback section
// This function will be automatically called when a MIDI message is received.
// It must be a void-returning function with the correct parameters,
// see documentation here:
// http://arduinomidilib.fortyseveneffects.com/a00022.html

//All commands are CC for simplicity sake
void handleControlChange(byte channel, byte number, byte value)
{
  byte lastNumber;//tracks last valid CC
  byte lastValue;//tracks last valid value
  if (channel == CC_CHANNEL)
  {
    switch (number) {//reassign CCs as needed for your application
      //transport controls
      case AllStop://STOP can also be handled by callback
        if (value == 0x0)
        {
          SerialOne.println('S');//stop
          lastPlayStatus = 0;
        }
        break;

      case PlayOn://can also be handled by callback
        if (value == 0x0)
        {
          if (lastPlayStatus != 1)
          {
            SerialOne.println('P');//play
            lastPlayStatus = 1;
          }
          else if (lastPlayStatus == 1)
          {
            SerialOne.println('X');//pause
            lastPlayStatus = 0;
          }
        }
        break;

      case FFRew://Rewind & FF
        if (value != 0)
        {
          if (value <= 0x32)
          {
            SerialOne.println('R');
          }
          else
          {
            SerialOne.println('Q');
          }
          lastPlayStatus = 0;
        }
        break;

      case RecStrobe: //Record Strobe
        if (value != 0x0 && lastPlayStatus == 1)
        {
          SerialOne.println('V');//Play record on
        }
        else if (value != 0x0 && lastPlayStatus != 1)
        {
          SerialOne.println('Y');//Pause record on
        }
        else if (value == 0)
        {
          SerialOne.println('W');//record off-does not work from pause
        }
        break;

      //Record toggle Track indicated - possible to poll for active tracks by
      // sending @2 and parsing the right nibble - not implemented
      // tracks 1,2,3,4 are byte 2, tracks 5,6,7,8 are byte 3
      case RecTog1:
        {
          SerialOne.println("C1");//record on
        }
        break;
      case RecTog2:
        {
          SerialOne.println("C2");//record on
        }
        break;
      case RecTog3:
        {
          SerialOne.println("C3");//record on
        }
        break;
#if !defined (Tas644) ||  !defined(TIME_SYNC) //sync Track is 4th track
      case RecTog4:
        {
          SerialOne.println("C4");//record on
        }
        break;
#endif
#if defined (Tas688)  //avoids sending nonsense messages to 644
      case RecTog5:
        {
          SerialOne.println("C5");//record on
        }
        break;
      case RecTog6:
        {
          SerialOne.println("C6");//record on
        }
        break;
      case RecTog7:
        {
          SerialOne.println("C7");//record on
        }
        break;

#if !defined (TIME_SYNC)//prevents accidental overwriting of synctrack
      case RecTog8:
        {
          SerialOne.println("C8");//record on
        }
        break;
#endif
#endif

      //end of recording
      case RepToggle://Repeat toggle
        SerialOne.write(0x5C);
        break;

      case SetMem1://set memo 1
        SerialOne.println("E1");
        break;

      case SetMem2://set memo 2
        SerialOne.println("E2");
        break;

      case MonModeIns://set Monitor Mode Insert
        if (value > 0x00)
        {
          SerialOne.println("O2");
        }
        break;

      case MonModeMix://set Mix Mode
        if (value > 0x00)
        {
          SerialOne.println("O3");
        }
        break;

      case DispChange://Display Change
        if (value > 0x00)
        {
          SerialOne.println("O8");
        }
        break;

      case RTZero://Return to zero
        if (value > 0x00)
        {
          SerialOne.println("Z");
        }
        break;

      case EMGStop://EMERGENCY STOP
        SerialOne.println('S');//stop
        lastPlayStatus = 0;
        break;

      default:
        lastNumber = number;
        lastValue = value;
    }
    lastNumber = number;
    lastValue = value;
  }
}
//this part can be done as a CC as well
void handleNoteOn(byte channel, byte pitch, byte velocity)
{ // send note on for muting
  // only really works as a toggle
if ((pitch >= CHAN_MIN) && (pitch <= CHAN_MAX))
  {
    MIDI.sendNoteOn(pitch, 127, channel);
  }
}
void handleNoteOff(byte channel, byte pitch, byte velocity)
{ // send note on for controlling the channels
  if ((pitch >= CHAN_MIN) && (pitch <= CHAN_MAX) )
  {
    MIDI.sendNoteOn(pitch, 1, channel);
  }
}
void handleProgramChange(byte channel, byte number)//sends program change-"SCENES"
{
  MIDI.sendProgramChange(number - 1, channel); //add offset because of weirdness in my controller
}
void handlePitchBend(byte channel, int bend)
{
  //coming soon
}

//void handleStart()//using CCs for now to simplify
//{
//  SerialOne.println('P');
//}
//void handleStop()
//{
//  SerialOne.println('S');
//}
//void handleContinue()
//{
//  SerialOne.println('X');
//}
//void handleSystemReset()
//{
// // SerialOne.println('Z');//return to zero
//}

//Sysex MMC is a pain.  These work, but it is probably best to choose either MMC or CC
//void handleSystemExclusive(byte *array, unsigned size)
//{
//  if ((array[0] == 0xF0) && (array[size - 1] == 0xf7))
//  {
//    if (array[1] == 0x7f)
//    {
//      if ( array[2] == 0x7f)
//      {
//        switch (array[4]) {
//
//          case 0x01://F0  7F  7F  06  01  F7  stop
//
//            SerialOne.println('S');
//            break;
//
//          case 0x02://F0  7F  7F  06  02  F7 play
//            SerialOne.println('P');//play
//            break;
//
//          case 0x03://F0  7F  7F  06  03  F7 FF
//            SerialOne.println('P');//play
//            break;
//
//          case 0x06://F0  7F  7F  06  06  F7  |  MMC Record Strobe
//            SerialOne.println('V');
//            break;
//
//          case 0x07://F0  7F  7F  06  06  F7  |  MMC Record Off
//            SerialOne.println('W');
//            break;
//
//          case 0x09://F0  7F  7F  06  09  F7  |  MMC Pause
//            SerialOne.println('X');
//            break;
//
//          default:
//            SerialOne.print(array[0], HEX);
//            SerialOne.print(array[1], HEX);
//            SerialOne.print(array[2], HEX);
//            SerialOne.print(array[3], HEX);
//            SerialOne.print(array[4], HEX);
//            SerialOne.print(array[5], HEX);
//            SerialOne.println("   unknown mmc message ");
//
//        }
//      }
//    }
//  }
//}




