#if defined (TIME_SYNC)

#if defined UNO
#define edgeCap TIMER1_CAPT_vect
#elif defined MEGA
#define edgeCap TIMER5_CAPT_vect
#endif

#if defined (Tas644) ||defined(Tas688)
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
* HD: chasesync nonblocking state machine
**********************************************************************/
typdef enum
{
  playPaused,  // Playing or paused
  rewindOrFastForward,  // Rewinding or fast forwarding
} chaseState_t;

volatile chaseState_t chaseState = playPaused;
unsigned int lastRWFFTime = 0; // Last rewind or fastforward
unsigned int lastDelayTime = 0; // Last delay time period for rewind or fastforward

/*********************************************************************
* HD: LTC "debouncing"
**********************************************************************/
// Amount of "debouce" time for LTC after sending PLAY
// since the LTC seems to take seconds to settle to a new value after PLAY is sent
#define LTC_DEBOUNCE_TIME_MS	2000
// The last time PLAY is sent
volatile unsigned long lastPlayTime = 0;

volatile unsigned int bitTime;
volatile bool validTcWord=false;
volatile bool onesBitCount=false;
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
      if (dropFrameFlagLTC)
      {
        dfl = 'd';
      }
      else
      {
        dfl = 'n';
      }
      framesLTC = (((bufferLTC[1] & 0x07) * 10) + (bufferLTC[0] & 0x0F));
      secondsLTC = (((bufferLTC[3] & 0x07) * 10) + (bufferLTC[2] & 0x0F));
      minutesLTC = (((bufferLTC[5] & 0x07) * 10) + (bufferLTC[4] & 0x0F));
      hoursLTC = (((bufferLTC[7] & 0x07) * 10) + (bufferLTC[6] & 0x0F));
      LTCWord = ((hoursLTC*225)<<4) + ((minutesLTC*15)<<2) + secondsLTC;
//      Serial1.println("LTC!!!");
      writeLTCOut = true;
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
    hoursMTC = ((bufferMTC[7] & 0x01) << 4) + bufferMTC[6];//we will only look at 0 or 1 hour
    minutesMTC = (bufferMTC[5] << 4) + bufferMTC[4];
    secondsMTC = (bufferMTC[3] << 4) + bufferMTC[2];
    framesMTC = (bufferMTC[1] << 4) + bufferMTC[0];
    MTCWord = ((hoursMTC*225)<<4) + ((minutesMTC<<2) * 15) + secondsMTC;

    dropFrameFlagMTC = (bufferMTC[1] & 0x04) != 0;
    if (dropFrameFlagMTC)
    {
      dfm = 'd';
    }
    else
    {
      dfm = 'n';
    }
    writeMTCOut = true;

}
}

void chaseSync()
{
  int frameCompare = framesLTC - framesMTC;
  int wordCompare = LTCWord - MTCWord;
  unsigned int wordCompareDelay = abs(wordCompare)<<6;
  writeLTCOut = false;
  writeMTCOut = false;
  
  SerialOne.print(" ltc : ");
  SerialOne.print(LTCWord,DEC);
  SerialOne.print(" mtc : ");
  SerialOne.println(MTCWord,DEC);  
  
  if (MTCWord >= TC_MAX+tcOffset)
  {
    SerialOne.print('Z'); //return to zero
    SerialOne.println('P');
    return;
  }
  else 
  {
    switch (chaseState)
    {
      case rewindOrFastForward:
        if (millis() - lastRWFFTime >= lastDelayTime) // If the program has rewinded or fastforwarded for long enough
        {
          // Send play
          SerialOne.println('P');
          SerialOne.println();
          // Update the last play time to wait for LTC to settle
          lastPlayTime = millis();
          // Switch to playing state
          chaseState = playPaused;
        }
        break;
      case playPaused:
        Serial.println(wordCompare)
        if ((abs(wordCompare) > 10) && (millis() - lastPlayTime > LTC_DEBOUNCE_TIME_MS))
        {          
          if (wordCompare > 0) // If LTC leads
          {
            SerialOne.println('R'); // Rewind
          }
          else  // If LTC lags
          {
            SerialOne.println('Q'); // Fast forward
          }
          // Update the last time REWIND or FASTFORWARD is sent
          lastRWFFTime = millis(); 
          // Update the amount of time before sending PLAY
          lastDelayTime = wordCompareDelay;
          // Switch to rewinding or fastforwarding state and wait for the next play
          chaseState = rewindOrFastForward;
        }
        break;
      default:
        break;
    }
  }
  return;//just in case
}

#endif
