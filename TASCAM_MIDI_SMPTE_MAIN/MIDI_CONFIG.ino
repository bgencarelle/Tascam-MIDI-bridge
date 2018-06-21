
volatile byte lastPlayStatus = 0;//needed for record toggle

// -----------------------------------------------------------------------------
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

      
      //Record toggle Track indicated - possible to poll for active tracks by
      // sending @2 and parsing the right nibble - not implemented
      // tracks 1,2,3,4 are byte 2, tracks 5,6,7,8 are byte 3
      case 0x14:
        {
          SerialOne.println("C1");//record on
        }
        break;
      case 0x15:
        {
          SerialOne.println("C2");//record on
        }
        break;
      case 0x16:
        {
          SerialOne.println("C3");//record on
        }
        break;
      case 0x17:
        {
          SerialOne.println("C4");//record on
        }
        break;
      case 0x18:
        {
          SerialOne.println("C5");//record on
        }
        break;
      case 0x19:
        {
          SerialOne.println("C6");//record on
        }
        break;
      case 0x1A:
        {
          SerialOne.println("C7");//record on
        }
        break;
      case 0x1B:
        {
          SerialOne.println("C8");//record on
        }
        break;
      case 0x32: //Record Strobe
        if (value != 0x0 && lastPlayStatus == 1)
        {
          SerialOne.println('V');//Play record on
        }

        else if (value !=0x0 && lastPlayStatus != 1)
        {
          SerialOne.println('Y');//Pause record on
        }
        else if (value == 0)
        {
          SerialOne.println('W');//record off-does not work from pause
        }
        break;
      //end of recording

      //transport controls
      case 0x33://stop can also be handled by callback
        if (value == 0x0)
        {      
        SerialOne.println('S');//stop
        lastPlayStatus = 0;
        }
        break;

      case 0x36://can also be handled by callback
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

      case 0x09://Rewind & FF
      if (value !=0)
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

      case 0x1c://Repeat toggle
          SerialOne.write(0x5C);     
        break;
        
      case 0x39://set memo 1
          SerialOne.println("E1");
        break;
      case 0x3A://set memo 2
          SerialOne.println("E2");
        break;
        
      case 0x3B://set Monitor Mode Insert
      if (value > 0x00)
      {
          SerialOne.println("O2");
      }
        break;      

      case 0x3C://set Mix Mode Insert
      if (value > 0x00)
      {
          SerialOne.println("O3");
      }
        break;   
                
      case 0x3D://Display Change
      if (value > 0x00)
      {
          SerialOne.println("O8");
      }
        break;  
         
      case 0x3E://Return to zero
      if (value > 0x00)
      {
          SerialOne.println("Z");
      }
        break;     
                
      case 0x7B://EMERGENCY STOP
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
//  SerialOne.println('P');
//}

void handleSystemReset()
{
  SerialOne.println('Z');
}

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



void handleNoteOn(byte channel, byte pitch, byte velocity)
{
  // do something different depending on the range value:
  {

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

//


