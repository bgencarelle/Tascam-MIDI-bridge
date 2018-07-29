
#define icpPin 8        // ICP input pin on arduino
#define one_time_max          475 // these values are setup for NTSC video
#define one_time_min          150 // PAL would be around 1000 for 0 and 500 for 1
#define zero_time_max         1900 // 80bits times 29.97 frames per sec
#define zero_time_min         600 // equals 833 (divide by 8 clock pulses)
                                  
#define end_data_position      63
#define end_sync_position      77
#define end_smpte_position     80

volatile unsigned int bit_time;
volatile boolean valid_tc_word;
volatile boolean ones_bit_count;
volatile boolean tc_sync;
volatile boolean write_tc_out;
volatile boolean drop_frame_flag;

volatile byte hoursLTC, minutesLTC, secondsLTC, framesLTC;      //hours, minutes, seconds, frame
volatile unsigned int MTCWord = 0;
String LTCWord= "This is a really long string";

volatile byte total_bits;
volatile bool current_bit;
volatile byte sync_count;

volatile unsigned int tc[10];
volatile unsigned int bufferLTC[10];
volatile char timeCode[13];

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
   //Serial.println(bit_time, DEC);
   total_bits = 0;
 }
 else
 {
   if (ones_bit_count == true) // only count the second ones pluse
  {
     ones_bit_count = false;
  }
   else
   {    
     if (bit_time > zero_time_min)
     {
       current_bit = 0;
       sync_count = 0;
     }
     else //if (bit_time < one_time_max)
     {
       ones_bit_count = true;
       current_bit = 1;

     }

    for (int i = 0; i < 9; i ++) {
       tc[i] = (tc[i] >> 1) | ((tc[i + 1] & 1) ? 0x80 : 0);
       bufferLTC[i] = tc[i];
    }
   tc[9] = (tc[9] >> 1) | (current_bit ? 0x80 : 0);
     total_bits++;
   }

    if (tc[8] == 0xfc && tc[9] == 0xbf && total_bits >= 80)
    {
        tc_sync=true;
        total_bits = 0;
    }
     if (tc_sync)
     {
       tc_sync = false;
       valid_tc_word = true;
     }
   
   
   if (valid_tc_word)
   {
     valid_tc_word = false;
      framesLTC = (((bufferLTC[1] & 0x07) * 10) + (bufferLTC[0] & 0x0F));
      secondsLTC = (((bufferLTC[3] & 0x07) * 10) + (bufferLTC[2] & 0x0F));
      minutesLTC = (((bufferLTC[5] & 0x07) * 10) + (bufferLTC[4] & 0x0F));
      hoursLTC = (((bufferLTC[7] & 0x07) * 10) + (bufferLTC[6] & 0x0F));
      LTCWord = String((hoursLTC)) + ":" + String((minutesLTC)) + ":" + String(secondsLTC)+":" +String (framesLTC);
     
     drop_frame_flag = bit_is_set(tc[1], 2);

     write_tc_out = true;
   }
 }
}


void setup()
{
  
 Serial.begin (115200);
 pinMode(icpPin, INPUT);                  // ICP pin (digital pin 8 on arduino) as input

 bit_time = 0;
 valid_tc_word = false;
 ones_bit_count = false;
 tc_sync = false;
 write_tc_out = false;
 drop_frame_flag = false;
 total_bits =  0;
 current_bit =  0;
 sync_count =  0;

 //Serial.println("Finished setup ");
 //delay (1000);

 TCCR1A = B00000000; // clear all
 TCCR1B = B11000010; // ICNC1 noise reduction + ICES1 start on rising edge + CS11 divide by 8
 TCCR1C = B00000000; // clear all
 TIMSK1 = B00100000; // ICIE1 enable the icp

 TCNT1 = 0; // clear timer1
}

void loop()
{
   if (write_tc_out)
   {
     write_tc_out = false;
     if (drop_frame_flag)
       Serial.print("TC-[df] ");
     else
       Serial.print("TC-[nd] ");
     Serial.print(LTCWord);
     Serial.println("\r");
   }
}

