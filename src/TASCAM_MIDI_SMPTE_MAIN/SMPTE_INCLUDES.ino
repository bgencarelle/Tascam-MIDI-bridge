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


//ff time 1:23
//rw time 1:16
//pin 13 on the controller handles chaseMode
//pin 5 is direction-rw is low ff/play/stop is high
//pin 6 is tach pulse  3.22 hz is play, rw is between 25 and ~78hz ends low, ff is 44 to 30 hz,play is 2.92hz
#define MOTOR_SPEED_LOW 9200
#define MOTOR_SPEED_RUN 9600
#define MOTOR_SPEED_HIGH 9900

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
volatile bool writeLTCOut;
volatile bool writeMTCOut;
volatile bool dropFrameFlagLTC;
volatile bool dropFrameFlagMTC;
char dfm = 'n';
char dfl = 'n';
volatile byte hoursMTC, minutesMTC, secondsMTC, framesMTC;      //hours, minutes, seconds, frame
volatile byte hoursLTC, minutesLTC, secondsLTC, framesLTC;      //hours, minutes, seconds, frame
volatile int framesCompare;
volatile int MTCWord = 0;
volatile int LTCWord = 0;

volatile byte bufferLTC[8];
volatile byte bufferMTC[8];     //timecode buffer
volatile byte command, data, index;

volatile unsigned int oldMTCWord =0;
volatile unsigned int oldLTCWord=0;

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
      byte framesTempLTC = (((bufferLTC[1] & 0x07) * 10) + (bufferLTC[0] & 0x0F));
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
      if (writeLTCOut)
      {
        framesLTC =framesTempLTC;
      }
      oldLTCWord=LTCWord;
    }
  }
}

void smpteSetup()
{

  MIDI.setHandleTimeCodeQuarterFrame(handleTimeCodeQuarterFrame);
  //pinMode(syncControlPin, INPUT);
  pinMode(icpPin, INPUT);// ICP pin (digital pin 8 on arduino uno) as input
  pinMode(chaseFrequencyPin,OUTPUT);
  pinMode(chaseStateControlPin,OUTPUT);
  digitalWrite(chaseStateControlPin, LOW);
  delay(100);
  digitalWrite(chaseStateControlPin, HIGH);
  delay(100);
  digitalWrite(chaseStateControlPin, LOW);
  delay(100);
  digitalWrite(chaseFrequencyPin, LOW);
  
  playspeed.begin(chaseFrequencyPin);//
  playspeed.play(MOTOR_SPEED_RUN);//
  digitalWrite(chaseStateControlPin, LOW);
  digitalWrite(chaseFrequencyPin, LOW);
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
    
    unsigned int tempFramesMTC = (bufferMTC[1] <<4) + bufferMTC[0];
    dropFrameFlagMTC = (bufferMTC[1] & 0x04) != 0;
    hoursMTC = ((bufferMTC[7] & 0x01) << 4) + bufferMTC[6];//we will only look at 0 or 1 hour
    minutesMTC = (bufferMTC[5] << 4) + bufferMTC[4];
    secondsMTC = (bufferMTC[3] << 4) + bufferMTC[2];
    MTCWord = (hoursMTC * 3600) + (minutesMTC * 60) + secondsMTC;
    writeMTCOut = ((MTCWord > 0) && (MTCWord == oldMTCWord + 1 || LTCWord == oldMTCWord));
    if (writeMTCOut )
    {
      chaseSync();
      framesMTC=tempFramesMTC;
    }
    oldMTCWord = MTCWord;
  }
}


void chaseSync()
{
  static unsigned int motorSpeed =MOTOR_SPEED_HIGH;
  static unsigned int oldMotorSpeed=motorSpeed;
  static bool ctrFlag;
  static unsigned int ctr=0;
  int tuningWord = 1;
  static byte state; 
  volatile int wordCompare; 
  unsigned long oldTime;
  unsigned long currentTime;
  unsigned int oldLTCWord;
  static int oldFramesCompare;
  volatile unsigned long wordCompareDelayRW;
  volatile unsigned long wordCompareDelayFF;  
  playspeed.play(oldMotorSpeed);
  
    if (ctrFlag)
  {
    ctr++;
  }
  else if(!ctrFlag)
  {
    ctr =0;
  }
  
  if (((MTCWord < tcOffsetCalc)&&(!writeLTCOut)))
  {
    SerialOne.println('P');
    writeMTCOut = false;
  }
  
   else if((MTCWord >= tcOffsetCalc) && (!writeLTCOut))
   {
    SerialOne.println('R');
    writeMTCOut = false;
   }
   
  else if ((MTCWord < tcOffsetCalc)&&(writeLTCOut))
  {
      oldTime = millis();
      currentTime=oldTime; 
      writeLTCOut = false;
      writeMTCOut = false;      

      if ((wordCompare > tuningWord))//LTC is leading
      {
      wordCompare = LTCWord - MTCWord;
      wordCompareDelayRW =  (75 * (unsigned long)(abs(wordCompare)));   
        SerialOne.println('R');
        while(currentTime <= oldTime+wordCompareDelayRW)
        {
           currentTime = millis();
        }
      }
      
      else if ((wordCompare < -tuningWord))//LTC is following
      {
      wordCompare = LTCWord - MTCWord;
      wordCompareDelayFF =  (105 * (unsigned long)(abs(wordCompare)));  
        SerialOne.println('Q');
        while(currentTime <= oldTime+wordCompareDelayFF)
        {
          if(MTCWord >= tcOffsetCalc)
           {
            SerialOne.println('S');
            wordCompareDelayFF =0;
           }
           currentTime = millis();
        }
      }
      
      else if ((wordCompare >= -tuningWord) && (wordCompare <= tuningWord))
      {
        
        framesCompare = (30*wordCompare)+(framesLTC - framesMTC);
        if (((framesCompare)>=2))
        {
          motorSpeed -= 25;
          playspeed.play(motorSpeed);
        SerialOne.println(framesCompare);
        }
        else if (((framesCompare <= -2) ))
        {
          motorSpeed += 25;  
          playspeed.play(motorSpeed);  
           
        SerialOne.println(framesCompare);    
        }
        else
        {
          motorSpeed=MOTOR_SPEED_RUN;
          playspeed.play(motorSpeed);
        SerialOne.println(framesCompare);
        }
        
        oldFramesCompare=framesCompare;
        SerialOne.println(motorSpeed,DEC);
        oldMotorSpeed = motorSpeed;
      }
    }
    if (motorSpeed>MOTOR_SPEED_HIGH || motorSpeed<MOTOR_SPEED_LOW)
    {
      motorSpeed=MOTOR_SPEED_RUN;
    }
}

#endif


