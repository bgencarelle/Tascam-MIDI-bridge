#if defined (TIME_SYNC)

#if defined UNO
#define edgeCap TIMER1_CAPT_vect
#elif defined MEGA
#define edgeCap TIMER5_CAPT_vect
#endif

#if defined (TAS_644) || defined(TAS_688)
#define TC_MAX 900 //largest sized timecode (15minutes *60 seconds in a minute)
int tcOffset = -21;
#endif


bool chaseMode = true;
int pushButtonD4 = 4;
int pushButtonD3 = 3;

//LTC stuff borrowed from Arduino forum
#define oneTimeMax          475 // these values are setup for NTSC video
#define oneTimeMin          300 // PAL would be around 1000 for 0 and 500 for 1
#define zeroTimeMax         875 // 80bits times 29.97 frames per sec
#define zeroTimeMin         700 // equals 833 (divide by 8 clock pulses)

#define endDataPosition      63
#define endSyncPosition      77
#define endSmptePosition     80

/*********************************************************************
  HD: LTC "debouncing"
**********************************************************************/
// Amount of "debouce" time for LTC after sending PLAY
// since the LTC seems to take seconds to settle to a new value after PLAY is sent
#define LTC_DEBOUNCE_TIME_MS  2000
// The last time PLAY is sent
volatile unsigned long lastPlayTime = 0;

volatile unsigned int bitTime;
volatile bool validTcWord = false;
volatile bool onesBitCount = false;
volatile bool tcSync;
volatile byte totalBits;
volatile bool currentBit;
volatile byte syncCount;

char* flagLTC;
char* flagMTC;
//MTC stuff from forum
char timeCodeMTC[14];
char timeCodeLTC2[14];
volatile byte timeCodeLTC[14];
volatile bool writeLTCOut;
volatile bool writeMTCOut;
volatile bool dropFrameFlagLTC;
volatile bool dropFrameFlagMTC;
char dfm = 'n';
char dfl = 'n';
byte hoursMTC, minutesMTC, secondsMTC, framesMTC;      //hours, minutes, seconds, frame
byte hoursLTC, minutesLTC, secondsLTC, framesLTC;      //hours, minutes, seconds, frame
unsigned int MTCWord = 0;
unsigned int LTCWord = 0;
unsigned int OldLTCWord = 0;
unsigned int OldMTCWord = 0;

volatile byte bufferLTC[8];
volatile byte bufferMTC[8];     //timecode buffer
byte command, data, index;
int ctr = 0;

/* ICR interrupt vector */
ISR(edgeCap)
{
#if defined UNO
  //toggleCaptureEdge
  TCCR1B ^= _BV(ICES1);
  bitTime = ICR1;
  //resetTimer1
  TCNT1 = 0;
#elif defined MEGA
  //toggleCaptureEdge
  TCCR5B ^= _BV(ICES5);
  bitTime = ICR5;
  //resetTimer1
  TCNT5 = 0;
#endif

  if ((bitTime < oneTimeMin) || (bitTime > zeroTimeMax)) // get rid of anything way outside the norm
  {
    totalBits = 0;
  }
  else
  {
    if (onesBitCount == true) // only count the second ones pluse
      onesBitCount = false;
    else
    {
      if (bitTime > zeroTimeMin)
      {
        currentBit = 0;
        syncCount = 0;
      }
      else if (bitTime < oneTimeMax)
      {
        onesBitCount = true;
        currentBit = 1;
        syncCount++;
        if (syncCount == 12) // part of the last two bytes of a timecode word
        {
          syncCount = 0;
          tcSync = true;
          totalBits = endSyncPosition;
        }
      }

      if (totalBits <= endDataPosition) // timecode runs least to most so we need
      { // to shift things around
        bufferLTC[0] = bufferLTC[0] >> 1;

        for (int n = 1; n < 8; n++)
        {
          if (bufferLTC[n] & 1)
            bufferLTC[n - 1] |= 0x80;

          bufferLTC[n] = bufferLTC[n] >> 1;
        }

        if (currentBit == 1)
          bufferLTC[7] |= 0x80;
      }
      totalBits++;
    }

    if (totalBits == endSmptePosition) // we have the 80th bit
    {
      totalBits = 0;
      if (tcSync)
      {
        tcSync = false;
        validTcWord = true;
      }
    }
    if (validTcWord)
    {
      validTcWord = false;
      dropFrameFlagLTC = (bufferLTC[1] & 0x04) != 0;
      framesLTC = (((bufferLTC[1] & 0x07) * 10) + (bufferLTC[0] & 0x0F));
      secondsLTC = (((bufferLTC[3] & 0x07) * 10) + (bufferLTC[2] & 0x0F));
      minutesLTC = (((bufferLTC[5] & 0x07) * 10) + (bufferLTC[4] & 0x0F));
      hoursLTC = (((bufferLTC[7] & 0x07) * 10) + (bufferLTC[6] & 0x0F));
      LTCWord = ((hoursLTC * 225) << 4) + ((minutesLTC * 15) << 2) + secondsLTC;

      if ((LTCWord > 0) && (LTCWord == OldLTCWord + 1 || LTCWord == OldLTCWord))
      {
        writeLTCOut = true;
        if ( (OldLTCWord >= TC_MAX + tcOffset) && (OldLTCWord + 1 >= TC_MAX + tcOffset) && (LTCWord + 1 >= TC_MAX + tcOffset))
        {
          SerialOne.println("ZP");
          writeLTCOut = false;
          LTCWord = 0;
        }
      }
      else
      {
        writeLTCOut = false;
      }

      OldLTCWord = LTCWord;
    }
  }
}

