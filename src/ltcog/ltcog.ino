
#define icpPin 8        // ICP input pin on arduino
#define ONE_TIME_MAX          475 // THESE VALUES ARE SETUP FOR NTSC VIDEO
#define ONE_TIME_MIN          150 // PAL WOULD BE AROUND 1000 FOR 0 AND 500 FOR 1
#define ZERO_TIME_MAX         1900 // 80BITS TIMES 29.97 FRAMES PER SEC
#define ZERO_TIME_MIN         600 // EQUALS 833 (DIVIDE BY 8 CLOCK PULSES)

#define END_DATA_POSITION      63
#define END_SYNC_POSITION      77
#define END_SMPTE_POSITION     80

volatile unsigned int bitTime;
volatile boolean validTcWord;
volatile boolean onesBitCount;
volatile boolean tcSync;
volatile boolean writeTcOut;
volatile boolean dropFrameFlag;

volatile byte hoursLTC, minutesLTC, secondsLTC, framesLTC;      //hours, minutes, seconds, frame
volatile unsigned int MTCWord = 0;
String LTCWord= "This is a really long string";

volatile byte totalBits;
volatile bool currentBit;
volatile byte syncCount;

volatile unsigned int tc[10];
volatile unsigned int bufferLTC[10];
volatile char timeCodeLTC[13];

/* ICR interrupt vector */
ISR(TIMER4_CAPT_vect)
{
 //toggleCaptureEdge
 TCCR4B ^= _BV(ICES4);

 bitTime = ICR4;

 //resetTimer1
 TCNT4 = 0;

 if ((bitTime < ONE_TIME_MIN) || (bitTime > ZERO_TIME_MAX)) // get rid of anything way outside the norm
 {
   //Serial.println(bitTime, DEC);
   totalBits = 0;
 }
 else
 {
   if (onesBitCount == true) // only count the second ones pluse
  {
     onesBitCount = false;
  }
   else
   {
     if (bitTime > ZERO_TIME_MIN)
     {
       currentBit = 0;
       syncCount = 0;
     }
     else //if (bitTime < oneTimeMax)
     {
       onesBitCount = true;
       currentBit = 1;

     }

    for (int i = 0; i < 9; i ++) {
       tc[i] = (tc[i] >> 1) | ((tc[i + 1] & 1) ? 0x80 : 0);
       bufferLTC[i] = tc[i];
    }
   tc[9] = (tc[9] >> 1) | (currentBit ? 0x80 : 0);
     totalBits++;
   }

    if (tc[8] == 0xfc && tc[9] == 0xbf && totalBits >= 80)
    {
        validTcWord=true;
        totalBits = 0;
    }

   if (validTcWord)
   {
     validTcWord = false;
      framesLTC = (((bufferLTC[1] & 0x07) * 10) + (bufferLTC[0] & 0x0F));
      secondsLTC = (((bufferLTC[3] & 0x07) * 10) + (bufferLTC[2] & 0x0F));
      minutesLTC = (((bufferLTC[5] & 0x07) * 10) + (bufferLTC[4] & 0x0F));
      hoursLTC = (((bufferLTC[7] & 0x07) * 10) + (bufferLTC[6] & 0x0F));
      LTCWord = String((hoursLTC)) + ":" + String((minutesLTC)) + ":" + String(secondsLTC)+":" +String (framesLTC);

     dropFrameFlag = bit_is_set(tc[1], 2);

     writeTcOut = true;
   }
 }
}


void setup()
{

 Serial.begin (9600);
 pinMode(icpPin, INPUT);                  // ICP pin (digital pin 8 on arduino) as input

 bitTime = 0;
 validTcWord = false;
 onesBitCount = false;
 tcSync = false;
 writeTcOut = false;
 dropFrameFlag = false;
 totalBits =  0;
 currentBit =  0;
 syncCount =  0;

 //Serial.println("Finished setup ");
 //delay (1000);

 TCCR4A = B00000000; // clear all
 TCCR4B = B11000010; // ICNC1 noise reduction + ICES1 start on rising edge + CS11 divide by 8
 TCCR4C = B00000000; // clear all
 TIMSK4 = B00100000; // ICIE1 enable the icp

 TCNT4 = 0; // clear timer1
}

void loop()
{
   if (writeTcOut)
   {
     writeTcOut = false;
     if (dropFrameFlag)
       Serial.print("TC-[df] ");
     else
       Serial.print("TC-[nd] ");
     Serial.print(LTCWord);
     Serial.println("\r");
   }
}
