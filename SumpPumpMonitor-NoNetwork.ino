/*
  Sump Pump Monitor
  -monitor 3 float sensors 
    1) normal operation
    2) stage 1 failure, water level higher than standard high level
    3) critical failure, water level very near flowing out of sump pit
  -include led status lights
  -include warning buzzer
 
*/
#include <SPI.h>
#include <RTClib.h>

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// morris code on a piezo transducer variables
 
//Specify digital pin on the Arduino that the positive lead of piezo buzzer is attached.
byte piezoPin = 7;

//specify the desired frequency
uint16_t Fz=3200; //3200 is loudest for my piezo

//input the tone pattern, enter 0 for a long pause between a series of tones
//Example Pattern for S.O.S = {300,300,300,0,600,600,600,0,300,300,300,0,0};
const PROGMEM uint16_t Pattern[]={300,300,300,0,600,600,600,0,300,300,300,0,0};

//determine the number of elements in the Pattern array
byte patternSize=12; //sizeof(Pattern)/sizeof(Pattern[0]); //count from first element  0

//set the standard delay between tones
uint16_t tShort=50; 

//set the pause duration for a 'long pause', must be greater than tShort
uint16_t tLong=150; 

//variable to loop through the Pattern array
byte counter=0;

//timing variable
unsigned long toneTime=0;

//pause an active alarm
unsigned long pauseAlarm = 0; 

//run test
byte testAlarm=0;
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// constants to set pin numbers:
const int testPin1 = 19;      //test/silence button
const int floatPin1 = 14;     // the number of the float 1 pin, normal operation float
const int floatPin2 = 15;     // the number of the float 2 pin, stage 1 pump failure
const int floatPin3 = 16;     // the number of the float 3 pin, sump pit water level critical
const int ledPin1 =  6;      // the number of the gren LED pin
const int ledPin2 =  5;      // the number of the yellow LED pin
const int ledPin3 =  3;      // the number of the red LED pin
const int ledPin4 = 2;       //Communications status

const unsigned long FlashInterval = 500; // led flash delay

// variables:
int testBut1 = 0;
int floatState1 = 0;         // variable for reading the float status
int floatState2 = 0;         // variable for reading the float status
int floatState3 = 0;         // variable for reading the float status
unsigned long greenFlash = 0;        // time variable to flash green led
unsigned long currentTime=0;
unsigned long t=0;

unsigned long sketchTime = 0;       // CPU milliseconds since last time server query.
  

void setup() {

  
  currentTime = (millis());

  // disable SD card if one in the slot
  pinMode(4,OUTPUT);
  digitalWrite(4,HIGH);

  // initialize the test button 
  pinMode(testPin1, INPUT);
  
  // initialize the LED pin as an output:
  pinMode(ledPin1, OUTPUT);
  pinMode(ledPin2, OUTPUT);  
  pinMode(ledPin3, OUTPUT);
  pinMode(ledPin4, OUTPUT);
  
  // initialize the pushbutton pin as an input:
  pinMode(floatPin1, INPUT);
  pinMode(floatPin2, INPUT);
  pinMode(floatPin3, INPUT);



  greenFlash = millis();

  // Set an initial time value 
  t = 0;
  sketchTime = millis(); 

    // start the SPI library:
  SPI.begin();

  // initialize serial communication at 9600 bits per second:
  Serial.begin(9600);


  // disable SD card if one in the slot
  pinMode(4,OUTPUT);
  digitalWrite(4,HIGH);

  
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  Serial.println("Initialization Complete");
}

void loop() {

  
  // read the state of the pushbutton value:
  floatState1 = digitalRead(floatPin1);
  floatState2 = digitalRead(floatPin2);
  floatState3 = digitalRead(floatPin3);
  testBut1 = digitalRead(testPin1);

  Serial.print("Float1: ");
  Serial.print(floatState1);
  Serial.print(", Float2: ");
  Serial.print(floatState2);
  Serial.print(", Float3: ");
  Serial.print(floatState3);
  Serial.print(", TestBut: ");
  Serial.println(testBut1);
  Serial.println(currentTime);
    Serial.println(millis());
  
  //reset greenFlash when millis rolls back to zero
  if (millis() < greenFlash) {
    greenFlash=millis();
   }

  currentTime=millis();

////test button state////

  //Pause alarm with test button
  if((floatState2 == LOW || floatState3 == LOW) && testBut1 == HIGH){
    if(currentTime+3600000<currentTime){ //checking for buffer overflow
      currentTime=0; 
      pauseAlarm=currentTime+3600000;//pause active alarm for 60 minutes
    }
    else{
      pauseAlarm=currentTime+3600000;//pause active alarm for 60 minutes
    }
  }

////Light Status Indicator////

  // check if the float 1 is under water. If it is, the floatState is LOW:
  if (floatState2 == HIGH && floatState3 == HIGH) {
    if (floatState1 == HIGH) {
      // turn LED on:
      digitalWrite(ledPin1, HIGH);
      digitalWrite(ledPin2, LOW);
      digitalWrite(ledPin3, LOW);
    } else if (floatState1 == LOW) {
      // blink LED:

      if (millis() < (greenFlash + FlashInterval) ) {
      digitalWrite(ledPin1, LOW);

      } 
      else if (millis() > (greenFlash + FlashInterval) && millis() < (greenFlash + 2*FlashInterval)) {
      digitalWrite(ledPin1, HIGH);  
      }
      
      else if (millis() > (greenFlash + 2*FlashInterval) ) {
      greenFlash=millis();  

      }
      
    }
  }
  
  // check if the float 2 is under water. If it is, the floatState is LOW:
  if (floatState3 == HIGH) {
    if (floatState2 == LOW) {
      // turn LED on:
      digitalWrite(ledPin2, HIGH);
      digitalWrite(ledPin1, HIGH);
      digitalWrite(ledPin3, LOW);
    } else {
      // turn LED off:
      digitalWrite(ledPin2, LOW);
    }
  }

  // check if the float 3 is under water. If it is, the floatState is HIGH:
  if (floatState3 == LOW) {
    // turn LED on:
    digitalWrite(ledPin3, HIGH);
    digitalWrite(ledPin1, HIGH);
    digitalWrite(ledPin2, HIGH);
  } else {
    // turn LED off:
    digitalWrite(ledPin3, LOW);
  }

////Sound the Alarm////

if((floatState2 == LOW || floatState3 == LOW || testBut1 == HIGH) && currentTime>(pauseAlarm))  {
if (millis() > (toneTime+pgm_read_word_near(Pattern + counter)+tShort) && pgm_read_word_near(Pattern + counter)!=0) {  

  toneTime=millis();
  tone(piezoPin, Fz, pgm_read_word_near(Pattern + counter ));
  //Serial.println(counter);
  Serial.println(pgm_read_word_near(Pattern + counter));
  counter=counter+1;
  
  if (counter>patternSize-1){
    counter=0;
  }
}

if (millis() > (toneTime+pgm_read_word_near(Pattern + counter)+tShort) && pgm_read_word_near(Pattern + counter)==0) {  

  toneTime=millis()+(tLong);
  //Serial.println(counter);
 Serial.println(pgm_read_word_near(Pattern + counter));
  counter=counter+1;
  
  if (counter>patternSize-1){
    counter=0;
  }
}
}


}





