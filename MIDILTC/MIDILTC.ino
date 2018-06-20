#define CC_CHANNEL 1
#include <MIDI.h>
#include <SoftwareSerialParity.h>
SoftwareSerialParity SerialOne(10, 11); // RX, TX
//#define icpPin 8        // ICP input pin on arduino -not needed as handled by interrupt

#define one_time_max          475 // these values are setup for NTSC video
#define one_time_min          300 // PAL would be around 1000 for 0 and 500 for 1
#define zero_time_max         875 // 80bits times 29.97 frames per sec
#define zero_time_min         700 // equals 833 (divide by 8 clock pulses)

#define end_data_position      63
#define end_sync_position      77
#define end_smpte_position     80

volatile unsigned int bit_time;
volatile boolean valid_tc_word;
volatile boolean ones_bit_count;
volatile boolean tc_sync;
volatile boolean write_tc_out;
volatile boolean write_mtc_out;
volatile boolean drop_frame_flag;

volatile byte total_bits;
bool current_bit;
volatile byte sync_count;

volatile byte tc[8];
char timeCodeLTC[11];
char timeCodeMTC[11];

int ctr = 0;
//struct MySettings : public midi::DefaultSettings
//{
////   static const unsigned SysExMaxSize = 128; // Accept SysEx messages up to 1024 bytes long.
////   static const bool Use1ByteParsing = true;
////   static const bool UseRunningStatus = true;
//};
/*MTC stuff

*/
byte h_m, m_m, s_m, f_m;      //hours, minutes, seconds, frame
byte h_l, m_l, s_l, f_l;      //hours, minutes, seconds, frame
byte buf_temp_mtc[8];     //timecode buffer
byte command, data, index;

