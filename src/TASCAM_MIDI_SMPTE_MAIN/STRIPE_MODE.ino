#if defined (STRIPE_MODE)

#define MOTOR_SPEED_RUN 9600


void stripeSetup()
{
 
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

}

#endif