void smpteSetup()
{
  MIDI.setHandleTimeCodeQuarterFrame(handleTimeCodeQuarterFrame);
  pinMode(icpPin, INPUT);// ICP pin (digital pin 8 on arduino uno) as input
  pinMode(pushButtonD4, INPUT_PULLUP);
  pinMode(pushButtonD3, INPUT_PULLUP);
  bitTime = 0;
  totalBits =  0;
  currentBit =  0;
  syncCount =  0;

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
  bufferMTC[index] = data;

  if (index >= 0x07)
  { //recalculate timecode once FRAMES LSB quarter-frame received
    dropFrameFlagMTC = (bufferMTC[1] & 0x04) != 0;
    hoursMTC = ((bufferMTC[7] & 0x01) << 4) + bufferMTC[6];//we will only look at 0 or 1 hour
    minutesMTC = (bufferMTC[5] << 4) + bufferMTC[4];
    secondsMTC = (bufferMTC[3] << 4) + bufferMTC[2];
    framesMTC = (bufferMTC[1] << 4) + bufferMTC[0];
    MTCWord = ((hoursMTC * 225) << 4) + ((minutesMTC << 2) * 15) + secondsMTC;
    if ( (OldMTCWord >= TC_MAX + tcOffset) && (OldMTCWord + 1 >= TC_MAX + tcOffset) && (MTCWord + 1 >= TC_MAX + tcOffset))
    {
      if (writeMTCOut)
      {
        SerialOne.print("ZP");
      }
      writeMTCOut = false;
    }
    else if (MTCWord > 0 && (MTCWord == OldMTCWord + 1 || LTCWord == OldMTCWord))
    {
      writeMTCOut = true;//return to zero
      chaseSync();
    }
    OldMTCWord = MTCWord;
  }
}

void chaseSync()
{
  int frameCompare = framesLTC - framesMTC;
  int wordCompare = LTCWord - MTCWord;
  long travelTime = 0;
  unsigned int lastLTCWord;
  int tuningWord = 2;
  unsigned long wordCompareDelay = (unsigned long)(((travelTime)) + (94 * ((unsigned long)(abs(wordCompare)))));
  SerialOne.println('P');
  if (writeLTCOut)
  {
    lastLTCWord = LTCWord;
    writeLTCOut = false;
    writeMTCOut = false;
    SerialOne.print(frameCompare, DEC);
    SerialOne.print(": ltc : ");
    SerialOne.print(LTCWord, DEC);
    SerialOne.print(" mtc : ");
    SerialOne.println(MTCWord, DEC);
    if ((wordCompare > 1) && (millis() - lastPlayTime > LTC_DEBOUNCE_TIME_MS))//LTC is leading
    {
     // SerialOne.println(wordCompareDelay);
      SerialOne.println('R');
      delay(wordCompareDelay);
      delayMicroseconds(10000);
      SerialOne.println();
      SerialOne.println('P');
      lastPlayTime = millis(); //Update last play time to wait for the next LTC to settle
    }

    else if ((wordCompare < -1) && (millis() - lastPlayTime > LTC_DEBOUNCE_TIME_MS))//LTC is following
    {
     // SerialOne.print(wordCompareDelay);
      SerialOne.println('Q');
      delay(wordCompareDelay);
      delayMicroseconds(10000);
      SerialOne.println();
      SerialOne.println('P');
      lastPlayTime = millis(); //Update last play time to wait for the next LTC to settle
    }
    else //if (wordCompare ==tolerance)
    {
      SerialOne.print(frameCompare, DEC);
      SerialOne.print(" ltc : ");
      SerialOne.print(LTCWord, DEC);
      SerialOne.print(" mtc : ");
      SerialOne.println(MTCWord, DEC);
    }
    return;
  }
  else if (!writeLTCOut && lastLTCWord >= 600)
  {
    writeMTCOut = false;
    SerialOne.println('R');
    delay(500);
    SerialOne.println('P');
    return;
  }
}

#endif
