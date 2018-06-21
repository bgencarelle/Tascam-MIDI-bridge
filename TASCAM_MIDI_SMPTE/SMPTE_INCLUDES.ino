
//LTC stuff borrowed from Arduino forum
#define icpPin 8        // ICP input pin on arduino -not needed as handled by interrupt
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
volatile boolean write_mtc_out;
volatile boolean drop_frame_flag;
volatile boolean drop_frame_flag_mtc;

volatile byte total_bits;
volatile bool current_bit;
volatile byte sync_count;

volatile byte tc[8];
volatile byte timeCodeLTC[12];
char timeCodeMTC[14];
char timeCodeLTC2[14];
char df = 'n';
char dfm = 'n';
char* flag_l;
char* flag_m;
int ctr = 0;
//MTC stuff from forum
byte h_m, m_m, s_m, f_m;      //hours, minutes, seconds, frame
char h_l, m_l, s_l, f_l;      //hours, minutes, seconds, frame
byte buf_temp_mtc[8];     //timecode buffer
byte command, data, index;


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
      sprintf(timeCodeLTC2, "LTC:%c:%s /r", df, timeCodeLTC);
      write_ltc_out = true;

    }
  }
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
    sprintf(timeCodeMTC, "MTC:%c:%.2d:%.2d:%.2d:%.2d ", dfm, h_m, m_m, s_m, f_m);
    SerialOne.println();
    write_mtc_out = true;
  }
}

void smpteSetup()
{
  MIDI.setHandleTimeCodeQuarterFrame(handleTimeCodeQuarterFrame);
  attachInterrupt(1, TIMER1_CAPT_vect, CHANGE);
  pinMode(icpPin, INPUT);                  // ICP pin (digital pin 8 on arduino) as input
  bit_time = 0;
  valid_tc_word = false;
  ones_bit_count = false;
  tc_sync = false;
  write_ltc_out = false;
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
