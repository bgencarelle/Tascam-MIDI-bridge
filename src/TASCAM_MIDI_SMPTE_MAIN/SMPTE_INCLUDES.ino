#if defined (TIME_SYNC)

#if defined UNO
#define edgeCap TIMER1_CAPT_vect
#elif defined MEGA
#define edgeCap TIMER5_CAPT_vect
#endif

#if (defined (TAS_644_HIGH_SPEED) || defined(TAS_688))
#define TC_MAX 900 //largest sized timecode (15minutes *60 seconds in a minute)
#elif (defined (TAS_644_LOW_SPEED))
#define TC_MAX 1800
#endif

int tcOffset = -21;
unsigned int tcOffsetCalc = TC_MAX + tcOffset;
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
#define LTC_DEBOUNCE_TIME_MS  1000
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
volatile bool writeLTCOut;
volatile bool writeMTCOut;
volatile bool dropFrameFlagLTC;
volatile bool dropFrameFlagMTC;
char dfm = 'n';
char dfl = 'n';
volatile byte hoursMTC, minutesMTC, secondsMTC, framesMTC;      //hours, minutes, seconds, frame
volatile byte hoursLTC, minutesLTC, secondsLTC, framesLTC;      //hours, minutes, seconds, frame
volatile int framesCompare;
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
      unsigned int LTCWordCalc = (hoursLTC * 3600) + (minutesLTC * 60) + secondsLTC;
      if (LTCWordCalc < tcOffsetCalc)
      {
        LTCWord = LTCWordCalc;
      }
      else
      {
        LTCWord = tcOffsetCalc;
      }
      writeLTCOut = ((LTCWord > 0) && (LTCWord == oldLTCWord + 1 || LTCWord == oldLTCWord)&&(LTCWord<tcOffsetCalc));
      oldLTCWord=LTCWord;
    }
  }
}

void smpteSetup()
{
  MIDI.setHandleTimeCodeQuarterFrame(handleTimeCodeQuarterFrame);
  //pinMode(syncControlPin, INPUT);
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
    
    framesMTC = (bufferMTC[1] <<4) + bufferMTC[0];
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
  static bool ctrFlag;
  static unsigned int ctr=0;
  int tuningWord = 2;
  static byte state; 
  int wordCompare; 
  writeMTCOut = false;
  unsigned long oldTime;
  unsigned long currentTime;
  
  //SerialOne.println("valid mtc");
  
    if (ctrFlag)
  {
    ctr++;
  }
  else if(!ctrFlag)
  {
    ctr =0;
  }
  
  if (((MTCWord < tcOffsetCalc)&&(!writeLTCOut))&&(ctr<=2))
  {
    ctrFlag = true;
    SerialOne.println('P');
    delay(500);
    lastPlayTime = millis(); //Update last play time to wait for the next LTC to settle
    
  }
  else if ((MTCWord < tcOffsetCalc)&&(writeLTCOut))
  {
     
      oldTime = millis();
      currentTime=oldTime;
      writeLTCOut = false;
      unsigned int oldLTCWord=LTCWord;
      ctrFlag=false;
      wordCompare = LTCWord - MTCWord;
     // SerialOne.println("valid ltc");
      if ((wordCompare > 1) && ((millis() - lastPlayTime) > LTC_DEBOUNCE_TIME_MS))//LTC is leading
      {
        unsigned long wordCompareDelayRW =  (75 * (unsigned long)(abs(wordCompare)));
        SerialOne.println('R');
        SerialOne.println(wordCompareDelayRW/1000);
        do{
            wordCompareDelayRW =  (75 * (unsigned long)(abs(oldLTCWord - MTCWord)));
           currentTime = millis();
        }
        while(currentTime <= oldTime+wordCompareDelayRW);
        
        delayMicroseconds(16000);
        SerialOne.println();
        state = lastTransport::RW;
        lastPlayTime = millis(); //Update last play time to wait for the next LTC to settle
      }
      else if ((wordCompare < -1) && ((millis() - lastPlayTime) > LTC_DEBOUNCE_TIME_MS))//LTC is following
      {
        unsigned long wordCompareDelayFF =  (105 * (unsigned long)(abs(wordCompare)));
        SerialOne.println(wordCompareDelayFF/1000);
        SerialOne.println('Q');
        
        do{
          if((MTCWord >= tcOffsetCalc)||MTCWord<oldLTCWord)
           {
            SerialOne.println('R');
           }
          else
          {
            //
          }
           currentTime = millis();
        }while(currentTime <= oldTime+wordCompareDelayFF);
        
        delayMicroseconds(16000);
        SerialOne.println('P');
        state = lastTransport::FF;
        lastPlayTime = millis(); //Update last play time to wait for the next LTC to settle
      }
      else if ((wordCompare >= -1) && (wordCompare <= 1))
      {
        framesCompare = framesLTC - framesMTC;
        if ((framesCompare)>=3)
        {
          SerialOne.println("over");
        }
        else if (framesCompare <= -3)
        {
          SerialOne.println("under");           
        }
        else
        {
          SerialOne.println("good enough");
        }
         
        SerialOne.println(framesCompare);
        state = lastTransport::Play;
      }
    }
   else if(MTCWord >= tcOffsetCalc)
   {
    writeLTCOut = false;
    ctrFlag=true;
    SerialOne.println('S');
    delay(2000);
   }
   else if (ctr>=3)
  {
    writeLTCOut = false;
    ctrFlag=false;
    SerialOne.println('R');
    delay(3000);
    SerialOne.println('P');
    delay(2000);
    state = lastTransport::RW;
    lastPlayTime = millis(); //Update last play time to wait for the next LTC to settle
  }   
}

#endif


