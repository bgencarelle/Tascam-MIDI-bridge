

#if defined (TIME_SYNC)

#if defined UNO
#define edgeCap TIMER1_CAPT_vect

#elif defined MEGA
#define edgeCap TIMER5_CAPT_vect
#endif

//LTC stuff borrowed from Arduino forum

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
volatile boolean write_ltc_out;
volatile boolean drop_frame_flag;
volatile byte total_bits;
volatile bool current_bit;
volatile byte sync_count;

volatile byte tc[8];
volatile byte timeCodeLTC[11];
char df = 'n';
char* flag_l;
//MTC stuff from forum
char timeCodeMTC[14];
char dfm = 'n';
char* flag_m;
volatile boolean drop_frame_flag_mtc;
volatile boolean write_mtc_out;
byte h_m, m_m, s_m, f_m;      //hours, minutes, seconds, frame
char h_l, m_l, s_l, f_l;      //hours, minutes, seconds, frame
byte buf_temp_mtc[8];     //timecode buffer
byte command, data, index;

int ctr = 0;

/* ICR interrupt vector */
ISR(edgeCap)
{


#if defined UNO
  //toggleCaptureEdge
  TCCR1B ^= _BV(ICES1);
  bit_time = ICR1;
  //resetTimer1
  TCNT1 = 0;

#elif defined MEGA
  //toggleCaptureEdge
  TCCR5B ^= _BV(ICES5);
  bit_time = ICR5;
  //resetTimer1
  TCNT5 = 0;
#endif

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
      timeCodeLTC[8] =  '.';
      timeCodeLTC[7] = (tc[2] & 0x0F) + 0x30;  // seconds
      timeCodeLTC[6] = (tc[3] & 0x07) + 0x30;  // 10's of seconds
      timeCodeLTC[5] =  ':';
      timeCodeLTC[4] = (tc[4] & 0x0F) + 0x30;  // minutes
      timeCodeLTC[3] = (tc[5] & 0x07) + 0x30;  // 10's of minutes
      timeCodeLTC[2] = ':';
      timeCodeLTC[1] = (tc[6] & 0x0F) + 0x30;  // hours
      timeCodeLTC[0] = (tc[7] & 0x03) + 0x30;  // 10's of hours
      drop_frame_flag = (tc[1] & 0x04) != 0;

      if (drop_frame_flag)
      {
        df = 'd';
      }
      else
      {
        df = 'n';
      }
      write_ltc_out = true;

    }
  }
}

void smpteSetup()
{
  MIDI.setHandleTimeCodeQuarterFrame(handleTimeCodeQuarterFrame);
  pinMode(icpPin, INPUT);                  // ICP pin (digital pin 8 on arduino uno) as input
  bit_time = 0;
  valid_tc_word = false;
  ones_bit_count = false;
  tc_sync = false;
  write_ltc_out = false;
  drop_frame_flag = false;
  total_bits =  0;
  current_bit =  0;
  sync_count =  0;

#if defined UNO
  attachInterrupt(1, TIMER1_CAPT_vect, CHANGE);
  TCCR1A = B00000000; // clear all
  TCCR1B = B11000010; // ICNC1 noise reduction + ICES1 start on rising edge + CS11 divide by 8
  TCCR1C = B00000000; // clear all
  TIMSK1 = B00100000; // ICIE1 enable the icp
  TCNT1 = 0; // clear timer1

#elif defined MEGA
  attachInterrupt(1, TIMER5_CAPT_vect, CHANGE);
  TCCR5A = B00000000; // clear all
  TCCR5B = B11000010; // ICNC1 noise reduction + ICES1 start on rising edge + CS11 divide by 8
  TCCR5C = B00000000; // clear all
  TIMSK5 = B00100000; // ICIE1 enable the icp
  TCNT5 = 0; // clear timer1

#endif

}

void handleTimeCodeQuarterFrame(byte data)
{
  index = data >> 4;  //extract index/packet ID
  data &= 0x0F;       //clear packet ID from data
  buf_temp_mtc[index] = data;

  if (index >= 0x07) {  //recalculate timecode once FRAMES LSB quarter-frame received
    h_m = byte((buf_temp_mtc[7] & 0x01) << 4) + buf_temp_mtc[6];
    m_m = byte(buf_temp_mtc[5] << 4) + buf_temp_mtc[4];
    s_m = byte(buf_temp_mtc[3] << 4) + buf_temp_mtc[2];
    f_m = byte(buf_temp_mtc[1] << 4) + buf_temp_mtc[0];

    drop_frame_flag_mtc = (buf_temp_mtc[1] & 0x04) != 0;
    if (drop_frame_flag_mtc)
    {
      dfm = 'd';
    }
    else
    {
      dfm = 'n';
    }
    write_mtc_out = true;
  }
}
void timeCodeCall()
{
char value [8] = {'f','F','s','S','m','M','h','H'};
  if (write_mtc_out && write_ltc_out)
  {
    
    SerialOne.println('S'); 
    sprintf(timeCodeMTC, "MTC:%c:%.2d:%.2d:%.2d:%.2d ", dfm, h_m, m_m, s_m, f_m);
    
    SerialOne.println(timeCodeMTC);
    
     SerialOne.println((char*)timeCodeLTC);
    
     
    write_ltc_out = false;
    write_mtc_out = false;
  
//
//      /// Hour config
//      if (ltcHours < mtcHours)
//      {
//        hourSync = 0;
//      }
//      else if (ltcHours == mtcHours)
//      {
//        hourSync = 1;
//      }
//      else if (ltcHours > mtcHours)
//      {
//        hourSync = 2;
//      }
//
//      ////Minute config
//      if (ltcMinutes < mtcMinutes)
//      {
//        minSync = 0;
//      }
//      if (ltcMinutes == mtcMinutes)
//      {
//        minSync = 1;
//      }
//      if (ltcMinutes > mtcMinutes)
//      {
//        minSync = 2;
//      }
//
//      ////Second config
//      if (ltcSeconds < mtcSeconds)
//      {
//        secSync = 0;
//      }
//      if (ltcSeconds == mtcSeconds)
//      {
//        secSync = 1;
//      }
//      if (ltcSeconds > mtcMinutes)
//      {
//        secSync = 2;
//      }
//
//      ////frames config
//      if (ltcFrames < mtcFrames)
//      {
//        frameSync = 0;
//      }
//      if (ltcFrames == mtcFrames)
//      {
//        frameSync = 1;
//      }
//      if (ltcFrames > mtcFrames)
//      {
//        frameSync = 2;
//      }
//    }

  }
}
#endif
