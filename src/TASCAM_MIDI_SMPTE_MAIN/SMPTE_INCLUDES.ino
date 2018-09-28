#if defined (TIME_SYNC)

#if defined UNO
#define edgeCap TIMER1_CAPT_vect
#elif defined MEGA
#define edgeCap TIMER4_CAPT_vect
#endif

#if (defined (TAS_644_HIGH_SPEED) || defined(TAS_688))
#define TC_MAX 900 //largest sized timecode (15minutes *60 seconds in a minute)
#elif (defined (TAS_644_LOW_SPEED))
#define TC_MAX 1800
#endif
int tcOffset = -40;
unsigned int tcOffsetCalc = TC_MAX + tcOffset;

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
#define LTC_DEBOUNCE_TIME_MS  500
// The last time PLAY is sent
volatile unsigned long lastPlayTime = 0;
volatile unsigned int bitTime;
volatile bool validTcWord = false;
volatile bool onesBitCount = false;
volatile bool tcSync;
volatile byte totalBits;
volatile bool currentBit;
volatile byte syncCount;
enum lastTransport
{
  Stopped = 0,
  Play = 1,
  FF = 2,
  RW = 3,
};

char* flagLTC;
char* flagMTC;
//MTC stuff from forum
volatile bool dropFrameFlagLTC;
volatile bool dropFrameFlagMTC;
char dfm = 'n';
char dfl = 'n';
volatile byte hoursMTC, minutesMTC, secondsMTC, framesMTC;      //hours, minutes, seconds, frame
volatile unsigned int MTCWord = 0;
volatile unsigned int LTCWord = 0;
unsigned int oldLTCWord = 0;
unsigned int oldMTCWord = 0;

volatile byte bufferLTC[8];
volatile byte bufferMTC[8];     //timecode buffer
volatile byte command, data, index;


/* ICR interrupt vector */
ISR(edgeCap)
{
  
volatile byte hoursLTC, minutesLTC, secondsLTC, framesLTC;      //hours, minutes, seconds, frame

#if defined UNO
  //toggleCaptureEdge
  TCCR1B ^= _BV(ICES1);
  bitTime = ICR1;
  //resetTimer1
  TCNT1 = 0;
#elif defined MEGA
  //toggleCaptureEdge
  TCCR4B ^= _BV(ICES4);
  bitTime = ICR4;
  //resetTimer1
  TCNT4 = 0;
  
#endif

  if ((bitTime < oneTimeMin) || (bitTime > zeroTimeMax)) // get rid of anything way outside the norm
  {
      SerialOne.println(bitTime);
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
      unsigned int LTCWordCalc = (hoursLTC * 3600) + (minutesLTC * 60) + secondsLTC;
      if (LTCWordCalc < tcOffsetCalc)
      {
        LTCWord = LTCWordCalc;
      }
      else
      {
        LTCWord = tcOffsetCalc;
      }
      writeLTCOut = (LTCWord > 0);
     if (writeLTCOut)
     {
       oledLTC(LTCWord,LTCWord,secondsLTC,framesLTC);
     }
      oldLTCWord = LTCWord;
    }
  }
}

void smpteSetup()
{
  MIDI.setHandleTimeCodeQuarterFrame(handleTimeCodeQuarterFrame);
  //pinMode(syncControlPin, INPUT);
  pinMode(icpPin, INPUT);// ICP pin (digital pin 8 on arduino uno) as input
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
  attachInterrupt(1, TIMER4_CAPT_vect, CHANGE);
  TCCR4A = B00000000; // clear all
  TCCR4B = B11000010; // ICNC1 noise reduction + ICES1 start on rising edge + CS11 divide by 8
  TCCR4C = B00000000; // clear all
  TIMSK4 = B00100000; // ICIE1 enable the icp
  TCNT4 = 0; // clear timer1
#endif
}

void handleTimeCodeQuarterFrame(byte data)
{
  index = data >> 4;  //extract index/packet ID
  data &= 0x0F;       //clear packet ID from data
  bufferMTC[index] = data;

  if (index >= 0x07)
  { //recalculate timecode once FRAMES LSB quarter-frame received

    framesMTC = (bufferMTC[1] << 4) + bufferMTC[0];
    dropFrameFlagMTC = (bufferMTC[1] & 0x04) != 0;
    hoursMTC = ((bufferMTC[7] & 0x01) << 4) + bufferMTC[6];//we will only look at 0 or 1 hour
    minutesMTC = (bufferMTC[5] << 4) + bufferMTC[4];
    secondsMTC = (bufferMTC[3] << 4) + bufferMTC[2];
    MTCWord = (hoursMTC * 3600) + (minutesMTC * 60) + secondsMTC;
    writeMTCOut = ((MTCWord > 0) && (MTCWord == oldMTCWord + 1 || LTCWord == oldMTCWord));
    if (writeMTCOut )
    {
      chaseSync();
    }
    oldMTCWord = MTCWord;
  }
}


void chaseSync()
{
 volatile  long  wordCompare = 0;
  volatile unsigned long wordCompareDelay = 0;
  volatile unsigned long currentTime = 0;
  unsigned long oldTime=0;
  volatile long framesCompare=0;
  static long oldFramesCompare;
  if (((MTCWord < tcOffsetCalc) && (!writeLTCOut)))
  {
    SerialOne.println('P');
    playSpeed.play(MOTOR_SPEED_RUN);
    while (currentTime <=  150)      {
        currentTime = millis();
    }
    playSpeed.play(MOTOR_SPEED_RUN);
  }
  
  else if (((MTCWord < tcOffsetCalc) && (writeLTCOut)))
  {
    
    wordCompare = ((long)LTCWord - (long)MTCWord);
    oldTime = millis();
    
    if ((wordCompare > 2) )//LTC is leading
    {
      wordCompareDelay = abs(93 * ((wordCompare)));
      SerialOne.println('R');
      while (currentTime <=  oldTime + wordCompareDelay)
      {
        currentTime = millis();
        playSpeed.play(MOTOR_SPEED_HIGH);
      }
    
        playSpeed.play(MOTOR_SPEED_RUN);
    }

    else if ((wordCompare < -2))//LTC is following
    {
      wordCompareDelay =  abs(107 * (wordCompare));
      SerialOne.println('Q');

      while (currentTime <=  oldTime + wordCompareDelay)
      {
        currentTime = millis();
        playSpeed.play(MOTOR_SPEED_HIGH);
      }
        playSpeed.play(MOTOR_SPEED_RUN);
    }
    else if (((wordCompare >= -2) && (wordCompare <= 2)))
    {
 
//      framesCompare = long(320*(((((int)secondsLTC*30)+framesLTC)) - (((int)secondsMTC*30)+framesMTC))); 
//            while (currentTime <=  oldTime + wordCompareDelay)
//      {
//        currentTime = millis();
//        playSpeed.play(MOTOR_SPEED_RUN + oldFramesCompare);
//      }
//      playSpeed.play(MOTOR_SPEED_RUN + oldFramesCompare);
//      oldFramesCompare=framesCompare;
//   
}
  }

  else if (MTCWord >= tcOffsetCalc)
  {
    SerialOne.println('S');
  }
  writeLTCOut = false;
  writeMTCOut = false;

}

#endif