//MIDI_CREATE_CUSTOM_INSTANCE(HardwareSerial, Serial, MIDI, MySettings);
MIDI_CREATE_DEFAULT_INSTANCE();
// -----------------------------------------------------------------------------
//MIDI callback section
// This function will be automatically called when a MIDI message is received.
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
  if ((array[0] == 0xF1))
  { 
         SerialOne.println("ANYTHING");//uses sprint
    byte data = array[1];
    index = data >> 4;  //extract index/packet ID
    data &= 0x0F;       //clear packet ID from data
    buf_temp_mtc[index] = data;

    if (index >= 0x07) {  //recalculate timecode once FRAMES LSB quarter-frame received
//      h_m = byte((buf_temp_mtc[7] & 0x01) << 4) + buf_temp_mtc[6];
//      m_m = byte(buf_temp_mtc[5] << 4) + buf_temp_mtc[4];
//      s_m = byte(buf_temp_mtc[3] << 4) + buf_temp_mtc[2];
//      f_m = byte(buf_temp_mtc[1] << 4) + buf_temp_mtc[0];
//
//    //  sprintf(timeCodeMTC, "EXC: %.2d:%.2d:%.2d:%.2d ",h_m,m_m,s_m,f_m);
 
    }
  }

  if ((array[0] == 0xF0) && (array[size - 1] == 0xf7))
  {
    if (array[1] == 0x7f)
    {
      if ( array[2] == 0x7f)
      {
        switch (array[4]) {

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


void handleTimeCodeQuarterFrame(byte data)
{
    index = data >> 4;  //extract index/packet ID
    data &= 0x0F;       //clear packet ID from data
    buf_temp_mtc[index] = data;

  if (index >= 0x07) {  //recalculate timecode once FRAMES LSB quarter-frame received
    h_m = byte((buf_temp_mtc[7] & 0x01)<<4) + buf_temp_mtc[6];
    m_m = byte(buf_temp_mtc[5]<<4) + buf_temp_mtc[4];
    s_m = byte(buf_temp_mtc[3]<<4) + buf_temp_mtc[2];
    f_m = byte(buf_temp_mtc[1]<<4) + buf_temp_mtc[0];

    sprintf(timeCodeMTC, "tcq: %.2d:%.2d:%.2d:%.2d ",h_m,m_m,s_m,f_m);
    SerialOne.println(timeCodeMTC);//uses sprintf

     }
}

void handleControlChange(byte channel, byte number, byte value)
{
  if (channel == CC_CHANNEL)
  {
    switch (number) {
      case 0x33:
        if (value == 0x00)
        {
          SerialOne.println('S');//stop
        }
        break;

      case 0x36://handled by Callback
        SerialOne.println('P');//play
        break;

      case 0x7B://EMERGENCY STOP
    //    SerialOne.println('S');//stop
        SerialOne.println('Z');//stop
        break;

      case 0x32: //Record Strobe
        if (value > 0)
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
    SerialOne.println(pitch);

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

/* ICR interrupt vector */
ISR(TIMER1_CAPT_vect)
{
  //toggleCaptureEdge
  TCCR1B ^= _BV(ICES1);

  bit_time = ICR1;

  //resetTimer1
  TCNT1 = 0;

  if ((bit_time < one_time_min) || (bit_time > zero_time_max)) // get rid of anything way outside the norm
  {
    total_bits = 0;
  }
  else
  {
    if (ones_bit_count == true) // only count the second ones pluse
      ones_bit_count = false;
    else
    {
      if (bit_time > zero_time_min)
      {
        current_bit = 0;
        sync_count = 0;
      }
      else if (bit_time < one_time_max)
      {
        ones_bit_count = true;
        current_bit = 1;
        sync_count++;
        if (sync_count == 12) // part of the last two bytes of a timecode word
        {
          sync_count = 0;
          tc_sync = true;
          total_bits = end_sync_position;
        }
      }

      if (total_bits <= end_data_position) // timecode runs least to most so we need
      { // to shift things around
        tc[0] = tc[0] >> 1;

        for (int n = 1; n < 8; n++)
        {
          if (tc[n] & 1)
            tc[n - 1] |= 0x80;

          tc[n] = tc[n] >> 1;
        }

        if (current_bit == 1)
          tc[7] |= 0x80;
      }
      total_bits++;
    }

    if (total_bits == end_smpte_position) // we have the 80th bit
    {
      total_bits = 0;
      if (tc_sync)
      {
        tc_sync = false;
        valid_tc_word = true;
      }
    }

    if (valid_tc_word)
    {
      valid_tc_word = false;
      timeCodeLTC[10] = (tc[0] & 0x0F) + 0x30;  // frames
      timeCodeLTC[9] = (tc[1] & 0x03) + 0x30;  // 10's of frames
      timeCodeLTC[8] = ':';
      timeCodeLTC[7] = (tc[2] & 0x0F) + 0x30;  // seconds
      timeCodeLTC[6] = (tc[3] & 0x07) + 0x30;  // 10's of seconds
      timeCodeLTC[5] = ':';
      timeCodeLTC[4] = (tc[4] & 0x0F) + 0x30;  // minutes
      timeCodeLTC[3] = (tc[5] & 0x07) + 0x30;  // 10's of minutes
      timeCodeLTC[2] = ':';
      timeCodeLTC[1] = (tc[6] & 0x0F) + 0x30;  // hours
      timeCodeLTC[0] = (tc[7] & 0x03) + 0x30;  // 10's of hours
      drop_frame_flag = bit_is_set(tc[1], 2);
      write_tc_out = true;
    }

  }

}


void setup()
{
  //Midi section//
  MIDI.setHandleNoteOn(handleNoteOn);  // Put only the name of the function
  MIDI.setHandleNoteOff(handleNoteOff);
  MIDI.setHandleControlChange(handleControlChange);
  //MIDI.setHandleStop(handleStop);
  MIDI.setHandleStart(handleStart);
  MIDI.setHandleContinue(handleContinue);
  MIDI.setHandleSystemExclusive(handleSystemExclusive);
  MIDI.setHandleTimeCodeQuarterFrame(handleTimeCodeQuarterFrame);
  // Initiate MIDI communications, listen to all channels
  MIDI.begin(MIDI_CHANNEL_OMNI);
  attachInterrupt(1, TIMER1_CAPT_vect, CHANGE);
  SerialOne.begin(9600, EVEN);
  SerialOne.println('S');
  SerialOne.println();
  //pinMode(icpPin, INPUT);                  // ICP pin (digital pin 8 on arduino) as input

  bit_time = 0;
  valid_tc_word = false;
  ones_bit_count = false;
  tc_sync = false;
  write_tc_out = false;
  drop_frame_flag = false;
  total_bits =  0;
  current_bit =  0;
  sync_count =  0;

  TCCR1A = B00000000; // clear all
  TCCR1B = B11000010; // ICNC1 noise reduction + ICES1 start on rising edge + CS11 divide by 8
  TCCR1C = B00000000; // clear all
  TIMSK1 = B00100000; // ICIE1 enable the icp
  TCNT1 = 0; // clear timer1

}

void loop()
{
  static byte temp_frame = 0;
  byte current_frame;
  MIDI.read();

     if (write_tc_out)
     {
       current_frame = (tc[0]& 0x0F) + (tc[1] & 0x0F) *10;
       SerialOne.println(current_frame);
       if (current_frame > temp_frame)
       {
         if (current_frame != temp_frame + 1)
         {
          //SerialOne.println(current_frame - temp_frame);
         }
       }
       temp_frame = current_frame;
       write_tc_out = false;
  ////     if (drop_frame_flag)
  ////     {
  ////       SerialOne.print("TC-[df] ");
  ////     }
  ////     else
  ////     {
  ////       SerialOne.print("TC-[nd] ");
  ////     }
     }

  if (write_mtc_out )
  {
    //  delay(100);
    //   SerialOne.print("LTC: ");//uses bits
    //    SerialOne.println((char*)timeCodeLTC);

    // delay(1000);
  }

}



